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

b32 PTPUsbkDeviceList_Open(PTPUsbkBackend* self) {
    PTP_TRACE("PTPUsbkDeviceList_Open");
    return TRUE;
}

b32 PTPUsbkDeviceList_Close(PTPUsbkBackend* self) {
    PTP_TRACE("PTPUsbkDeviceList_Close");
    PTPUsbkDeviceList_ReleaseList(self);
    if (self->openDevices) {
        MArrayFree(self->allocator, self->openDevices);
    }
    return TRUE;
}

// Check configuration descriptors for PTP support
static b32 CheckDeviceHasPtpEndPoints(PTPUsbkBackend* self, KUSB_HANDLE usbHandle, u32 timeoutMilliseconds) {
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

static PTPResult IssueOverlappedEventRead(PTPUsbkDeviceUsbk* dev, MAllocator* alloc, UINT* transferred, BOOL* immediateResult) {
    // Create event for async operation
    if (dev->eventsEvent == 0) {
        dev->eventsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!dev->eventsEvent) {
            WinUtils_LogLastError(&dev->logger, "Failed to create event");
            return PTP_GENERAL_ERROR;
        }
    } else {
        ResetEvent(dev->eventsEvent);
    }
    dev->eventOverlapped = (OVERLAPPED){0};
    dev->eventOverlapped.hEvent = dev->eventsEvent;

    if (!dev->eventMem.allocator) {
        MMemInitAlloc(&dev->eventMem, alloc, 1024);
    } else {
        dev->eventMem.size = 0;
    }

    *transferred = 0;
    if (UsbK_ReadPipe(dev->usbHandle, dev->usbInterrupt, dev->eventMem.mem, dev->eventMem.capacity, transferred,
            &dev->eventOverlapped)) {
        *immediateResult = TRUE;
        return PTP_OK;
    } else {
        if (GetLastError() != ERROR_IO_PENDING) {
            dev->eventOverlapped = (OVERLAPPED){0};
            WinUtils_LogLastError(&dev->logger, "Failed to start interrupt read");
            return PTP_GENERAL_ERROR;
        }
        *immediateResult = FALSE;
        return PTP_OK;
    }
}

static b32 ReadEventFromBuffer(PTPUsbkDeviceUsbk* dev, UINT transferred, PTPEvent* outEvent) {
    MMemIO payloadRead;
    MMemInitRead(&payloadRead, dev->eventMem.mem, transferred);

    if (payloadRead.capacity >= sizeof(PTPContainerHeader)) {
        PTPContainerHeader* event = (PTPContainerHeader*)payloadRead.mem;
        if (event->type == PTP_CONTAINER_EVENT) {
            size_t headerSize = sizeof(PTPContainerHeader);
            outEvent->code = event->code;
            if (event->length > headerSize) {
                payloadRead.size += headerSize;
                MMemReadU32LE(&payloadRead, &outEvent->param1);
                MMemReadU32LE(&payloadRead, &outEvent->param2);
                MMemReadU32LE(&payloadRead, &outEvent->param3);
            }
            return TRUE;
        }
    }
    return FALSE;
}

