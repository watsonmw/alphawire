#include <windows.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#include <libusbk.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <stdlib.h>

#include "mlib/mlib.h"

#include "ptp-backend-libusbk.h"

#define SONY_VID 0x054C
#define USB_EN_US 0x0409
#define USB_PROTOCOL_PTP 0x01
#define USB_CLASS_STILL_IMAGE 0x06
#define USB_SUBCLASS_STILL_IMAGE 0x01
#define BULK_MAX_PACKET_SIZE 512

// PTP Container Types
#define PTP_CONTAINER_COMMAND  0x0001
#define PTP_CONTAINER_DATA     0x0002
#define PTP_CONTAINER_RESPONSE 0x0003
#define PTP_CONTAINER_EVENT    0x0004

typedef MSTRUCTPACKED(struct {
    u32 length;
    u16 type;
    u16 code;
    u32 transactionId;
}) PtpContainerHeader;

b32 PTPUsbkDeviceList_Open(PTPUsbkDeviceList* self) {
    return FALSE;
}

b32 PTPUsbkDeviceList_Close(PTPUsbkDeviceList* self) {
    PTPUsbkDeviceList_ReleaseList(self);
    if (self->openDevices) {
        MArrayFree(self->openDevices);
    }
    return TRUE;
}

// Check configuration descriptors for PTP support
b32 CheckDeviceHasPtpEndPoints(KUSB_HANDLE usbHandle) {
    // Get the len of the configuration descriptor
    USB_CONFIGURATION_DESCRIPTOR configDesc;
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0,
                            0,
                            (PUCHAR)&configDesc,
                            sizeof(USB_CONFIGURATION_DESCRIPTOR),
                            NULL)) {
        MLogf("Error getting configuration descriptor");
        return FALSE;
    }

    // Allocate buffer for the entire configuration descriptor
    UCHAR* configBuffer = NULL;
    UINT descriptorLength = configDesc.wTotalLength;
    configBuffer = (UCHAR*)MMalloc(descriptorLength);
    if (!configBuffer) {
        MLogf("Memory allocation failed");
        return FALSE;
    }

    // Get the full configuration descriptor
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0,
                            0,
                            configBuffer,
                            descriptorLength,
                            NULL)) {
        MLogf("Error getting full configuration descriptor");
        free(configBuffer);
        return FALSE;
    }

    // Parse the configuration descriptor
    UCHAR* ptr = configBuffer;
    UCHAR* end = configBuffer + descriptorLength;

    b32 hasPTP = FALSE;
    while (ptr < end) {
        USB_COMMON_DESCRIPTOR* common = (USB_COMMON_DESCRIPTOR*)ptr;
        if (common->bDescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) {
            USB_INTERFACE_DESCRIPTOR *iface = (USB_INTERFACE_DESCRIPTOR *) ptr;

            // Check if this interface supports PTP
            if (iface->bInterfaceClass == USB_CLASS_STILL_IMAGE &&
                iface->bInterfaceSubClass == USB_SUBCLASS_STILL_IMAGE &&
                iface->bInterfaceProtocol == USB_PROTOCOL_PTP) {
                hasPTP = TRUE;
                break;
            }
        }

        ptr += common->bLength;
    }

    MFree(configBuffer, descriptorLength);
    return hasPTP;
}


void PTPUsbkDevice_ReadEvent(PTPDevice* device) {
    PTPDeviceUsbk* deviceUsbk = device->device;
    UCHAR eventBuffer[512];
    UINT transferred = 0;

    if (UsbK_ReadPipe(deviceUsbk->usbHandle, deviceUsbk->usbInterrupt, eventBuffer, sizeof(eventBuffer), &transferred, NULL)) {
        PtpContainerHeader* event = (PtpContainerHeader*)eventBuffer;
        if (event->type == PTP_CONTAINER_EVENT) {
            MLogf("PTP Event received:");
            MLogf("Length: %d", event->length);
            MLogf("Type: 0x%04X", event->type);
            MLogf("Event Code: 0x%04X", event->code);
            MLogf("Transaction: 0x%08X", event->transactionId);
        }
    }
}

