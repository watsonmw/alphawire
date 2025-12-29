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
#ifdef _MSC_VER
#include <malloc.h>
#endif

#include "mlib/mlib.h"
#include "ptp/ptp-control.h"

#include "platform/usb-const.h"
#include "platform/windows/ptp-backend-libusbk.h"
#include "platform/windows/win-utils.h"

b32 PTPUsbkDeviceList_Open(PTPUsbkDeviceList* self) {
    PTP_TRACE("PTPUsbkDeviceList_Open");
    return TRUE;
}

b32 PTPUsbkDeviceList_Close(PTPUsbkDeviceList* self) {
    PTP_TRACE("PTPUsbkDeviceList_Close");
    PTPUsbkDeviceList_ReleaseList(self);
    if (self->openDevices) {
        MArrayFree(self->allocator, self->openDevices);
    }
    return TRUE;
}

// Check configuration descriptors for PTP support
static b32 CheckDeviceHasPtpEndPoints(PTPUsbkDeviceList* self, KUSB_HANDLE usbHandle, u32 timeoutMilliseconds) {
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
        PTP_ERROR("Failed to create event for overlapped I/O");
        return FALSE;
    }

    // Get the len of the configuration descriptor
    USB_CONFIGURATION_DESCRIPTOR configDesc;
    UINT transferred = 0;
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0, 0,
                            (PUCHAR) &configDesc,
                            sizeof(USB_CONFIGURATION_DESCRIPTOR),
                            &transferred)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&self->logger, "Error getting configuration descriptor");
            return FALSE;
        }

        if (WaitForSingleObject(overlapped.hEvent, timeoutMilliseconds) != WAIT_OBJECT_0 ||
            !UsbK_GetOverlappedResult(usbHandle, &overlapped, &transferred, FALSE)) {
            CloseHandle(overlapped.hEvent);
            PTP_ERROR("Overlapped I/O failed for configuration descriptor");
            return FALSE;
        }
    }

    // Allocate buffer for the entire configuration descriptor
    UCHAR *configBuffer = NULL;
    UINT descriptorLength = configDesc.wTotalLength;
    configBuffer = (UCHAR *) MMalloc(self->allocator, descriptorLength);
    if (!configBuffer) {
        CloseHandle(overlapped.hEvent);
        PTP_ERROR("Memory allocation failed");
        return FALSE;
    }

    // Reset event for next operation
    ResetEvent(overlapped.hEvent);

    // Get the full configuration descriptor
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0, 0,
                            configBuffer,
                            descriptorLength,
                            &transferred)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            MFree(self->allocator, configBuffer, descriptorLength);
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&self->logger, "Error getting full configuration descriptor");
            return FALSE;
        }

        if (WaitForSingleObject(overlapped.hEvent, timeoutMilliseconds) != WAIT_OBJECT_0 ||
            !UsbK_GetOverlappedResult(usbHandle, &overlapped, &transferred, FALSE)) {
            MFree(self->allocator, configBuffer, descriptorLength);
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&self->logger, "Overlapped I/O failed for full configuration");
            return FALSE;
        }
    }

    // Parse the configuration descriptor
    UCHAR *ptr = configBuffer;
    UCHAR *end = configBuffer + descriptorLength;

    b32 hasPTP = FALSE;
    while (ptr < end) {
        USB_COMMON_DESCRIPTOR *common = (USB_COMMON_DESCRIPTOR *) ptr;
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

    CloseHandle(overlapped.hEvent);
    MFree(self->allocator, configBuffer, descriptorLength);
    return hasPTP;
}