// Background thread function for processing events
static DWORD WINAPI EventThreadProc(LPVOID lpParameter) {
    PTPUsbkDeviceUsbk* dev = (PTPUsbkDeviceUsbk*)lpParameter;

    while (TRUE) {
        // Check if we should stop
        if (WaitForSingleObject(dev->eventThreadStopEvent, 0) == WAIT_OBJECT_0) {
            break;
        }

        UINT transferred = 0;
        BOOL immediateResult = FALSE;

        // Issue overlapped read if not already pending
        if (dev->eventOverlapped.hEvent == 0) {
            PTPResult r = IssueOverlappedEventRead(dev, dev->allocator, &transferred, &immediateResult);
            if (r != PTP_OK) {
                Sleep(100);
                continue;
            }
        }

        DWORD waitResult;
        if (!immediateResult) {
            // Wait for either event completion or stop signal
            HANDLE handles[2] = { dev->eventOverlapped.hEvent, dev->eventThreadStopEvent };
            waitResult = WaitForMultipleObjects(2, handles, FALSE, 1000);

            if (waitResult == WAIT_OBJECT_0 + 1) {
                // Stop signal received
                UsbK_AbortPipe(dev->usbHandle, dev->usbInterrupt);
                break;
            } else if (waitResult == WAIT_OBJECT_0) {
                // Event data available
                UsbK_GetOverlappedResult(dev->usbHandle, &dev->eventOverlapped, &transferred, FALSE);
            } else if (waitResult == WAIT_TIMEOUT) {
                // Timeout, continue loop
                continue;
            } else {
                // Error
                Sleep(100);
                continue;
            }
        }

        if (transferred > 0) {
            PTPEvent tempEvent = {};
            if (ReadEventFromBuffer(dev, transferred, &tempEvent)) {
                AcquireSRWLockExclusive(&dev->eventLock);
                PTPEvent* event = MArrayAddPtr(dev->allocator, dev->eventList);
                *event = tempEvent;
                ReleaseSRWLockExclusive(&dev->eventLock);
            }
        }

        // Reset for next read
        dev->eventOverlapped = (OVERLAPPED){0};
    }

    return 0;
}

static PTPResult PTPDeviceUsbk_ReadEvents(PTPDevice* self, int timeoutMilliseconds, MAllocator* alloc, PTPEvent** outEvents) {
    PTPUsbkDeviceUsbk* dev = self->device;

    if (outEvents == NULL) {
        return PTP_GENERAL_ERROR;
    }

    // If event thread is running, return stored events
    if (dev->eventThread) {
        AcquireSRWLockExclusive(&dev->eventLock);

        // Copy all events to output
        if (dev->eventList && MArraySize(dev->eventList) > 0) {
            for (int i = 0; i < MArraySize(dev->eventList); i++) {
                PTPEvent* event = MArrayAddPtr(alloc, *outEvents);
                *event = dev->eventList[i];
            }
            // Clear the stored events
            MArrayClear(dev->eventList);
        }

        ReleaseSRWLockExclusive(&dev->eventLock);
        return PTP_OK;
    }

    // Non-threaded mode - this may miss some events if ReadEvents is not called frequently enough
    UINT transferred = 0;
    BOOL immediateResult = FALSE;

    if (dev->eventOverlapped.hEvent == 0) {
        PTPResult r = IssueOverlappedEventRead(dev, self->transport.allocator, &transferred, &immediateResult);
        if (r != PTP_OK) {
            return r;
        }
    }

    if (immediateResult) {
        PTPEvent outEvent = {};
        if (ReadEventFromBuffer(dev, transferred, &outEvent)) {
            PTPEvent* event = MArrayAddPtr(alloc, *outEvents);
            *event = outEvent;
        }
    } else {
        DWORD result = WaitForSingleObject(dev->eventOverlapped.hEvent, 0);
        if (result == WAIT_OBJECT_0) {
            // Get results
            UsbK_GetOverlappedResult(dev->usbHandle, &dev->eventOverlapped, &transferred, FALSE);
        } else if (result != WAIT_TIMEOUT) {
            WinUtils_LogLastError(&dev->logger, "Failed waiting for event");
            // No event available, cancel the transfer
            UsbK_AbortPipe(dev->usbHandle, dev->usbInterrupt);
        }
    }

    // Reset and issue another event
    transferred = 0;
    PTPResult r = IssueOverlappedEventRead(dev, self->transport.allocator, &transferred, &immediateResult);
    if (r != PTP_OK) {
        return r;
    }

    if (immediateResult) {
        PTPEvent outEvent = {};
        if (ReadEventFromBuffer(dev, transferred, &outEvent)) {
            PTPEvent* event = MArrayAddPtr(alloc, *outEvents);
            *event = outEvent;
        }
    }

    return PTP_OK;
}

