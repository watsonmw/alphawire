#include "aw-backend-wia.h"

#include <windows.h>
#include <Wia.h>
#include <stdio.h>

#include "win-utils.h"

// MinGW headers are missing this id - we need it to lookup usb vendor and product id
#ifndef WIA_DIP_PNP_ID
#define WIA_DIP_PNP_ID 0x00000011
#endif

typedef MSTRUCTPACKED(struct sWiaPtpRequest {
    u16 OpCode;
    u32 SessionId;
    u32 TransactionId;
    u32 Params[PTP_MAX_PARAMS];
    u32 NumParams;
    u32 NextPhase;
    u8 VendorWriteData[];
}) WiaPtpRequest;

typedef MSTRUCTPACKED(struct sWiaPtpResponse {
    u16 ResponseCode;
    u32 SessionId;
    u32 TransactionId;
    u32 Params[PTP_MAX_PARAMS];
    u8 VendorReadData[];
}) WiaPtpResponse;

#define ESCAPE_PTP_ADD_REM_PARM1  0x0000
#define ESCAPE_PTP_ADD_REM_PARM2  0x0001
#define ESCAPE_PTP_ADD_REM_PARM3  0x0002
#define ESCAPE_PTP_ADD_REM_PARM4  0x0003
#define ESCAPE_PTP_ADD_REM_PARM5  0x0004
#define ESCAPE_PTP_ADD_OBJ_CMD    0x0010
#define ESCAPE_PTP_REM_OBJ_CMD    0x0020
#define ESCAPE_PTP_ADD_OBJ_RESP   0x0040
#define ESCAPE_PTP_REM_OBJ_RESP   0x0080
#define ESCAPE_PTP_VENDOR_COMMAND 0x0100
#define ESCAPE_PTP_CLEAR_STALLS   0x0200

static void PrintComPropertyValue(MAllocator* allocator, PROPVARIANT* pValue) {
    MStr utf8Str;
    switch (pValue->vt) {
        case VT_BSTR:
            utf8Str = WinUtils_BSTRToUTF8(allocator, pValue->bstrVal);
            if (utf8Str.str) {
                MLogf("String: %.*s\n", utf8Str.size, utf8Str.str);
                MStrFree(allocator, utf8Str);
            }
            break;
        case VT_I4:
            MLogf("Integer: %ld", pValue->lVal);
            break;
        case VT_UI4:
            MLogf("Unsigned: %lu", pValue->ulVal);
            break;
    }
}

typedef struct {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IWiaEventCallback* This, REFIID riid, void** ppvObject);
    ULONG (STDMETHODCALLTYPE *AddRef)(IWiaEventCallback* This);
    ULONG (STDMETHODCALLTYPE *Release)(IWiaEventCallback* This);
    HRESULT (STDMETHODCALLTYPE *ImageEventCallback)(IWiaEventCallback* This, const GUID* pEventGuid,
                                                    BSTR bstrEventDescription, BSTR bstrDeviceID,
                                                    BSTR bstrDeviceDescription, DWORD dwDeviceType,
                                                    BSTR bstrFullItemName, ULONG* pulEventType, ULONG ulReserved);
} WiaEventCallbackVtbl;

typedef struct {
    const WiaEventCallbackVtbl* lpVtbl;
    LONG refCount;
    AwWiaDeviceList* deviceList;
} WiaEventCallback;

static ULONG STDMETHODCALLTYPE WiaEventCallback_AddRef(IWiaEventCallback* This) {
    WiaEventCallback* self = (WiaEventCallback*)This;
    return InterlockedIncrement(&self->refCount);
}