b32 PTPUsbkDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds) {
    PTPDeviceUsbk* deviceUsbk = device->device;
    UCHAR eventBuffer[512];
    UINT transferred = 0;

    OVERLAPPED overlapped = {0};
    BOOL hasEvent = FALSE;
    if (outEvent == NULL) {
        return FALSE;
    }

    // Create event for async operation
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
        WinUtils_LogLastError(&deviceUsbk->logger, "Failed to create event");
        return 0;
    }

    if (!UsbK_ReadPipe(deviceUsbk->usbHandle, deviceUsbk->usbInterrupt, eventBuffer, sizeof(eventBuffer),
                       &transferred, &overlapped)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&deviceUsbk->logger, "Failed to start interrupt read");
            return 0;
        }

        // Wait for data or timeout
        DWORD result = WaitForSingleObject(overlapped.hEvent, timeoutMilliseconds);
        if (result == WAIT_OBJECT_0) {
            // Get results
            if (UsbK_GetOverlappedResult(deviceUsbk->usbHandle, &overlapped, &transferred, FALSE)) {
                hasEvent = TRUE;
            }
        } else if (result == WAIT_TIMEOUT) {
            PTP_LOG_ERROR(&deviceUsbk->logger, "Timeout waiting for event");
            // No event available, cancel the transfer
            UsbK_AbortPipe(deviceUsbk->usbHandle, deviceUsbk->usbInterrupt);
        }
    } else {
       // Immediate completion
       hasEvent = TRUE;
    }

    CloseHandle(overlapped.hEvent);

    if (hasEvent && transferred >= sizeof(PTPContainerHeader)) {
        PTPContainerHeader* event = (PTPContainerHeader*)eventBuffer;
        if (event->type == PTP_CONTAINER_EVENT) {
            PTP_LOG_INFO(&deviceUsbk->logger, "PTP Event received:");
            PTP_LOG_INFO_F(&deviceUsbk->logger, "Length: %d", event->length);
            PTP_LOG_INFO_F(&deviceUsbk->logger, "Transferred: %d", transferred);
            PTP_LOG_INFO_F(&deviceUsbk->logger, "Type: 0x%04X", event->type);
            PTP_LOG_INFO_F(&deviceUsbk->logger, "Event Code: %s (0x%04X)", PTP_GetEventStr(event->code), event->code);
            PTP_LOG_INFO_F(&deviceUsbk->logger, "Transaction: 0x%08X", event->transactionId);
            size_t headerSize = sizeof(PTPContainerHeader);
            if (event->length > headerSize) {
                size_t payloadSize = event->length - headerSize;
                MMemIO memIO;
                MMemInitRead(&memIO, eventBuffer + headerSize, payloadSize);
                memset(outEvent, 0, sizeof(PTPEvent));
                outEvent->code = event->code;
                MMemReadU32BE(&memIO, &outEvent->param1);
                MMemReadU32BE(&memIO, &outEvent->param2);
                MMemReadU32BE(&memIO, &outEvent->param3);
                // MLogfNoNewLine("Payload: 0x");
                // MLogBytes(eventBuffer + headerSize, payloadSize);
            }
            return TRUE;
        }
    }

    return FALSE;
}

b32 PTPUsbkDeviceList_NeedsRefresh(PTPUsbkDeviceList* self) {
    PTP_TRACE("PTPUsbkDeviceList_NeedsRefresh");
    return FALSE;
}

b32 PTPUsbkDeviceList_RefreshList(PTPUsbkDeviceList* self, PTPDeviceInfo** devices) {
    PTP_TRACE("PTPUsbkDeviceList_RefreshList");

    KLST_HANDLE deviceList = NULL;
    KLST_DEVINFO_HANDLE deviceInfo = NULL;
    KUSB_HANDLE usbHandle = NULL;

    // Initialize the device list
    if (!LstK_Init(&deviceList, 0)) {
        PTP_ERROR("Failed to initialize device list.");
        return FALSE;
    }

    self->deviceList = deviceList;

    // Iterate through the device list
    while (LstK_MoveNext(deviceList, &deviceInfo)) {
        // Skip if not Sony
        if (deviceInfo->Common.Vid != USB_SONY_VID) {
            PTP_ERROR("Skipping non Sony device");
            continue;
        }

        // Connect to the device and check its descriptors to see if it is a PTP device
        if (UsbK_Init(&usbHandle, deviceInfo)) {
            if (CheckDeviceHasPtpEndPoints(self, usbHandle, self->timeoutMilliseconds)) {
                USB_DEVICE_DESCRIPTOR deviceDescriptor;

                // Get the device descriptor
                if (!UsbK_GetDescriptor(usbHandle, USB_DESCRIPTOR_TYPE_DEVICE, 0, 0,
                        (PUCHAR)&deviceDescriptor,sizeof(USB_DEVICE_DESCRIPTOR), NULL)) {
                    PTP_ERROR("Error getting device descriptor");
                    UsbK_Free(usbHandle);
                    LstK_Free(deviceList);
                    return FALSE;
                }

                if (deviceDescriptor.iProduct > 0) {  // Check if product string exists
                    UINT transferLength = 0;
                    UCHAR stringDescriptor[256] = {};
                    if (UsbK_GetDescriptor(usbHandle,
                                           USB_DESCRIPTOR_TYPE_STRING,
                                           deviceDescriptor.iProduct,  // Index from device descriptor
                                           USB_EN_US,
                                           stringDescriptor,
                                           sizeof(stringDescriptor),
                                           &transferLength) && transferLength >= 2) {

                        // Convert the USB string descriptor to a printable string, USB string descriptors are UTF-16LE
                        // First byte is the length, second byte is the descriptor type, actual string starts at offset 2
                        i32 length = *((u8*)&stringDescriptor);
                        WCHAR* wideString = (WCHAR*)(&stringDescriptor[2]);

                        UsbkDeviceInfo *usbkDevice = MArrayAddPtr(self->allocator, self->devices);
                        PTPDeviceInfo* device = MArrayAddPtrZ(self->allocator, *devices);
                        usbkDevice->deviceId = deviceInfo;
                        device->manufacturer = MStrMake(self->allocator, deviceInfo->Mfg);
                        device->backendType = PTP_BACKEND_LIBUSBK;
                        device->device = usbkDevice;
                        device->product = WinUtils_BSTRWithSizeToUTF8(self->allocator, wideString, length/2);
                        device->usbVID = deviceInfo->Common.Vid;
                        device->usbPID = deviceInfo->Common.Pid;
                        device->serial = MStrMake(self->allocator, deviceInfo->SerialNumber);
                        device->usbVersion = deviceDescriptor.bcdUSB;

                        PTP_INFO_F("Found device: %s (%s)", device->product.str, device->manufacturer.str);
                    } else {
                        PTP_ERROR("Error getting product string descriptor");
                    }
                }
            }

            // Close the USB device
            UsbK_Free(usbHandle);
        } else {
            PTP_ERROR("Failed to open device.");
        }
    }

    return TRUE;
}

