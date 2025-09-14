#include "ptp-backend-wia.h"

#include <windows.h>
#include <wia.h>
#include <stdio.h>

#include "win-utils.h"

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
                MLogf("String: %s\n", utf8Str.str);
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
    PTPWiaDeviceList* deviceList;
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
            PTPDeviceWia* device = self->deviceList->openDevices + i;
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
static HRESULT CreateEventCallback(PTPWiaDeviceList* self, IWiaEventCallback** ppCallback) {
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

static HRESULT RegisterForDeviceConnectDisconnectEvents(PTPWiaDeviceList* self) {
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

b32 PTPWiaDeviceList_Open(PTPWiaDeviceList* self) {
    HRESULT hr = 0;

    // Create WIA Device Manager instance
    hr = CoCreateInstance(&CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,&IID_IWiaDevMgr,
        (void**)&self->deviceMgr);

    RegisterForDeviceConnectDisconnectEvents(self);

    return SUCCEEDED(hr);
}

void PTPWiaDeviceList_ReleaseList(PTPWiaDeviceList* self) {
    if (self->devices) {
        MArrayEachPtr(self->devices, it) {
            if (it.p->deviceId) {
                SysFreeString(it.p->deviceId); it.p->deviceId = NULL;
            }
        }
        MArrayInit(self->allocator, self->devices, 0);
    }
}

b32 PTPWiaDeviceList_Close(PTPWiaDeviceList* self) {
    PTPWiaDeviceList_ReleaseList(self);
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
    return TRUE;
}

b32 PTPWiaDeviceList_NeedsRefresh(PTPWiaDeviceList* self) {
    return (self->deviceListUpToDate == FALSE);
}

b32 PTPWiaDeviceList_RefreshList(PTPWiaDeviceList* self, PTPDeviceInfo** deviceList) {
    PTPWiaDeviceList_ReleaseList(self);

    IEnumWIA_DEV_INFO* pEnum = NULL;
    HRESULT hr = self->deviceMgr->lpVtbl->EnumDeviceInfo(self->deviceMgr, WIA_DEVINFO_ENUM_LOCAL, &pEnum);
    if (FAILED(hr)) {
        goto cleanup;
    }

    ULONG countElements = 0;
    hr = pEnum->lpVtbl->GetCount(pEnum, &countElements);

#define PROP_NUM 3
    PROPSPEC propSpec[PROP_NUM];
    PROPVARIANT propVar[PROP_NUM];
    propSpec[0].ulKind = PRSPEC_PROPID;
    propSpec[0].propid = WIA_DIP_DEV_ID;
    propSpec[1].ulKind = PRSPEC_PROPID;
    propSpec[1].propid = WIA_DIP_VEND_DESC;
    propSpec[2].ulKind = PRSPEC_PROPID;
    propSpec[2].propid = WIA_DIP_DEV_NAME;
    // propSpec[3].ulKind = PRSPEC_PROPID;
    // propSpec[3].propid = WIA_DIP_REMOTE_DEV_ID;

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
        PTPDeviceInfo* deviceInfo = MArrayAddPtrZ(self->allocator, *deviceList);
        wiaDeviceInfo->deviceId = SysAllocString(propVar[0].bstrVal);
        deviceInfo->manufacturer = WinUtils_BSTRToUTF8(self->allocator, propVar[1].bstrVal);
        deviceInfo->product = WinUtils_BSTRToUTF8(self->allocator, propVar[2].bstrVal);
        // PTP_INFO_F("Remote device id: %s", WinUtils_BSTRToUTF8(self->allocator, propVar[3].bstrVal));
        deviceInfo->backendType = PTP_ENABLE_WIA;
        deviceInfo->device = wiaDeviceInfo;
        PTP_INFO_F("Found device: %s (%s)", deviceInfo->product.str, deviceInfo->manufacturer.str);

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
        return TRUE;
    } else {
        return FALSE;
    }
}

void* PTPDeviceWia_ReallocBuffers(void* self, PTPBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    size_t headerSize = 0;
    if (type == PTP_BUFFER_IN) {
        headerSize = sizeof(WiaPtpRequest);
    } else if (type == PTP_BUFFER_OUT) {
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

void PTPDeviceWia_FreeBuffers(void* self, PTPBufferType type, void* dataMem, size_t dataSize) {
    size_t headerSize = 0;
    if (type == PTP_BUFFER_IN) {
        headerSize = sizeof(WiaPtpRequest);
    } else if (type == PTP_BUFFER_OUT) {
        headerSize = sizeof(WiaPtpResponse);
    }

    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        CoTaskMemFree(mem); dataMem = NULL;
    }
}

PTPResult PTPDeviceWia_SendAndRecvEx(void* self, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                     PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
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

    PTPDeviceWia* wia = ((PTPDevice*)self)->device;
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
        return PTP_OK;
    } else {
        return PTP_GENERAL_ERROR;
    }
}

b32 PTPWiaDeviceList_ConnectDevice(PTPWiaDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
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
    PTPDeviceWia* wiaDevice = MArrayAddPtr(self->allocator, self->openDevices);
    wiaDevice->device = pWiaItemExtras;
    wiaDevice->deviceId = wiaDeviceInfo->deviceId;
    wiaDevice->disconnected = FALSE;
    wiaDevice->logger = self->logger;

    (*deviceOut)->transport.reallocBuffer = PTPDeviceWia_ReallocBuffers;
    (*deviceOut)->transport.freeBuffer = PTPDeviceWia_FreeBuffers;
    (*deviceOut)->transport.sendAndRecvEx = PTPDeviceWia_SendAndRecvEx;
    (*deviceOut)->transport.requiresSessionOpenClose = FALSE;
    (*deviceOut)->device = wiaDevice;
    (*deviceOut)->backendType = PTP_BACKEND_WIA;
    (*deviceOut)->disconnected = FALSE;
    (*deviceOut)->logger = self->logger;

    pWiaItemExtras = NULL; // Prevent release in cleanup, ownership is now with PTPDeviceWia

cleanup:
    if (pWiaItemRoot) {
         pWiaItemRoot->lpVtbl->Release(pWiaItemRoot);
    }

    if (pWiaItemExtras) {
        pWiaItemExtras->lpVtbl->Release(pWiaItemExtras);
    }

    return SUCCEEDED(hr);
}

b32 PTPWiaDeviceList_DisconnectDevice(PTPWiaDeviceList* self, PTPDevice* device) {
    PTPDeviceWia* deviceWia = device->device;
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

    return TRUE;
}

b32 PTPWiaDeviceList_Reset(PTPWiaDeviceList* self, PTPDeviceWia* device) {
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

    return SUCCEEDED(hr);
}

b32 PTPWiaDeviceList_Close_(PTPBackend* backend) {
    PTPWiaDeviceList* self = backend->self;
    b32 r = PTPWiaDeviceList_Close(self);
    MFree(self->allocator, self, sizeof(PTPWiaDeviceList));
    return r;
}

b32 PTPWiaDeviceList_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPWiaDeviceList* self = backend->self;
    return PTPWiaDeviceList_RefreshList(self, deviceList);
}