int PTPUsbkDevice_ReadEvent2(KUSB_HANDLE usbHandle, UCHAR interrupt_ep) {
    PtpContainerHeader event;
    UINT32 transferred = 0;
    OVERLAPPED overlapped = {0};
    BOOL hasEvent = FALSE;

    // Create event for async operation
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
        MLogf("Failed to create event");
        return 0;
    }

    // Start async read from interrupt endpoint
    if (!UsbK_ReadPipe(usbHandle, interrupt_ep, (PUCHAR)&event,
                       sizeof(PtpContainerHeader), &transferred, &overlapped)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            CloseHandle(overlapped.hEvent);
            MLogf("Failed to start interrupt read: %d", GetLastError());
            return 0;
        }

        // Wait for data or timeout (100ms in this example)
        DWORD result = WaitForSingleObject(overlapped.hEvent, 100);

        if (result == WAIT_OBJECT_0) {
            // Get results
            if (UsbK_GetOverlappedResult(usbHandle, &overlapped, &transferred, FALSE)) {
                hasEvent = TRUE;
            }
        } else if (result == WAIT_TIMEOUT) {
            // No event available, cancel the transfer
            UsbK_AbortPipe(usbHandle, interrupt_ep);
        }
    } else {
       // Immediate completion
       hasEvent = TRUE;
    }

    CloseHandle(overlapped.hEvent);

    if (hasEvent && transferred >= sizeof(PtpContainerHeader)) {
        if (event.type == PTP_CONTAINER_EVENT) {
            MLogf("PTP Event received:");
            MLogf("Length: %d", event.length);
            MLogf("Type: 0x%04X", event.type);
            MLogf("Event Code: 0x%04X", event.code);
            MLogf("Transaction: 0x%08X", event.transactionId);
            return 1;
        }
    }

    return 0;
}

b32 PTPUsbkDeviceList_RefreshList(PTPUsbkDeviceList* self, PTPDeviceInfo** devices) {
    KLST_HANDLE deviceList = NULL;
    KLST_DEVINFO_HANDLE deviceInfo = NULL;
    KUSB_HANDLE usbHandle = NULL;

    // Initialize the device list
    if (!LstK_Init(&deviceList, 0)) {
        MLogf("Failed to initialize device list.");
        return FALSE;
    }

    self->deviceList = deviceList;

    // Iterate through the device list
    while (LstK_MoveNext(deviceList, &deviceInfo)) {
        // Skip if not Sony
        if (deviceInfo->Common.Vid != SONY_VID) {
            MLogf("Skipping non Sony device");
            continue;
        }

        // Connect to the device and check its descriptors to see if it is a PTP device
        if (UsbK_Init(&usbHandle, deviceInfo)) {
            if (CheckDeviceHasPtpEndPoints(usbHandle)) {
                USB_DEVICE_DESCRIPTOR deviceDescriptor;

                // Get the device descriptor
                if (!UsbK_GetDescriptor(usbHandle,
                                        USB_DESCRIPTOR_TYPE_DEVICE,
                                        0,
                                        0,
                                        (PUCHAR)&deviceDescriptor,
                                        sizeof(USB_DEVICE_DESCRIPTOR),
                                        NULL)) {
                    MLogf("Error getting device descriptor");
                    UsbK_Free(usbHandle);
                    LstK_Free(deviceList);
                    return FALSE;
                }

                UsbkDeviceInfo *usbkDevice = MArrayAddPtr(self->devices);
                PTPDeviceInfo* device = MArrayAddPtr(*devices);
                usbkDevice->deviceId = deviceInfo;
                device->manufacturer = MStrMake(deviceInfo->Mfg);
                device->backendType = PTP_BACKEND_LIBUSBK;
                device->device = usbkDevice;

                char productString[256];
                if (deviceDescriptor.iProduct > 0) {  // Check if product string exists
                    UINT transferLength = 0;
                    UCHAR stringDescriptor[256];
                    if (UsbK_GetDescriptor(usbHandle,
                                           USB_DESCRIPTOR_TYPE_STRING,
                                           deviceDescriptor.iProduct,  // Index from device descriptor
                                           USB_EN_US,
                                           stringDescriptor,
                                           sizeof(stringDescriptor),
                                           &transferLength) && transferLength >= 2) {

                        // Convert the USB string descriptor to a printable string, USB string descriptors are UTF-16LE
                        // First byte is the length, second byte is the descriptor type,
                        // actual string starts at offset 2
                        WCHAR* wideString = (WCHAR*)(&stringDescriptor[2]);
                        int stringLength = (transferLength - 2) / 2;  // Convert bytes to WCHAR count
                        wideString[stringLength] = 0;  // Null terminate

                        // Convert to narrow string for printing
                        WideCharToMultiByte(CP_UTF8, 0, wideString, -1,
                                            productString, sizeof(productString),
                                            NULL, NULL);

                        device->deviceName = MStrMake(productString);
                    } else {
                        MLogf("Error getting product string descriptor");
                    }
                }
            }

            // Close the USB device
            UsbK_Free(usbHandle);
        } else {
            MLogf("Failed to open device.");
        }
    }

    return TRUE;
}