void PTPUsbkDeviceList_ReleaseList(PTPUsbkDeviceList* self) {
    PTP_TRACE("PTPUsbkDeviceList_ReleaseList");

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
        MArrayFree(self->allocator, self->devices);
    }
}

void* PTPDeviceUsbk_ReallocBuffer(void* deviceSelf, PTPBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataOldSize + headerSize); dataMem = NULL;
    }
    dataMem = MMallocZ(self->transport.allocator, dataSize);
    return ((u8*)dataMem) + headerSize;
}

void PTPDeviceUsbk_FreeBuffer(void* deviceSelf, PTPBufferType type, void* dataMem, size_t dataOldSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataSize); dataMem = NULL;
    }
}

PTPResult PTPDeviceUsbk_SendAndRecv(void* deviceSelf, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                    PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                    size_t* actualDataOutSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    PTPDeviceUsbk* deviceUsbk = self->device;
    KUSB_HANDLE usbHandle = deviceUsbk->usbHandle;
    OVERLAPPED overlapped = {0};
    BOOL result;
    DWORD waitResult;

    // Create event for overlapped I/O
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
        WinUtils_LogLastError(&self->logger, "Failed to create event for overlapped I/O");
        return PTP_GENERAL_ERROR;
    }

    size_t requestSize = sizeof(PTPContainerHeader) + (request->NumParams * sizeof(u32));
    void* requestHeader = alloca(requestSize);

    PTPContainerHeader* requestData = (PTPContainerHeader*)requestHeader;
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
    result = UsbK_WritePipe(usbHandle, deviceUsbk->usbBulkOut, (PUCHAR)requestData, requestSize,
                            &transferred, &overlapped);
    if (!result && GetLastError() != ERROR_IO_PENDING) {
        CloseHandle(overlapped.hEvent);
        WinUtils_LogLastError(&deviceUsbk->logger, "Failed to send PTP request");
        return PTP_GENERAL_ERROR;
    }

    waitResult = WaitForSingleObject(overlapped.hEvent, deviceUsbk->timeoutMilliseconds);
    if (waitResult != WAIT_OBJECT_0) {
        UsbK_AbortPipe(usbHandle, deviceUsbk->usbBulkOut);
        CloseHandle(overlapped.hEvent);
        PTP_ERROR("Timeout or error while sending PTP request");
        return PTP_GENERAL_ERROR;
    }

    // 2. Write additional data, if provided
    if (dataInSize) {
        ResetEvent(overlapped.hEvent);
        requestSize = sizeof(PTPContainerHeader) + dataInSize;
        requestData = (PTPContainerHeader*)(((u8*)dataIn) - sizeof(PTPContainerHeader));
        requestData->length = requestSize;
        requestData->type = PTP_CONTAINER_DATA;
        requestData->code = request->OpCode;
        requestData->transactionId = request->TransactionId;

        result = UsbK_WritePipe(usbHandle, deviceUsbk->usbBulkOut, (PUCHAR)requestData, requestSize,
                                &transferred, &overlapped);
        if (!result && GetLastError() != ERROR_IO_PENDING) {
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&self->logger, "Failed to send PTP data");
            return PTP_GENERAL_ERROR;
        }

        waitResult = WaitForSingleObject(overlapped.hEvent, deviceUsbk->timeoutMilliseconds);
        if (waitResult != WAIT_OBJECT_0) {
            UsbK_AbortPipe(usbHandle, deviceUsbk->usbBulkOut);
            CloseHandle(overlapped.hEvent);
            PTP_ERROR("Timeout or error while sending PTP data");
            return PTP_GENERAL_ERROR;
        }
    }

    // 3. Read data packets
    ResetEvent(overlapped.hEvent);
    u32 actual = 0;
    PTPContainerHeader* responseHeader = (PTPContainerHeader*)(((u8*)dataOut) - sizeof(PTPContainerHeader));
    result = UsbK_ReadPipe(usbHandle, deviceUsbk->usbBulkIn, (PUCHAR)responseHeader,
                          sizeof(PTPContainerHeader) + dataOutSize, &actual, &overlapped);
    if (!result && GetLastError() != ERROR_IO_PENDING) {
        CloseHandle(overlapped.hEvent);
        WinUtils_LogLastError(&self->logger, "Failed to read PTP response");
        return PTP_GENERAL_ERROR;
    }

    waitResult = WaitForSingleObject(overlapped.hEvent, deviceUsbk->timeoutMilliseconds);
    if (waitResult != WAIT_OBJECT_0) {
        UsbK_AbortPipe(usbHandle, deviceUsbk->usbBulkIn);
        CloseHandle(overlapped.hEvent);
        PTP_ERROR("Timeout or error while reading PTP response");
        return PTP_GENERAL_ERROR;
    }

    if (!UsbK_GetOverlappedResult(usbHandle, &overlapped, &actual, FALSE)) {
        CloseHandle(overlapped.hEvent);
        WinUtils_LogLastError(&self->logger, "Failed to get overlapped result");
        return PTP_GENERAL_ERROR;
    }

    u32 dataBytesTransferred = 0;
    if (responseHeader->type == PTP_CONTAINER_DATA) {
        u32 payloadLength = responseHeader->length;
        unsigned char* cp = ((u8*)responseHeader) + actual;

        while (actual < payloadLength) {
            ResetEvent(overlapped.hEvent);
            u32 dataTransfer = 0;
            result = UsbK_ReadPipe(usbHandle, deviceUsbk->usbBulkIn, (PUCHAR)cp, payloadLength - actual,
                &dataTransfer, &overlapped);
            if (!result && GetLastError() != ERROR_IO_PENDING) {
                CloseHandle(overlapped.hEvent);
                WinUtils_LogLastError(&self->logger, "Failed to read PTP response data");
                return PTP_GENERAL_ERROR;
            }

            waitResult = WaitForSingleObject(overlapped.hEvent, deviceUsbk->timeoutMilliseconds);
            if (waitResult != WAIT_OBJECT_0) {
                UsbK_AbortPipe(usbHandle, deviceUsbk->usbBulkIn);
                CloseHandle(overlapped.hEvent);
                PTP_ERROR("Timeout or error while reading PTP response data");
                return PTP_GENERAL_ERROR;
            }

            if (!UsbK_GetOverlappedResult(usbHandle, &overlapped, &dataTransfer, FALSE)) {
                CloseHandle(overlapped.hEvent);
                WinUtils_LogLastError(&self->logger, "Failed to get overlapped result for data");
                return PTP_GENERAL_ERROR;
            }

            actual += dataTransfer;
            cp += dataTransfer;
        }
        dataBytesTransferred = (actual > sizeof(PTPContainerHeader)) ? actual - sizeof(PTPContainerHeader) : 0;

        // Read response packet
        ResetEvent(overlapped.hEvent);
        size_t responseSize = sizeof(PTPContainerHeader) + (PTP_MAX_PARAMS * sizeof(u32));
        responseHeader = alloca(responseSize);

        result = UsbK_ReadPipe(usbHandle, deviceUsbk->usbBulkIn, (PUCHAR)responseHeader,
                              responseSize, &actual, &overlapped);
        if (!result && GetLastError() != ERROR_IO_PENDING) {
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&self->logger, "Failed to read final PTP response");
            return PTP_GENERAL_ERROR;
        }

        waitResult = WaitForSingleObject(overlapped.hEvent, deviceUsbk->timeoutMilliseconds);
        if (waitResult != WAIT_OBJECT_0) {
            UsbK_AbortPipe(usbHandle, deviceUsbk->usbBulkIn);
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&self->logger,"Timeout or error while reading final PTP response");
            return PTP_GENERAL_ERROR;
        }

        if (!UsbK_GetOverlappedResult(usbHandle, &overlapped, &actual, FALSE)) {
            CloseHandle(overlapped.hEvent);
            WinUtils_LogLastError(&self->logger, "Failed to get overlapped result for final response");
            return PTP_GENERAL_ERROR;
        }
    }

    CloseHandle(overlapped.hEvent);
    *actualDataOutSize = dataBytesTransferred;

    // 4. Read response packet params
    response->ResponseCode = responseHeader->code;
    response->TransactionId = responseHeader->transactionId;
    MMemIO memIo;
    int paramsSize = responseHeader->length - sizeof(PTPContainerHeader);
    MMemInit(&memIo, self->transport.allocator, ((u8*)responseHeader) + sizeof(PTPContainerHeader), paramsSize);
    response->NumParams = paramsSize / sizeof(u32);
    for (int i = 0; i < response->NumParams; i++) {
        MMemReadU32BE(&memIo, response->Params + i);
    }

    return PTP_OK;
}