static HRESULT STDMETHODCALLTYPE WiaEventCallback_QueryInterface(IWiaEventCallback* This, REFIID riid,
                                                                 void** ppvObject) {
    WiaEventCallback* self = (WiaEventCallback*)This;

    if (!ppvObject) {
        return E_INVALIDARG;
    }

    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWiaEventCallback)) {
        *ppvObject = self;
        WiaEventCallback_AddRef(This);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE WiaEventCallback_Release(IWiaEventCallback* This) {
    WiaEventCallback* self = (WiaEventCallback*)This;
    LONG count = InterlockedDecrement(&self->refCount);

    if (count == 0) {
        MFree(self->deviceList->allocator, self, sizeof(WiaEventCallback));
    }

    return count;
}

// WIA Event callback function - called from a WIA thread - not the main thread
// TODO: Add locking for device list iteration
static HRESULT STDMETHODCALLTYPE WiaEventCallback_ImageEventCallback(IWiaEventCallback* This,
                                                                     const GUID* pEventGuid,
                                                                     BSTR bstrEventDescription,
                                                                     BSTR bstrDeviceID,
                                                                     BSTR bstrDeviceDescription,
                                                                     DWORD dwDeviceType,
                                                                     BSTR bstrFullItemName,
                                                                     ULONG* pulEventType,
                                                                     ULONG ulReserved) {

    if (IsEqualGUID(pEventGuid, &WIA_EVENT_DEVICE_CONNECTED)) {
        WiaEventCallback* self = (WiaEventCallback*)This;
        self->deviceList->deviceListUpToDate = FALSE;
        MLogf("Device Connected");
    } else if (IsEqualGUID(pEventGuid, &WIA_EVENT_DEVICE_DISCONNECTED)) {
        WiaEventCallback* self = (WiaEventCallback*)This;
        for (int i = 0; i < MArraySize(self->deviceList->openDevices); ++i) {
            AwDeviceWia* device = self->deviceList->openDevices + i;
            HRESULT hr = VarBstrCmp(bstrDeviceID, device->deviceId, LOCALE_USER_DEFAULT, 0);
             if (hr == VARCMP_EQ) {
                 device->disconnected = TRUE;
            }
        }
        self->deviceList->deviceListUpToDate = FALSE;
        MLogf("Device Disconnected");
    }

    return S_OK;
}

static const WiaEventCallbackVtbl WiaEventCallback_Vtbl = {
    WiaEventCallback_QueryInterface,
    WiaEventCallback_AddRef,
    WiaEventCallback_Release,
    WiaEventCallback_ImageEventCallback
};

// Helper function to create the event callback object
static HRESULT CreateEventCallback(AwWiaDeviceList* self, IWiaEventCallback** ppCallback) {
    WiaEventCallback* callback;

    if (!ppCallback) {
        return E_INVALIDARG;
    }

    *ppCallback = NULL;

    callback = (WiaEventCallback*)MMalloc(self->allocator, sizeof(WiaEventCallback));
    if (!callback) {
        return E_OUTOFMEMORY;
    }

    // Initialize the callback object
    callback->lpVtbl = &WiaEventCallback_Vtbl;
    callback->refCount = 1;
    callback->deviceList = self;

    *ppCallback = (IWiaEventCallback*)callback;
    return S_OK;
}

static HRESULT RegisterForDeviceConnectDisconnectEvents(AwWiaDeviceList* self) {
    HRESULT hr = S_OK;
    IWiaEventCallback* pCallback = NULL;

    // Create event callback object
    // Note: You'll need to implement IWiaEventCallback interface
    hr = CreateEventCallback(self, &pCallback);
    if (FAILED(hr)) {
        goto cleanup;
    }

    IUnknown *pEventObject1;
    hr = self->deviceMgr->lpVtbl->RegisterEventCallbackInterface(self->deviceMgr, 0, NULL,
                                                                 &WIA_EVENT_DEVICE_CONNECTED, pCallback,
                                                                 &pEventObject1);
    if (FAILED(hr)) {
        goto cleanup;
    }

    IUnknown *pEventObject2;
    hr = self->deviceMgr->lpVtbl->RegisterEventCallbackInterface(self->deviceMgr, 0, NULL,
                                                                 &WIA_EVENT_DEVICE_DISCONNECTED, pCallback,
                                                                 &pEventObject2);
    if (FAILED(hr)) {
        pEventObject1->lpVtbl->Release(pEventObject1);
    } else {
        MArrayAdd(self->allocator, self->eventListeners, pEventObject1);
        MArrayAdd(self->allocator, self->eventListeners, pEventObject2);
    }

cleanup:
    if (pCallback) {
        pCallback->lpVtbl->Release(pCallback);
    }
    return hr;
}

AwResult AwWiaDeviceList_Open(AwWiaDeviceList* self) {
    HRESULT hr = 0;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        if (hr == RPC_E_CHANGED_MODE) {
            // Already initialized with a different mode.
            // We proceed and hope for the best, but we MUST NOT call CoUninitialize later.
            self->comInitialized = FALSE;
        } else {
            AW_ERROR_F("Failed to initialize COM: %08x", hr);
            return (AwResult){.code = AW_RESULT_TRANSPORT_ERROR};
        }
    } else {
        // hr is S_OK or S_FALSE, meaning we successfully incremented the COM ref count.
        self->comInitialized = TRUE;
    }

    // Create WIA Device Manager instance
    hr = CoCreateInstance(&CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,&IID_IWiaDevMgr,
        (void**)&self->deviceMgr);

    if (SUCCEEDED(hr) && self->deviceMgr) {
        RegisterForDeviceConnectDisconnectEvents(self);
        return (AwResult){.code = AW_RESULT_OK};
    } else {
        AW_ERROR_F("Failed to create WIA Device Manager: %08x", hr);
        return (AwResult){.code = AW_RESULT_TRANSPORT_ERROR};
    }
}