b32 PTPUsbkDeviceList_NeedsRefresh(PTPUsbkBackend* self) {
    PTP_TRACE("PTPUsbkDeviceList_NeedsRefresh");
    return FALSE;
}

b32 PTPUsbkDeviceList_RefreshList(PTPUsbkBackend* self, PTPDeviceInfo** devices) {
    PTP_TRACE("PTPUsbkDeviceList_RefreshList");

    KLST_HANDLE deviceList = NULL;
    KLST_DEVINFO_HANDLE deviceInfo = NULL;
    KUSB_HANDLE usbHandle = NULL;

    // Initialize the device list
    if (!LstK_Init(&deviceList, 0)) {
        PTP_ERROR("Failed to initialize device list.");
        return FALSE;
    }

    self->deviceListHandle = deviceList;

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

                        UsbkDeviceInfo *usbkDevice = MArrayAddPtr(self->allocator, self->deviceList);
                        PTPDeviceInfo* device = MArrayAddPtrZ(self->allocator, *devices);
                        usbkDevice->deviceId = deviceInfo;
                        device->manufacturer = MStrMakeCopyCStr(self->allocator, deviceInfo->Mfg);
                        device->backendType = PTP_BACKEND_LIBUSBK;
                        device->device = usbkDevice;
                        device->product = WinUtils_BSTRWithSizeToUTF8(self->allocator, wideString, length/2);
                        device->usbVID = deviceInfo->Common.Vid;
                        device->usbPID = deviceInfo->Common.Pid;
                        device->serial = MStrMakeCopyCStr(self->allocator, deviceInfo->SerialNumber);
                        device->usbVersion = deviceDescriptor.bcdUSB;

                        PTP_INFO_F("Found device: %.*s (%.*s)", device->product.size, device->product.str,
                            device->manufacturer.size, device->manufacturer.str);
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

    DWORD winRrr = GetLastError();
    if (winRrr != ERROR_SUCCESS && winRrr != ERROR_EMPTY && winRrr != ERROR_NO_MORE_ITEMS) {
        WinUtils_LogLastError(&self->logger, "Failed to enumerate USB devices");
    }

    return TRUE;
}

void PTPUsbkDeviceList_ReleaseList(PTPUsbkBackend* self) {
    PTP_TRACE("PTPUsbkDeviceList_ReleaseList");

    // Free the device list
    if (self->deviceListHandle) {
        LstK_Free((KLST_HANDLE)self->deviceListHandle);
        self->deviceListHandle = NULL;
    }

    if (self->deviceList) {
        for (int i = 0; i < MArraySize(self->deviceList); i++) {
            UsbkDeviceInfo* deviceInfo = self->deviceList + i;
            deviceInfo->deviceId = NULL;
        }
        MArrayFree(self->allocator, self->deviceList);
    }
}

static void* PTPDeviceUsbk_ReallocBuffer(PTPDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize,
                                         size_t dataNewSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataOldSize + headerSize); dataMem = NULL;
    }
    dataMem = MMallocZ(self->transport.allocator, dataSize);
    return ((u8*)dataMem) + headerSize;
}

static void PTPDeviceUsbk_FreeBuffer(PTPDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataSize); dataMem = NULL;
    }
}