void PTPUsbkDeviceList_ReleaseList(PTPUsbkDeviceList* self) {
    // Free the device list
    if (self->deviceList) {
        LstK_Free((KLST_HANDLE)self->deviceList);
        self->deviceList = NULL;
    }

    if (self->devices) {
        for (int i = 0; i < MArraySize(self->devices); i++) {
            UsbkDeviceInfo* deviceInfo = self->devices + i;
            deviceInfo->deviceId = NULL;
        }
        MArrayFree(self->devices);
    }
}

void* PTPDeviceUsbk_ReallocBuffer(void* self, PtpBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    size_t headerSize = sizeof(PtpContainerHeader);

    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(mem, dataOldSize + headerSize); dataMem = NULL;
    }
    dataMem = MMalloc(dataSize);
    memset(dataMem, 0, dataSize);
    return ((u8*)dataMem) + headerSize;
}

void PTPDeviceUsbk_FreeBuffer(void* self, PtpBufferType type, void* dataMem, size_t dataOldSize) {
    size_t headerSize = sizeof(PtpContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(mem, dataSize); dataMem = NULL;
    }
}

PTPResult PTPDeviceUsbk_SendAndRecv(void* deviceSelf, PtpRequest* request, u8* dataIn, size_t dataInSize,
                                    PtpResponse* response, u8* dataOut, size_t dataOutSize,
                                    size_t* actualDataOutSize) {

    PTPDeviceUsbk* usbk = (PTPDeviceUsbk*)((PTPDevice*)deviceSelf)->device;
    KUSB_HANDLE usbHandle = usbk->usbHandle;

    size_t requestSize = sizeof(PtpContainerHeader) + (request->NumParams * sizeof(u32));
    void* requestHeader = alloca(requestSize);

    PtpContainerHeader* requestData = (PtpContainerHeader*)requestHeader;
    requestData->length = requestSize;
    requestData->type = PTP_CONTAINER_COMMAND;
    requestData->code = request->OpCode;
    requestData->transactionId = request->TransactionId;
    u32* params = (u32*)(requestData+1);
    for (int i = 0; i < request->NumParams; i++) {
        params[i] = request->Params[i];
    }

    // 1. Send request header packet (with params)
    u32 transferred = 0;
    BOOL b = UsbK_WritePipe(usbHandle, usbk->usbBulkOut, (PUCHAR)requestData, requestSize,
                            &transferred, NULL);
    if (!b) {
        MLogf("Failed to send PTP request: %d", GetLastError());
        return PTP_GENERAL_ERROR;
    }

    // 2. Write additional data, if provided
    if (dataInSize) {
        requestSize = sizeof(PtpContainerHeader) +  dataInSize;
        requestData = (PtpContainerHeader*)(((u8*)dataIn) - sizeof(PtpContainerHeader));
        requestData->length = requestSize;
        requestData->type = PTP_CONTAINER_DATA;
        requestData->code = request->OpCode;
        requestData->transactionId = request->TransactionId;

        b = UsbK_WritePipe(usbHandle, usbk->usbBulkOut, (PUCHAR)requestData, requestSize,
                           &transferred, NULL);
        if (!b) {
            MLogf("Failed to send PTP request: %d", GetLastError());
            return PTP_GENERAL_ERROR;
        }
    }

    // 3. Read data packets, if necessary
    u32 actual = 0;
    PtpContainerHeader* responseHeader = (PtpContainerHeader*)(((u8*)dataOut) - sizeof(PtpContainerHeader));
    if (!UsbK_ReadPipe(usbHandle, usbk->usbBulkIn, (PUCHAR)responseHeader,
                   sizeof(PtpContainerHeader) + dataOutSize, &actual, NULL)) {
        MLogf("Failed to read PTP response: %d", GetLastError());
        return PTP_GENERAL_ERROR;
    }

    u32 dataBytesTransferred = 0;
    if (responseHeader->type == PTP_CONTAINER_DATA) {
        u32 payloadLength = responseHeader->length;
        unsigned char* cp = ((u8*)responseHeader) + actual;

        while (actual < payloadLength) {
            u32 dataTransfer = 0;
            if (!UsbK_ReadPipe(usbHandle, usbk->usbBulkIn, (PUCHAR)cp,
                               payloadLength - actual, &dataTransfer, NULL)) {
                MLogf("Failed to read PTP response: %d", GetLastError());
                return PTP_GENERAL_ERROR;
                               }
            actual += dataTransfer;
            cp += dataTransfer;
        }
        dataBytesTransferred = (actual > sizeof(PtpContainerHeader)) ? actual - sizeof(PtpContainerHeader) : 0;

        size_t responseSize = sizeof(PtpContainerHeader) + (PTP_MAX_PARAMS * sizeof(u32));
        responseHeader = alloca(responseSize);

        if (!UsbK_ReadPipe(usbHandle, usbk->usbBulkIn, (PUCHAR)responseHeader,
                           responseSize, &actual, NULL)) {
            MLogf("Failed to read PTP response: %d", GetLastError());
            return PTP_GENERAL_ERROR;
        }
    }

    *actualDataOutSize = dataBytesTransferred;

    // 4. Read response packet params
    response->ResponseCode = responseHeader->code;
    response->TransactionId = responseHeader->transactionId;
    MMemIO memIo;
    int paramsSize = responseHeader->length - sizeof(PtpContainerHeader);
    MMemInit(&memIo, ((u8*)responseHeader) + sizeof(PtpContainerHeader), paramsSize);
    response->NumParams = paramsSize / sizeof(u32);
    for (int i = 0; i < response->NumParams; i++) {
        MMemReadU32BE(&memIo, response->Params + i);
    }

    return PTP_OK;
}