AwResult AwWiaDeviceList_ReleaseList(AwWiaDeviceList* self) {
    if (self->devices) {
        MArrayEachPtr(self->devices, it) {
            if (it.p->deviceId) {
                SysFreeString(it.p->deviceId); it.p->deviceId = NULL;
            }
        }
        MArrayInit(self->allocator, self->devices, 0);
    }
    return (AwResult){.code=AW_RESULT_OK};
}

AwResult AwWiaDeviceList_Close(AwWiaDeviceList* self) {
    AwResult r = AwWiaDeviceList_ReleaseList(self);
    if (self->deviceMgr) {
        self->deviceMgr->lpVtbl->Release(self->deviceMgr);
        self->deviceMgr = NULL;
    }
    if (self->eventListeners) {
        MArrayEachPtr(self->eventListeners, it) {
            IUnknown* eventObject = *it.p;
            eventObject->lpVtbl->Release(eventObject);
        }
        MArrayFree(self->allocator, self->eventListeners);
    }
    if (self->devices) {
        MArrayFree(self->allocator, self->devices);
    }
    if (self->openDevices) {
        MArrayFree(self->allocator, self->openDevices);
    }
    if (self->comInitialized) {
        CoUninitialize();
        self->comInitialized = FALSE;
    }

    return r;
}

b32 AwWiaDeviceList_NeedsRefresh(AwWiaDeviceList* self) {
    return (self->deviceListUpToDate == FALSE);
}