static b32 PTPDeviceUsbk_Reset(void* deviceSelf) {
    PTPDevice* device = (PTPDevice*)deviceSelf;
    PTPDeviceUsbk* deviceUsbk = device->device;
    if (!deviceUsbk || !deviceUsbk->usbHandle) {
        return FALSE;
    }
    return UsbK_ResetDevice(deviceUsbk->usbHandle) ? TRUE : FALSE;
}

static b32 FindBulkInOutEndpoints(PTPUsbkDeviceList* self, KUSB_HANDLE usbHandle, UCHAR* bulkIn, UCHAR* bulkOut,
                                  UCHAR* interruptOut, u32 timeoutMilliseconds) {
    USB_CONFIGURATION_DESCRIPTOR configDesc = {};
    OVERLAPPED overlapped = {0};
    UINT transferred = 0;

    // Create event for overlapped I/O
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent) {
        WinUtils_LogLastError(&self->logger, "Failed to create event for overlapped I/O");
        return FALSE;
    }

    // Get configuration descriptor size
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0, 0,
                            (PUCHAR)&configDesc,
                            sizeof(USB_CONFIGURATION_DESCRIPTOR),
                            &transferred)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            CloseHandle(overlapped.hEvent);
            return FALSE;
        }

        // Wait for completion or timeout
        if (WaitForSingleObject(overlapped.hEvent, timeoutMilliseconds) != WAIT_OBJECT_0 ||
              !UsbK_GetOverlappedResult(usbHandle, &overlapped, &transferred, FALSE)) {
            CloseHandle(overlapped.hEvent);
            return FALSE;
        }
    }

    // Allocate buffer for full descriptor
    UCHAR* buffer = (UCHAR*)MMalloc(self->allocator, configDesc.wTotalLength);
    if (!buffer) {
        CloseHandle(overlapped.hEvent);
        return FALSE;
    }

    // Reset event for next operation
    ResetEvent(overlapped.hEvent);

    // Get full configuration descriptor
    if (!UsbK_GetDescriptor(usbHandle,
                            USB_DESCRIPTOR_TYPE_CONFIGURATION,
                            0, 0,
                            buffer,
                            configDesc.wTotalLength,
                            &transferred)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            MFree(self->allocator, buffer, configDesc.wTotalLength);
            CloseHandle(overlapped.hEvent);
            return FALSE;
        }

        // Wait for completion or timeout
        if (WaitForSingleObject(overlapped.hEvent, timeoutMilliseconds) != WAIT_OBJECT_0 ||
              !UsbK_GetOverlappedResult(usbHandle, &overlapped, &transferred, FALSE)) {
            MFree(self->allocator, buffer, configDesc.wTotalLength);
            CloseHandle(overlapped.hEvent);
            return FALSE;
        }
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
            } else if (endPointType == USB_ENDPOINT_TYPE_INTERRUPT) {
                *interruptOut = endPoint->bEndpointAddress;
            }

            if (*bulkIn && *bulkOut && *interruptOut) {
                found = TRUE;
                break;
            }
        }
        ptr += common->bLength;
    }

    MFree(self->allocator, buffer, configDesc.wTotalLength);
    CloseHandle(overlapped.hEvent);
    return found;
}