b32 FindBulkInOutEndpoints(KUSB_HANDLE usbHandle, UCHAR* bulkIn, UCHAR* bulkOut, UCHAR* interruptOut) {
    USB_CONFIGURATION_DESCRIPTOR configDesc = {};

    // Get configuration descriptor size
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0, 0,
                            (PUCHAR)&configDesc,
                            sizeof(USB_CONFIGURATION_DESCRIPTOR),
                            NULL)) {
        return FALSE;
    }

    // Allocate buffer for full descriptor
    UCHAR* buffer = (UCHAR*)MMalloc(configDesc.wTotalLength);

    // Get full configuration descriptor
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0, 0,
                            buffer,
                            configDesc.wTotalLength,
                            NULL)) {
        MFree(buffer, configDesc.wTotalLength);
        return FALSE;
    }

    // Parse descriptor for endpoints
    UCHAR* ptr = buffer;
    UCHAR* end = buffer + configDesc.wTotalLength;

    b32 found = FALSE;
    while (ptr < end) {
        USB_COMMON_DESCRIPTOR* common = (USB_COMMON_DESCRIPTOR*)ptr;

        if (common->bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT) {
            USB_ENDPOINT_DESCRIPTOR* endPoint = (USB_ENDPOINT_DESCRIPTOR*)ptr;
            UCHAR endPointType = endPoint->bmAttributes & USB_ENDPOINT_TYPE_MASK;

            // Check if it's a bulk endpoint
            if ((endPoint->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) {
                if (endPoint->bEndpointAddress & USB_ENDPOINT_DIRECTION_MASK) {
                    *bulkIn = endPoint->bEndpointAddress;
                } else {
                    *bulkOut = endPoint->bEndpointAddress;
                }

            }  else if (endPointType == USB_ENDPOINT_TYPE_INTERRUPT) {
                *interruptOut = endPoint->bEndpointAddress;
            }

            if (*bulkIn && *bulkOut && *interruptOut) {
                found = TRUE;
                break;
            }
        }
        ptr += common->bLength;
    }

    MFree(buffer, configDesc.wTotalLength);
    return found;
}