AwResult AwWiaDeviceList_RefreshList(AwWiaDeviceList* self, AwDeviceInfo** deviceList) {
    AwWiaDeviceList_ReleaseList(self);

    IEnumWIA_DEV_INFO* pEnum = NULL;
    HRESULT hr = self->deviceMgr->lpVtbl->EnumDeviceInfo(self->deviceMgr, WIA_DEVINFO_ENUM_LOCAL, &pEnum);
    if (FAILED(hr)) {
        goto cleanup;
    }

    ULONG countElements = 0;
    hr = pEnum->lpVtbl->GetCount(pEnum, &countElements);

#define PROP_NUM 4
    PROPSPEC propSpec[PROP_NUM];
    PROPVARIANT propVar[PROP_NUM];
    propSpec[0].ulKind = PRSPEC_PROPID;
    propSpec[0].propid = WIA_DIP_DEV_ID;
    propSpec[1].ulKind = PRSPEC_PROPID;
    propSpec[1].propid = WIA_DIP_VEND_DESC;
    propSpec[2].ulKind = PRSPEC_PROPID;
    propSpec[2].propid = WIA_DIP_DEV_NAME;
    propSpec[3].ulKind = PRSPEC_PROPID;
    propSpec[3].propid = WIA_DIP_PNP_ID;

    MArrayInit(self->allocator, self->devices, countElements);
    for (int i = 0; i < countElements; i++) {
        ULONG countFetched = 0;
        IWiaPropertyStorage* pWiaPropertyStorage = NULL;
        hr = pEnum->lpVtbl->Next(pEnum, 1, &pWiaPropertyStorage, &countFetched);
        if (FAILED(hr) || countFetched == 0) {
            break;
        }

        // Get device ID
        hr = pWiaPropertyStorage->lpVtbl->ReadMultiple(pWiaPropertyStorage, PROP_NUM, propSpec, propVar);
        if (FAILED(hr)) {
            goto next;
        }

        WiaDeviceInfo* wiaDeviceInfo = MArrayAddPtr(self->allocator, self->devices);
        AwDeviceInfo* deviceInfo = MArrayAddPtrZ(self->allocator, *deviceList);
        wiaDeviceInfo->deviceId = SysAllocString(propVar[0].bstrVal);
        deviceInfo->manufacturer = WinUtils_BSTRToUTF8(self->allocator, propVar[1].bstrVal);
        deviceInfo->product = WinUtils_BSTRToUTF8(self->allocator, propVar[2].bstrVal);

        if (propVar[3].vt == VT_BSTR) {
            MStr pnpId = WinUtils_BSTRToUTF8(self->allocator, propVar[3].bstrVal);
            if (pnpId.str) {
                // Parse \\?\usb#vid_054c&pid_0e0c#d085e0154e26#{6bdd1fc6-810f-11d0-bec7-08002be2092f}
                sscanf_s(pnpId.str, "\\\\?\\usb#vid_%04hx&pid_%04hx", &deviceInfo->usbVID, &deviceInfo->usbPID);
                MStrFree(self->allocator, pnpId);
            }
        }

        deviceInfo->backendType = AW_BACKEND_WIA;
        deviceInfo->device = wiaDeviceInfo;
        AW_INFO_F("Found device: %.*s (%.*s)", deviceInfo->product.size, deviceInfo->product.str,
            deviceInfo->manufacturer.size, deviceInfo->manufacturer.str);

        for (int j = 0; j < PROP_NUM; j++) {
            PropVariantClear(propVar + j);
        }
#undef PROP_NUM

next:
        if (pWiaPropertyStorage) {
            pWiaPropertyStorage->lpVtbl->Release(pWiaPropertyStorage);
            pWiaPropertyStorage = NULL;
        }
    }
    self->deviceListUpToDate = TRUE;

cleanup:
    if (pEnum) {
        pEnum->lpVtbl->Release(pEnum);
    }

    if (SUCCEEDED(hr)) {
        return (AwResult){.code=AW_RESULT_OK};
    } else {
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }
}

static void* AwDeviceWia_ReallocBuffers(AwDevice* self, AwBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    size_t headerSize = 0;
    if (type == AW_BUFFER_IN) {
        headerSize = sizeof(WiaPtpRequest);
    } else if (type == AW_BUFFER_OUT) {
        headerSize = sizeof(WiaPtpResponse);
    }

    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        CoTaskMemFree(mem); dataMem = NULL;
    }
    dataMem = CoTaskMemAlloc(dataSize);
    memset(dataMem, 0, dataSize);
    return ((u8*)dataMem) + headerSize;
}

static void AwDeviceWia_FreeBuffers(AwDevice* self, AwBufferType type, void* dataMem, size_t dataSize) {
    size_t headerSize = 0;
    if (type == AW_BUFFER_IN) {
        headerSize = sizeof(WiaPtpRequest);
    } else if (type == AW_BUFFER_OUT) {
        headerSize = sizeof(WiaPtpResponse);
    }

    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        CoTaskMemFree(mem); dataMem = NULL;
    }
}