b32 PTPUsbkDeviceList_ConnectDevice(PTPUsbkDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPUsbkDeviceList_ConnectDevice");
    UsbkDeviceInfo* device = deviceInfo->device;
    KLST_DEVINFO_HANDLE deviceInfoHandle = (KLST_DEVINFO_HANDLE)device->deviceId;
    KUSB_HANDLE usbHandle = NULL;

    if (UsbK_Init(&usbHandle, deviceInfoHandle)) {
        // Find bulk endpoints
        UCHAR bulkIn = 0, bulkOut = 0, interruptOut = 0;
        if (!FindBulkInOutEndpoints(self, usbHandle, &bulkIn, &bulkOut, &interruptOut, self->timeoutMilliseconds)) {
            PTP_WARNING("Failed to connected to device: Unable to get endpoints");
            UsbK_Free(usbHandle);
            return FALSE;
        }

        // Store the interface pointer
        PTPDeviceUsbk *usbkDevice = MArrayAddPtr(self->allocator, self->openDevices);
        usbkDevice->deviceId = device->deviceId;
        usbkDevice->usbHandle = usbHandle;
        usbkDevice->usbBulkIn = bulkIn;
        usbkDevice->usbBulkOut = bulkOut;
        usbkDevice->usbInterrupt = interruptOut;
        usbkDevice->disconnected = FALSE;
        usbkDevice->timeoutMilliseconds = self->timeoutMilliseconds;
        usbkDevice->logger = self->logger;

        (*deviceOut)->transport.reallocBuffer = PTPDeviceUsbk_ReallocBuffer;
        (*deviceOut)->transport.freeBuffer = PTPDeviceUsbk_FreeBuffer;
        (*deviceOut)->transport.sendAndRecvEx = PTPDeviceUsbk_SendAndRecv;
        (*deviceOut)->transport.reset = PTPDeviceUsbk_Reset;
        (*deviceOut)->transport.requiresSessionOpenClose = TRUE;
        (*deviceOut)->logger = self->logger;
        (*deviceOut)->device = usbkDevice;
        (*deviceOut)->backendType = PTP_BACKEND_LIBUSBK;
        (*deviceOut)->disconnected = FALSE;

        return TRUE;
    } else {
        PTP_WARNING("Failed to connected to device: UsbK_Init failed");
        return FALSE;
    }
}