b32 PTPUsbkDeviceList_ConnectDevice(PTPUsbkDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    UsbkDeviceInfo* device = deviceInfo->device;
    KLST_DEVINFO_HANDLE deviceInfoHandle = (KLST_DEVINFO_HANDLE)device->deviceId;
    KUSB_HANDLE usbHandle = NULL;

    if (UsbK_Init(&usbHandle, deviceInfoHandle)) {
        // Find bulk endpoints
        UCHAR bulkIn = 0, bulkOut = 0, interruptOut = 0;
        if (!FindBulkInOutEndpoints(usbHandle, &bulkIn, &bulkOut, &interruptOut)) {
            UsbK_Free(usbHandle);
            return FALSE;
        }

        // Store the interface pointer
        PTPDeviceUsbk *usbkDevice = MArrayAddPtr(self->openDevices);
        usbkDevice->deviceId = device->deviceId;
        usbkDevice->usbHandle = usbHandle;
        usbkDevice->usbBulkIn = bulkIn;
        usbkDevice->usbBulkOut = bulkOut;
        usbkDevice->usbInterrupt = interruptOut;
        usbkDevice->disconnected = FALSE;

        (*deviceOut)->transport.reallocBuffer = PTPDeviceUsbk_ReallocBuffer;
        (*deviceOut)->transport.freeBuffer = PTPDeviceUsbk_FreeBuffer;
        (*deviceOut)->transport.sendAndRecvEx = PTPDeviceUsbk_SendAndRecv;
        (*deviceOut)->transport.requiresSessionOpenClose = TRUE;
        (*deviceOut)->device = usbkDevice;
        (*deviceOut)->backendType = PTP_BACKEND_LIBUSBK;
        (*deviceOut)->disconnected = FALSE;

        return TRUE;
    } else {
        return FALSE;
    }
}

b32 PTPUsbkDeviceList_DisconnectDevice(PTPUsbkDeviceList* self, PTPDevice* device) {
    PTPDeviceUsbk* deviceUsbk = device->device;
    if (deviceUsbk->usbHandle) {
        UsbK_Free(deviceUsbk->usbHandle);
        deviceUsbk->usbHandle = NULL;
    }

    MArrayEachPtr(self->openDevices, it) {
        if (it.p == deviceUsbk) {
            MArrayRemoveIndex(self->openDevices, it.i);
            break;
        }
    }

    return TRUE;
}

b32 PTPUsbkDeviceList_Close_(PTPBackend* backend) {
    PTPUsbkDeviceList* self = backend->self;
    b32 r = PTPUsbkDeviceList_Close(self);
    MFree(self, sizeof(PTPUsbkDeviceList));
    return r;
}

b32 PTPUsbkDeviceList_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPUsbkDeviceList* self = backend->self;
    return PTPUsbkDeviceList_RefreshList(self, deviceList);
}

void PTPUsbkDeviceList_ReleaseList_(PTPBackend* backend) {
    PTPUsbkDeviceList* self = backend->self;
    PTPUsbkDeviceList_ReleaseList(self);
}

b32 PTPUsbkDeviceList_ConnectDevice_(PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPUsbkDeviceList* self = backend->self;
    return PTPUsbkDeviceList_ConnectDevice(self, deviceInfo, deviceOut);
}

b32 PTPUsbkDeviceList_DisconnectDevice_(PTPBackend* backend, PTPDevice* device) {
    PTPUsbkDeviceList* self = backend->self;
    return PTPUsbkDeviceList_DisconnectDevice(self, device);
}

b32 PTPUsbkDeviceList_OpenBackend(PTPBackend* backend) {
    PTPUsbkDeviceList* deviceList = MMallocZ(sizeof(PTPUsbkDeviceList));
    backend->self = deviceList;
    backend->close = PTPUsbkDeviceList_Close_;
    backend->refreshList = PTPUsbkDeviceList_RefreshList_;
    backend->releaseList = PTPUsbkDeviceList_ReleaseList_;
    backend->openDevice = PTPUsbkDeviceList_ConnectDevice_;
    backend->closeDevice = PTPUsbkDeviceList_DisconnectDevice_;
    return PTPUsbkDeviceList_Open(deviceList);
}