b32 PTPWiaDeviceList_NeedsRefresh_(PTPBackend* backend) {
    PTPWiaDeviceList* self = backend->self;
    return PTPWiaDeviceList_NeedsRefresh(self);
}

void PTPWiaDeviceList_ReleaseList_(PTPBackend* backend) {
    PTPWiaDeviceList* self = backend->self;
    PTPWiaDeviceList_ReleaseList(self);
}

b32 PTPWiaDeviceList_ConnectDevice_(PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPWiaDeviceList* self = backend->self;
    return PTPWiaDeviceList_ConnectDevice(self, deviceInfo, deviceOut);
}

b32 PTPWiaDeviceList_DisconnectDevice_(PTPBackend* backend, PTPDevice* device) {
    PTPWiaDeviceList* self = backend->self;
    return PTPWiaDeviceList_DisconnectDevice(self, device);
}

b32 PTPWiaDeviceList_OpenBackend(PTPBackend* backend) {
    PTPWiaDeviceList* deviceList = MMallocZ(backend->allocator, sizeof(PTPWiaDeviceList));
    backend->self = deviceList;
    backend->close = PTPWiaDeviceList_Close_;
    backend->refreshList = PTPWiaDeviceList_RefreshList_;
    backend->needsRefresh = PTPWiaDeviceList_NeedsRefresh_;
    backend->releaseList = PTPWiaDeviceList_ReleaseList_;
    backend->openDevice = PTPWiaDeviceList_ConnectDevice_;
    backend->closeDevice = PTPWiaDeviceList_DisconnectDevice_;
    deviceList->logger = backend->logger;
    deviceList->allocator = backend->allocator;
    return PTPWiaDeviceList_Open(deviceList);
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