b32 PTPUsbkDeviceList_DisconnectDevice(PTPUsbkDeviceList* self, PTPDevice* device) {
    PTP_TRACE("PTPUsbkDeviceList_DisconnectDevice");
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
    MFree(self->allocator, self, sizeof(PTPUsbkDeviceList));
    return r;
}

b32 PTPUsbkDeviceList_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPUsbkDeviceList* self = backend->self;
    return PTPUsbkDeviceList_RefreshList(self, deviceList);
}

b32 PTPUsbkDeviceList_NeedsRefresh_(PTPBackend* backend) {
    PTPUsbkDeviceList* self = backend->self;
    return PTPUsbkDeviceList_NeedsRefresh(self);
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

b32 PTPUsbkDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds) {
    PTP_LOG_TRACE(&backend->logger, "PTPUsbkDeviceList_OpenBackend");

    if (timeoutMilliseconds == 0) {
        timeoutMilliseconds = USB_TIMEOUT_DEFAULT_MILLISECONDS;
    }

    PTPUsbkDeviceList* deviceList = MMallocZ(backend->allocator, sizeof(PTPUsbkDeviceList));
    backend->self = deviceList;
    backend->close = PTPUsbkDeviceList_Close_;
    backend->refreshList = PTPUsbkDeviceList_RefreshList_;
    backend->needsRefresh = PTPUsbkDeviceList_NeedsRefresh_;
    backend->releaseList = PTPUsbkDeviceList_ReleaseList_;
    backend->openDevice = PTPUsbkDeviceList_ConnectDevice_;
    backend->closeDevice = PTPUsbkDeviceList_DisconnectDevice_;
    deviceList->timeoutMilliseconds = timeoutMilliseconds;
    deviceList->allocator = backend->allocator;
    deviceList->logger = backend->logger;
    return PTPUsbkDeviceList_Open(deviceList);
}