static AwResult AwDeviceWia_SendAndRecv(AwDevice* self, AwPtpRequestHeader* request, u8* dataIn, size_t dataInSize,
                                         AwPtpResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                         size_t* actualDataOutSize) {

    WiaPtpRequest* requestData = (WiaPtpRequest*)(dataIn-sizeof(WiaPtpRequest));

    requestData->OpCode = request->OpCode;
    requestData->SessionId = request->SessionId;
    requestData->TransactionId = request->TransactionId;
    requestData->NumParams = request->NumParams;
    for (int i = 0; i < request->NumParams; i++) {
        requestData->Params[i] = request->Params[i];
    }
    requestData->NextPhase = request->NextPhase;

    AwDeviceWia* wia = ((AwDevice*)self)->device;
    DWORD dwActualDataOutSize = 0;
    WiaPtpResponse* responseData = (WiaPtpResponse*)(dataOut - sizeof(WiaPtpResponse));

    HRESULT hr = wia->device->lpVtbl->Escape(wia->device, ESCAPE_PTP_VENDOR_COMMAND,
                                             (u8*)requestData, dataInSize + sizeof(WiaPtpRequest),
                                             (u8*)responseData, dataOutSize + sizeof(WiaPtpResponse),
                                             &dwActualDataOutSize);

    *actualDataOutSize = dwActualDataOutSize;

    if (SUCCEEDED(hr)) {
        response->ResponseCode = responseData->ResponseCode;
        response->SessionId = responseData->SessionId;
        response->TransactionId = responseData->TransactionId;
        for (int i = 0; i < PTP_MAX_PARAMS; i++) {
            response->Params[i] = responseData->Params[i];
        }
        if (response->ResponseCode == PTP_OK) {
            return (AwResult){.code=AW_RESULT_OK,.ptp=responseData->ResponseCode};
        } else {
            return (AwResult){.code=AW_RESULT_PTP_FAILURE,.ptp=responseData->ResponseCode};
        }
    } else {
        WinUtils_LogLastError(&wia->logger, "Error sending WIA request");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }
}

AwResult AwWiaDeviceList_OpenDevice(AwWiaDeviceList* self, AwDeviceInfo* deviceInfo, AwDevice** deviceOut) {
    HRESULT hr = 0;
    IWiaItem* pWiaItemRoot = NULL;
    IWiaItemExtras* pWiaItemExtras = NULL;

    WiaDeviceInfo* wiaDeviceInfo = deviceInfo->device;

    // Create device
    hr = self->deviceMgr->lpVtbl->CreateDevice(self->deviceMgr, (BSTR)wiaDeviceInfo->deviceId, &pWiaItemRoot);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Query for IWiaItemExtras interface
    hr = pWiaItemRoot->lpVtbl->QueryInterface(pWiaItemRoot, &IID_IWiaItemExtras, (void**)&pWiaItemExtras);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Store the interface pointer
    AwDeviceWia* wiaDevice = MArrayAddPtr(self->allocator, self->openDevices);
    wiaDevice->device = pWiaItemExtras;
    wiaDevice->deviceId = wiaDeviceInfo->deviceId;
    wiaDevice->disconnected = FALSE;
    wiaDevice->logger = self->logger;

    (*deviceOut)->transport.reallocBuffer = AwDeviceWia_ReallocBuffers;
    (*deviceOut)->transport.freeBuffer = AwDeviceWia_FreeBuffers;
    (*deviceOut)->transport.sendAndRecv = AwDeviceWia_SendAndRecv;
    (*deviceOut)->transport.requiresSessionOpenClose = FALSE;
    (*deviceOut)->device = wiaDevice;
    (*deviceOut)->backendType = AW_BACKEND_WIA;
    (*deviceOut)->disconnected = FALSE;
    (*deviceOut)->logger = self->logger;

    pWiaItemExtras = NULL; // Prevent release in cleanup, ownership is now with AwDeviceWia

cleanup:
    if (pWiaItemRoot) {
         pWiaItemRoot->lpVtbl->Release(pWiaItemRoot);
    }

    if (pWiaItemExtras) {
        pWiaItemExtras->lpVtbl->Release(pWiaItemExtras);
    }

    if (SUCCEEDED(hr)) {
        return (AwResult){.code=AW_RESULT_OK};
    } else {
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }
}

AwResult AwWiaDeviceList_CloseDevice(AwWiaDeviceList* self, AwDevice* device) {
    AwDeviceWia* deviceWia = device->device;
    if (deviceWia->device) {
        deviceWia->device->lpVtbl->Release(deviceWia->device);
        deviceWia->device = NULL;
    }

    MArrayEachPtr(self->openDevices, it) {
        if (it.p == deviceWia) {
            MArrayRemoveIndex(self->openDevices, it.i);
            break;
        }
    }

    return (AwResult){.code=AW_RESULT_OK};
}