static PTPResult PTPDeviceUsbk_SendAndRecv(PTPDevice* self, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                           PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                           size_t* actualDataOutSize) {
    PTPUsbkDeviceUsbk* deviceUsbk = self->device;
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
        return PTP_AW_TIMEOUT;
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
            return PTP_AW_TIMEOUT;
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
        return PTP_AW_TIMEOUT;
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
            return PTP_AW_TIMEOUT;
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

static b32 PTPDeviceUsbk_Reset(PTPDevice* device) {
    PTPUsbkDeviceUsbk* deviceUsbk = device->device;
    if (!deviceUsbk || !deviceUsbk->usbHandle) {
        return FALSE;
    }
    return UsbK_ResetDevice(deviceUsbk->usbHandle) ? TRUE : FALSE;
}

static b32 FindBulkInOutEndpoints(PTPUsbkBackend* self, KUSB_HANDLE usbHandle, UCHAR* bulkIn, UCHAR* bulkOut,
                                  UCHAR* interruptOut, UCHAR* interruptIntervalOut, u32 timeoutMilliseconds) {
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
                *interruptIntervalOut = endPoint->bInterval;
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

b32 PTPUsbkDeviceList_OpenDevice(PTPUsbkBackend* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPUsbkDeviceList_OpenDevice");
    UsbkDeviceInfo* device = deviceInfo->device;
    KLST_DEVINFO_HANDLE deviceInfoHandle = (KLST_DEVINFO_HANDLE)device->deviceId;
    KUSB_HANDLE usbHandle = NULL;

    if (UsbK_Init(&usbHandle, deviceInfoHandle)) {
        // Find bulk endpoints
        UCHAR bulkIn = 0, bulkOut = 0, interruptOut = 0;
        UCHAR interruptInterval = 0;
        if (!FindBulkInOutEndpoints(self, usbHandle, &bulkIn, &bulkOut, &interruptOut, &interruptInterval, self->timeoutMilliseconds)) {
            PTP_WARNING("Failed to connected to device: Unable to get endpoints");
            UsbK_Free(usbHandle);
            return FALSE;
        }

        // Store the interface pointer
        PTPUsbkDeviceUsbk *usbkDevice = MArrayAddPtrZ(self->allocator, self->openDevices);
        usbkDevice->deviceId = device->deviceId;
        usbkDevice->usbHandle = usbHandle;
        usbkDevice->usbBulkIn = bulkIn;
        usbkDevice->usbBulkOut = bulkOut;
        usbkDevice->usbInterrupt = interruptOut;
        usbkDevice->usbInterruptInterval = (u32)interruptInterval;
        usbkDevice->disconnected = FALSE;
        usbkDevice->timeoutMilliseconds = self->timeoutMilliseconds;
        usbkDevice->logger = self->logger;
        usbkDevice->allocator = self->allocator;
        usbkDevice->eventThread = NULL;
        usbkDevice->eventThreadStopEvent = NULL;
        usbkDevice->eventList = NULL;

        (*deviceOut)->transport.reallocBuffer = PTPDeviceUsbk_ReallocBuffer;
        (*deviceOut)->transport.freeBuffer = PTPDeviceUsbk_FreeBuffer;
        (*deviceOut)->transport.sendAndRecv = PTPDeviceUsbk_SendAndRecv;
        (*deviceOut)->transport.reset = PTPDeviceUsbk_Reset;
        (*deviceOut)->transport.readEvents = PTPDeviceUsbk_ReadEvents;
        (*deviceOut)->transport.requiresSessionOpenClose = TRUE;
        (*deviceOut)->logger = self->logger;
        (*deviceOut)->device = usbkDevice;
        (*deviceOut)->backendType = PTP_BACKEND_LIBUSBK;
        (*deviceOut)->disconnected = FALSE;

        // Start event thread if allowed
        if (!self->backend->config.disallowSpawnEventThread) {
            InitializeSRWLock(&usbkDevice->eventLock);
            AcquireSRWLockExclusive(&usbkDevice->eventLock);

            MArrayInit(usbkDevice->allocator, usbkDevice->eventList, 16);

            usbkDevice->eventThreadStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (usbkDevice->eventThreadStopEvent) {
                usbkDevice->eventThread = CreateThread(NULL, 0, EventThreadProc, usbkDevice, 0, NULL);
                if (!usbkDevice->eventThread) {
                    WinUtils_LogLastError(&self->logger, "Failed to create event thread");
                    CloseHandle(usbkDevice->eventThreadStopEvent);
                    usbkDevice->eventThreadStopEvent = NULL;
                } else {
                    PTP_DEBUG("Started event thread");
                }
            } else {
                WinUtils_LogLastError(&self->logger, "Failed to create event stop event");
            }

            ReleaseSRWLockExclusive(&usbkDevice->eventLock);
        }

        return TRUE;
    } else {
        PTP_WARNING("Failed to connected to device: UsbK_Init failed");
        return FALSE;
    }
}

b32 PTPUsbkDeviceList_CloseDevice(PTPUsbkBackend* self, PTPDevice* device) {
    PTP_TRACE("PTPUsbkDeviceList_CloseDevice");
    PTPUsbkDeviceUsbk* deviceUsbk = device->device;

    // Stop event thread if running
    if (deviceUsbk->eventThread) {
        // Signal thread to stop
        SetEvent(deviceUsbk->eventThreadStopEvent);

        // Wait for thread to exit (with timeout)
        DWORD waitResult = WaitForSingleObject(deviceUsbk->eventThread, 5000);
        if (waitResult == WAIT_TIMEOUT) {
            PTP_WARNING("Event thread did not exit in time, terminating");
            TerminateThread(deviceUsbk->eventThread, 1);
        }

        CloseHandle(deviceUsbk->eventThread);
        CloseHandle(deviceUsbk->eventThreadStopEvent);
        deviceUsbk->eventThread = NULL;
        deviceUsbk->eventThreadStopEvent = NULL;

        // Clean up event list
        if (deviceUsbk->eventList) {
            MArrayFree(deviceUsbk->allocator, deviceUsbk->eventList);
            deviceUsbk->eventList = NULL;
        }
    }

    if (deviceUsbk->eventsEvent) {
        CloseHandle(deviceUsbk->eventsEvent);
    }
    MMemFree(&deviceUsbk->eventMem);

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

static b32 PTPUsbkDeviceList_Close_(PTPBackend* backend) {
    PTPUsbkBackend* self = backend->self;
    b32 r = PTPUsbkDeviceList_Close(self);
    MFree(self->allocator, self, sizeof(PTPUsbkBackend));
    return r;
}

static b32 PTPUsbkDeviceList_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPUsbkBackend* self = backend->self;
    return PTPUsbkDeviceList_RefreshList(self, deviceList);
}

static b32 PTPUsbkDeviceList_NeedsRefresh_(PTPBackend* backend) {
    PTPUsbkBackend* self = backend->self;
    return PTPUsbkDeviceList_NeedsRefresh(self);
}

static void PTPUsbkDeviceList_ReleaseList_(PTPBackend* backend) {
    PTPUsbkBackend* self = backend->self;
    PTPUsbkDeviceList_ReleaseList(self);
}

static b32 PTPUsbkDeviceList_OpenDevice_(PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPUsbkBackend* self = backend->self;
    return PTPUsbkDeviceList_OpenDevice(self, deviceInfo, deviceOut);
}

static b32 PTPUsbkDeviceList_CloseDevice_(PTPBackend* backend, PTPDevice* device) {
    PTPUsbkBackend* self = backend->self;
    return PTPUsbkDeviceList_CloseDevice(self, device);
}

b32 PTPUsbkDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds) {
    PTP_LOG_TRACE(&backend->logger, "PTPUsbkDeviceList_OpenBackend");

    if (timeoutMilliseconds == 0) {
        timeoutMilliseconds = USB_TIMEOUT_DEFAULT_MILLISECONDS;
    }

    PTPUsbkBackend* deviceList = MMallocZ(backend->allocator, sizeof(PTPUsbkBackend));
    backend->self = deviceList;
    backend->close = PTPUsbkDeviceList_Close_;
    backend->refreshList = PTPUsbkDeviceList_RefreshList_;
    backend->needsRefresh = PTPUsbkDeviceList_NeedsRefresh_;
    backend->releaseList = PTPUsbkDeviceList_ReleaseList_;
    backend->openDevice = PTPUsbkDeviceList_OpenDevice_;
    backend->closeDevice = PTPUsbkDeviceList_CloseDevice_;
    deviceList->timeoutMilliseconds = timeoutMilliseconds;
    deviceList->allocator = backend->allocator;
    deviceList->logger = backend->logger;
    deviceList->backend = backend;
    return PTPUsbkDeviceList_Open(deviceList);
}