AwResult AwWiaDeviceList_Reset(AwWiaDeviceList* self, AwDeviceWia* device) {
    DWORD dwActualDataOutSize = 0;

    u8* memIn = CoTaskMemAlloc(1000);
    memset(memIn, 0, 1000);

    u8* memOut = CoTaskMemAlloc(1000);
    memset(memOut, 0, 1000);

    HRESULT hr = device->device->lpVtbl->Escape(device->device, ESCAPE_PTP_CLEAR_STALLS,
                                                memIn, 1000, memOut, 1000,
                                                &dwActualDataOutSize);

    if (memIn) {
        CoTaskMemFree(memIn);
    }

    if (memOut) {
        CoTaskMemFree(memOut);
    }

    if (SUCCEEDED(hr)) {
        return (AwResult){.code=AW_RESULT_OK};
    } else {
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }
}

static AwResult AwWiaDeviceList_Close_(AwBackend* backend) {
    AwWiaDeviceList* self = backend->self;
    AwResult r = AwWiaDeviceList_Close(self);
    MFree(self->allocator, self, sizeof(AwWiaDeviceList));
    return r;
}

static AwResult AwWiaDeviceList_RefreshList_(AwBackend* backend, AwDeviceInfo** deviceList) {
    AwWiaDeviceList* self = backend->self;
    return AwWiaDeviceList_RefreshList(self, deviceList);
}

static b32 AwWiaDeviceList_NeedsRefresh_(AwBackend* backend) {
    AwWiaDeviceList* self = backend->self;
    return AwWiaDeviceList_NeedsRefresh(self);
}

static AwResult AwWiaDeviceList_ReleaseList_(AwBackend* backend) {
    AwWiaDeviceList* self = backend->self;
    return AwWiaDeviceList_ReleaseList(self);
}

static AwResult AwWiaDeviceList_OpenDevice_(AwBackend* backend, AwDeviceInfo* deviceInfo, AwDevice** deviceOut) {
    AwWiaDeviceList* self = backend->self;
    return AwWiaDeviceList_OpenDevice(self, deviceInfo, deviceOut);
}

static AwResult AwWiaDeviceList_CloseDevice_(AwBackend* backend, AwDevice* device) {
    AwWiaDeviceList* self = backend->self;
    return AwWiaDeviceList_CloseDevice(self, device);
}

AwResult AwWiaDeviceList_OpenBackend(AwBackend* backend) {
    AwWiaDeviceList* deviceList = MMallocZ(backend->allocator, sizeof(AwWiaDeviceList));
    backend->self = deviceList;
    backend->close = AwWiaDeviceList_Close_;
    backend->refreshList = AwWiaDeviceList_RefreshList_;
    backend->needsRefresh = AwWiaDeviceList_NeedsRefresh_;
    backend->releaseList = AwWiaDeviceList_ReleaseList_;
    backend->openDevice = AwWiaDeviceList_OpenDevice_;
    backend->closeDevice = AwWiaDeviceList_CloseDevice_;
    deviceList->logger = backend->logger;
    deviceList->allocator = backend->allocator;
    return AwWiaDeviceList_Open(deviceList);
}

// Function to check if process has admin rights
static BOOL IsElevated() {
    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(hToken, TokenElevation, &elevation,
                                sizeof(elevation), &cbSize)) {
            fIsElevated = elevation.TokenIsElevated;
        }
    }

    if (hToken) {
        CloseHandle(hToken);
    }

    return fIsElevated;
}

b32 RunElevatedCommand(char* command) {
    // Initialize the shell execute info structure
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpVerb = "runas";  // This triggers the UAC prompt
    sei.lpFile = "cmd.exe";
    sei.lpParameters = command;
    sei.nShow = SW_HIDE;  // Hide the window
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;  // Prevent console window

    // Execute the command with elevation
    if (!ShellExecuteEx(&sei)) {
        DWORD error = GetLastError();
        MLogf("Failed to execute command. Error code: %lu", error);
        return FALSE;
    }

    // Wait for the process to complete
    if (sei.hProcess != NULL) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        return FALSE;
    }
    return TRUE;
}

void PTPWiaServiceRestart() {
    // Prepare the command to restart the service
    char command[512];
    char* serviceName = "StiSvc";
    snprintf(command, sizeof(command), "/c sc stop %s && sc start %s ", serviceName, serviceName);
    RunElevatedCommand(command);
}
