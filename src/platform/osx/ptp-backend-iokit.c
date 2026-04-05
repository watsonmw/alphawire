#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOReturn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USBSpec.h>
#include <mach/mach_error.h>

#include <stdlib.h>
#include <pthread.h>

#include "mlib/mlib.h"
#include "ptp/ptp-control.h"
#include "platform/osx/ptp-backend-iokit.h"
#include "platform/usb-const.h"

const char* IOReturnToString(IOReturn ret) {
    if (ret & sys_iokit) {
        switch (ret) {
            case kIOReturnError:
                return "kIOReturnError";
            case kIOReturnNoMemory:
                return "kIOReturnNoMemory - Can't allocate memory";
            case kIOReturnNoResources:
                return "kIOReturnNoResources - Resource shortage";
            case kIOReturnIPCError:
                return "kIOReturnIPCError - Error during IPC";
            case kIOReturnNoDevice:
                return "kIOReturnNoDevice - No such device";
            case kIOReturnNotPrivileged:
                return "kIOReturnNotPrivileged - Privilege violation";
            case kIOReturnBadArgument:
                return "kIOReturnBadArgument - Invalid argument";
            case kIOReturnLockedRead:
                return "kIOReturnLockedRead - Device read locked";
            case kIOReturnLockedWrite:
                return "kIOReturnLockedWrite - Device write locked";
            case kIOReturnExclusiveAccess:
                return "kIOReturnExclusiveAccess - Exclusive access and device already open";
            case kIOReturnBadMessageID:
                return "kIOReturnBadMessageID - Sent/received messages had different msg_id";
            case kIOReturnUnsupported:
                return "kIOReturnUnsupported - Unsupported function";
            case kIOReturnVMError:
                return "kIOReturnVMError - Misc. VM failure";
            case kIOReturnInternalError:
                return "kIOReturnInternalError - Internal error";
            case kIOReturnIOError:
                return "kIOReturnIOError - General I/O error";
            case kIOReturnCannotLock:
                return "kIOReturnCannotLock - Can't acquire lock";
            case kIOReturnNotOpen:
                return "kIOReturnNotOpen - Device not open";
            case kIOReturnNotReadable:
                return "kIOReturnNotReadable - Read not supported";
            case kIOReturnNotWritable:
                return "kIOReturnNotWritable - Write not supported";
            case kIOReturnNotAligned:
                return "kIOReturnNotAligned - Alignment error";
            case kIOReturnBadMedia:
                return "kIOReturnBadMedia - Media Error";
            case kIOReturnStillOpen:
                return "kIOReturnStillOpen - Device(s) still open";
            case kIOReturnRLDError:
                return "kIOReturnRLDError - Rld failure";
            case kIOReturnDMAError:
                return "kIOReturnDMAError - DMA failure";
            case kIOReturnBusy:
                return "kIOReturnBusy - Device Busy";
            case kIOReturnTimeout:
                return "kIOReturnTimeout - I/O Timeout";
            case kIOReturnOffline:
                return "kIOReturnOffline - Device offline";
            case kIOReturnNotReady:
                return "kIOReturnNotReady - Not ready";
            case kIOReturnNotAttached:
                return "kIOReturnNotAttached - Device not attached";
            case kIOReturnNoChannels:
                return "kIOReturnNoChannels - No DMA channels left";
            case kIOReturnNoSpace:
                return "kIOReturnNoSpace - No space for data";
            case kIOReturnPortExists:
                return "kIOReturnPortExists - Port already exists";
            case kIOReturnCannotWire:
                return "kIOReturnCannotWire - Can't wire down physical memory";
            case kIOReturnNoInterrupt:
                return "kIOReturnNoInterrupt - No interrupt attached";
            case kIOReturnNoFrames:
                return "kIOReturnNoFrames - No DMA frames enqueued";
            case kIOReturnMessageTooLarge:
                return "kIOReturnMessageTooLarge - Oversized msg received on interrupt port";
            case kIOReturnNotPermitted:
                return "kIOReturnNotPermitted - Not permitted";
            case kIOReturnNoPower:
                return "kIOReturnNoPower - No power to device";
            case kIOReturnNoMedia:
                return "kIOReturnNoMedia - Media not present";
            case kIOReturnUnformattedMedia:
                return "kIOReturnUnformattedMedia - Media not formatted";
            case kIOReturnUnsupportedMode:
                return "kIOReturnUnsupportedMode - No such mode";
            case kIOReturnUnderrun:
                return "kIOReturnUnderrun - Data underrun";
            case kIOReturnOverrun:
                return "kIOReturnOverrun - Data overrun";
            case kIOReturnDeviceError:
                return "kIOReturnDeviceError - The device is not working properly!";
            case kIOReturnNoCompletion:
                return "kIOReturnNoCompletion - A completion routine is required";
            case kIOReturnAborted:
                return "kIOReturnAborted - Operation aborted";
            case kIOReturnNoBandwidth:
                return "kIOReturnNoBandwidth - Bus bandwidth would be exceeded";
            case kIOReturnNotResponding:
                return "kIOReturnNotResponding - Device not responding";
            case kIOReturnIsoTooOld:
                return "kIOReturnIsoTooOld - Isochronous I/O request for distant past!";
            case kIOReturnIsoTooNew:
                return "kIOReturnIsoTooNew - Isochronous I/O request for distant future";
            case kIOReturnNotFound:
                return "kIOReturnNotFound - Data was not found";
            case kIOReturnInvalid:
                return "kIOReturnInvalid - Internal error";
            default:
                break;
        }
    }

    return mach_error_string(ret);
}

// Match USB devices register with Sony vendor id
static CFMutableDictionaryRef BuildSonyUSBDeviceMatchingDict() {
    CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
    if (matchingDict) {
        SInt32 vendorId = USB_SONY_VID;
        CFNumberRef vidRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vendorId);
        CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorID), vidRef);
        CFRelease(vidRef);
        CFDictionarySetValue (matchingDict, CFSTR (kUSBProductID), CFSTR("*"));
    }
    return matchingDict;
}

// Check If device is Sony device and has PTP interface
static b32 CheckIfSupportedDevice(io_object_t usbDevice, io_service_t* serviceOut) {
    b32 isSonyPtpDevice = FALSE;
    CFMutableDictionaryRef props;
    kern_return_t kr = IORegistryEntryCreateCFProperties(usbDevice, &props, kCFAllocatorDefault, 0);
    if (kr == KERN_SUCCESS && props) {
        CFNumberRef cfVid = (CFNumberRef)CFDictionaryGetValue(props, CFSTR(kUSBVendorID));
        SInt32 vidVal = -1;
        if (cfVid) {
            CFNumberGetValue(cfVid, kCFNumberSInt32Type, &vidVal);
        }
        if (vidVal == USB_SONY_VID) {
            // Check for PTP interface on the device
            io_iterator_t interfaceIterator;
            kr = IORegistryEntryGetChildIterator(usbDevice, kIOServicePlane, &interfaceIterator);
            if (kr == KERN_SUCCESS) {
                io_service_t ioService;
                while (!isSonyPtpDevice && (ioService = IOIteratorNext(interfaceIterator)) != IO_OBJECT_NULL) {
                    CFMutableDictionaryRef interfaceProps = NULL;
                    if (IORegistryEntryCreateCFProperties(ioService, &interfaceProps, kCFAllocatorDefault, 0) == KERN_SUCCESS && interfaceProps) {
                        SInt32 classVal = 0, subClassVal = 0, protocolVal = 0;
                        CFNumberRef ifaceClass = (CFNumberRef) CFDictionaryGetValue(interfaceProps, CFSTR(kUSBInterfaceClass));
                        if (ifaceClass) {
                            CFNumberGetValue(ifaceClass, kCFNumberSInt32Type, &classVal);
                        }
                        CFNumberRef ifaceSubClass = (CFNumberRef) CFDictionaryGetValue(interfaceProps, CFSTR(kUSBInterfaceSubClass));
                        if (ifaceSubClass) {
                            CFNumberGetValue(ifaceSubClass, kCFNumberSInt32Type, &subClassVal);
                        }
                        CFNumberRef ifaceProtocol = (CFNumberRef) CFDictionaryGetValue(interfaceProps, CFSTR(kUSBInterfaceProtocol));
                        if (ifaceProtocol) {
                            CFNumberGetValue(ifaceProtocol, kCFNumberSInt32Type, &protocolVal);
                        }

                        if (classVal == USB_CLASS_STILL_IMAGE &&
                            subClassVal == USB_SUBCLASS_STILL_IMAGE &&
                            protocolVal == USB_PROTOCOL_PTP) {
                            isSonyPtpDevice = TRUE;
                            *serviceOut = ioService;
                        }
                        CFRelease(interfaceProps);
                    }
                    IOObjectRelease(ioService);
                }
                IOObjectRelease(interfaceIterator);
            }
        }
    }
    if (props) {
        CFRelease(props);
    }
    return isSonyPtpDevice;
}

// Device attach callback: logs when a Sony PTP-capable device is attached.
static void PTPIokit_DeviceAdded(void* deviceList, io_iterator_t deviceIter) {
    PTPIokitDeviceList* self = (PTPIokitDeviceList*)deviceList;

    io_object_t usbDevice;
    while ((usbDevice = IOIteratorNext(deviceIter)) != IO_OBJECT_NULL) {
        io_service_t ioService;
        b32 isSonyPtpDevice = CheckIfSupportedDevice(usbDevice, &ioService);
        if (isSonyPtpDevice) {
            PTP_LOG_INFO(&self->logger, "PTP device attached");
            self->deviceListUpToDate = FALSE;
        }

        IOObjectRelease(usbDevice);
    }
}

static void PTPIokit_DeviceRemoved(void* deviceList, io_iterator_t deviceIter) {
    PTPIokitDeviceList* self = (PTPIokitDeviceList*)deviceList;
    io_object_t usbDevice;
    while ((usbDevice = IOIteratorNext(deviceIter)) != IO_OBJECT_NULL) {
        CFMutableDictionaryRef props = NULL;
        kern_return_t kr = IORegistryEntryCreateCFProperties(usbDevice, &props, kCFAllocatorDefault, 0);

        if (kr == KERN_SUCCESS && props) {
            PTP_LOG_INFO(&self->logger, "PTP device removed");
            self->deviceListUpToDate = FALSE;
        }

        if (props) {
            CFRelease(props);
        }
        IOObjectRelease(usbDevice);
    }
}

AwResult PTPIokitDeviceList_Open(PTPIokitDeviceList* self) {
    PTP_TRACE("PTPIokitDeviceList_Open");

    // Set up device attach notifications
    mach_port_t mainPort = MACH_PORT_NULL;
    kern_return_t kr = IOMainPort(MACH_PORT_NULL, &mainPort);
    if (kr != KERN_SUCCESS) {
        PTP_ERROR("Failed to get IOKit main port for notifications");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    self->notifyPort = IONotificationPortCreate(mainPort);
    if (!self->notifyPort) {
        PTP_ERROR("Failed to create IOKit notification port");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    self->runLoopSource = IONotificationPortGetRunLoopSource(self->notifyPort);
    if (self->runLoopSource) {
        CFRunLoopAddSource(CFRunLoopGetCurrent(), self->runLoopSource, kCFRunLoopDefaultMode);
    }

    // Match USB devices; filtering is done inside the callback
    CFMutableDictionaryRef addDeviceMatchingDict = BuildSonyUSBDeviceMatchingDict();
    if (!addDeviceMatchingDict) {
        PTP_ERROR("Failed to create matching dictionary for notifications");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    // Register for matched (attach) notifications
    kr = IOServiceAddMatchingNotification(
        self->notifyPort,
        kIOMatchedNotification,
        addDeviceMatchingDict,
        PTPIokit_DeviceAdded,
        self,
        &self->deviceAddedIter);

    if (kr != KERN_SUCCESS) {
        PTP_ERROR("Failed to add matched notification");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    // Arm the notification by draining existing matches
    PTPIokit_DeviceAdded(self, self->deviceAddedIter);

    CFMutableDictionaryRef removeDeviceMatchingDict = BuildSonyUSBDeviceMatchingDict();
    if (!removeDeviceMatchingDict) {
        PTP_ERROR("Failed to create matching dictionary for notifications");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    kr = IOServiceAddMatchingNotification(
        self->notifyPort,
        kIOTerminatedNotification,
        removeDeviceMatchingDict,
        PTPIokit_DeviceRemoved,
        self,
        &self->deviceRemovedIter);

    if (kr != KERN_SUCCESS) {
        PTP_ERROR("Failed to add matched notification");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    // Arm the notification by draining existing matches
    PTPIokit_DeviceRemoved(self, self->deviceRemovedIter);

    return (AwResult){.code=AW_RESULT_OK};
}

AwResult PTPIokitDeviceList_Close(PTPIokitDeviceList* self) {
    PTP_TRACE("PTPIokitDeviceList_Close");
    PTPIokitDeviceList_ReleaseList(self);
    if (self->openDevices) {
        MArrayFree(self->allocator, self->openDevices);
    }
    // Remove notifications
    if (self->deviceRemovedIter) {
        IOObjectRelease(self->deviceRemovedIter);
        self->deviceRemovedIter = IO_OBJECT_NULL;
    }
    if (self->deviceAddedIter) {
        IOObjectRelease(self->deviceAddedIter);
        self->deviceAddedIter = IO_OBJECT_NULL;
    }
    if (self->runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), self->runLoopSource, kCFRunLoopDefaultMode);
        self->runLoopSource = NULL;
    }
    if (self->notifyPort) {
        IONotificationPortDestroy(self->notifyPort);
        self->notifyPort = NULL;
    }
    return (AwResult){.code=AW_RESULT_OK};
}

static MStr CFStringToMStr(MAllocator* alloc, CFStringRef str) {
    MStr r = {};
    if (!str) {
        return r;
    }

    // Get the length needed for the UTF8 string
    CFIndex maxLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), kCFStringEncodingUTF8) + 1;

    // Create a buffer to hold the string
    char* buffer = (char *) MMalloc(alloc, maxLength);
    if (!buffer) {
        return r;
    }

    // Try to convert the string
    if (!CFStringGetCString(str, buffer, maxLength, kCFStringEncodingUTF8)) {
        MFree(alloc, buffer, maxLength);
        return r;
    }

    r.str = buffer;
    r.size = MCStrLen(buffer);
    r.capacity = maxLength;
    return r;
}

AwResult PTPIokitDeviceList_RefreshList(PTPIokitDeviceList* self, PTPDeviceInfo** devices) {
    PTP_TRACE("PTPIokitDeviceList_RefreshList");

    // Setup matching dictionary for USB devices
    CFMutableDictionaryRef matchingDict = BuildSonyUSBDeviceMatchingDict();
    if (!matchingDict) {
        PTP_ERROR("Failed to create matching dictionary");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    // Initialize the device list
    io_iterator_t usbDeviceIter = IO_OBJECT_NULL;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &usbDeviceIter);
    if (kr != KERN_SUCCESS) {
        PTP_ERROR_F("IOServiceGetMatchingServices failed: %s (0x%x)", IOReturnToString(kr), kr);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    io_object_t usbDevice;
    // Iterate through the device list
    while ((usbDevice = IOIteratorNext(usbDeviceIter)) != IO_OBJECT_NULL) {
        // Pull interesting properties from the registry
        CFMutableDictionaryRef props = NULL;
        kr = IORegistryEntryCreateCFProperties(usbDevice, &props, kCFAllocatorDefault, 0);
        if (kr != KERN_SUCCESS || !props) {
            goto nextFail;
        }

        // Iterate through interfaces to find PTP interface
        io_service_t ioService;
        b32 isSonyPtpDevice = CheckIfSupportedDevice(usbDevice, &ioService);

        // Skip device if no PTP interface found
        if (!isSonyPtpDevice) {
            goto nextFail;
        }

        IOKitDeviceInfo* ioKitDevice = MArrayAddPtr(self->allocator, self->devices);
        PTPDeviceInfo* device = MArrayAddPtrZ(self->allocator, *devices);
        ioKitDevice->deviceId = usbDevice;
        device->backendType = PTP_BACKEND_IOKIT;
        device->device = ioKitDevice;

        // Read USB Vendor ID + Product ID
        kern_return_t kr = IORegistryEntryCreateCFProperties(usbDevice, &props, kCFAllocatorDefault, 0);
        if (kr != KERN_SUCCESS) {
            goto nextFail;
        }

        CFNumberRef cfVid = (CFNumberRef)CFDictionaryGetValue(props, CFSTR(kUSBVendorID));
        SInt32 vidVal = -1;
        if (cfVid) {
            CFNumberGetValue(cfVid, kCFNumberSInt32Type, &vidVal);
        }
        device->usbVID = vidVal & 0xFFFF;
        CFNumberRef cfPid = (CFNumberRef)CFDictionaryGetValue(props, CFSTR(kUSBProductID));
        SInt32 pidVal = -1;
        if (cfPid) {
            CFNumberGetValue(cfPid, kCFNumberSInt32Type, &pidVal);
        }
        device->usbPID = pidVal & 0xFFFF;

        // Read USB Vendor / Product / Serial Number
        CFStringRef vendorStr = (CFStringRef)CFDictionaryGetValue(props, CFSTR("USB Vendor Name"));
        if (!vendorStr) {
            vendorStr = (CFStringRef)CFDictionaryGetValue(props, CFSTR(kUSBVendorString));
        }
        device->manufacturer = CFStringToMStr(self->allocator, vendorStr);

        CFStringRef productStr = (CFStringRef)CFDictionaryGetValue(props, CFSTR("USB Product Name"));
        if (!productStr) {
            productStr = (CFStringRef)CFDictionaryGetValue(props, CFSTR(kUSBProductString));
        }
        device->product = CFStringToMStr(self->allocator, productStr);

        CFStringRef serialStr = (CFStringRef)CFDictionaryGetValue(props, CFSTR(kUSBSerialNumberString));
        device->serial = CFStringToMStr(self->allocator, serialStr);

        CFNumberRef usbVersion = (CFNumberRef)CFDictionaryGetValue(props, CFSTR("bcdUSB"));
        SInt32 usbVersionBcdVal = -1;
        if (usbVersion) {
            CFNumberGetValue(usbVersion, kCFNumberSInt32Type, &usbVersionBcdVal);
        }
        device->usbVersion = usbVersionBcdVal;
        goto next;

nextFail:
        IOObjectRelease(usbDevice);
next:
        if (props) {
            CFRelease(props);
        }
    }

    IOObjectRelease(usbDeviceIter);
    self->deviceListUpToDate = TRUE;

    return (AwResult){.code=AW_RESULT_OK};
}

b32 PTPIokitDeviceList_NeedsRefresh(PTPIokitDeviceList* self) {
    PTP_TRACE("PTPIokitDeviceList_NeedsRefresh");
    return !self->deviceListUpToDate;
}

AwResult PTPIokitDeviceList_ReleaseList(PTPIokitDeviceList* self) {
    PTP_TRACE("PTPIokitDeviceList_ReleaseList");

    if (self->devices) {
        for (int i = 0; i < MArraySize(self->devices); i++) {
            IOKitDeviceInfo* deviceInfo = self->devices + i;
            IOObjectRelease(deviceInfo->deviceId);
            deviceInfo->deviceId = 0;
        }
        MArrayFree(self->allocator, self->devices);
    }
    return (AwResult){.code=AW_RESULT_OK};
}

static void* PTPDeviceIokit_ReallocBuffer(PTPDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataOldSize + headerSize); dataMem = NULL;
    }
    dataMem = MMallocZ(self->transport.allocator, dataSize);
    return ((u8*)dataMem) + headerSize;
}

static void PTPDeviceIokit_FreeBuffer(PTPDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataSize); dataMem = NULL;
    }
}

static AwResult PTPDeviceIokit_SendAndRecv(PTPDevice* self, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                           PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                           size_t* actualDataOutSize) {
    PTPDeviceIOKit * deviceIokit = self->device;
    IOReturn result;

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
    result = (*deviceIokit->ioUsbInterface)->WritePipeTO(deviceIokit->ioUsbInterface, deviceIokit->usbBulkOut,
                                                         requestData, requestSize, deviceIokit->timeoutMilliseconds,
                                                         deviceIokit->timeoutMilliseconds);
    if (result != kIOReturnSuccess) {
        PTP_ERROR_F("Failed to send PTP request data: %s (%08x)",
            IOReturnToString(result), result);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    // 2. Write additional data, if provided
    if (dataInSize) {
        requestSize = sizeof(PTPContainerHeader) + dataInSize;
        requestData = (PTPContainerHeader *) (((u8 *) dataIn) - sizeof(PTPContainerHeader));
        requestData->length = requestSize;
        requestData->type = PTP_CONTAINER_DATA;
        requestData->code = request->OpCode;
        requestData->transactionId = request->TransactionId;

        result = (*deviceIokit->ioUsbInterface)->WritePipeTO(deviceIokit->ioUsbInterface, deviceIokit->usbBulkOut,
                                                             requestData, requestSize, deviceIokit->timeoutMilliseconds,
                                                             deviceIokit->timeoutMilliseconds);
        if (result != kIOReturnSuccess) {
            PTP_ERROR_F("Failed to read PTP response data: %s (%08x)",
                IOReturnToString(result), result);
            return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
        }
    }

    // 3. Read data packets
    u32 readPipeDataSize = sizeof(PTPContainerHeader) + dataOutSize;
    PTPContainerHeader *responseHeader = (PTPContainerHeader *) (((u8 *) dataOut) - sizeof(PTPContainerHeader));
    result = (*deviceIokit->ioUsbInterface)->ReadPipeTO(deviceIokit->ioUsbInterface, deviceIokit->usbBulkIn,
                                                      responseHeader, &readPipeDataSize,
                                                      deviceIokit->timeoutMilliseconds,
                                                      deviceIokit->timeoutMilliseconds);
    if (result != kIOReturnSuccess) {
        PTP_ERROR_F("Failed to read PTP response data: %s (%08x)",
            IOReturnToString(result), result);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    u32 dataBytesTransferred = 0;
    if (responseHeader->type == PTP_CONTAINER_DATA) {
        u32 payloadLength = responseHeader->length;
        unsigned char *cp = ((u8 *) responseHeader) + readPipeDataSize;

        while (readPipeDataSize < payloadLength) {
            u32 dataTransfer = payloadLength - readPipeDataSize;
            result = (*deviceIokit->ioUsbInterface)->ReadPipeTO(deviceIokit->ioUsbInterface, deviceIokit->usbBulkIn,
                                                                cp, &dataTransfer, deviceIokit->timeoutMilliseconds,
                                                                deviceIokit->timeoutMilliseconds);
            if (result != kIOReturnSuccess) {
                PTP_ERROR_F("Failed to read PTP response data: %s (%08x)",
                    IOReturnToString(result), result);
                return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
            }

            readPipeDataSize += dataTransfer;
            cp += dataTransfer;
        }
        dataBytesTransferred = (readPipeDataSize > sizeof(PTPContainerHeader)) ?
                readPipeDataSize - sizeof(PTPContainerHeader) : 0;

        // Read response packet
        size_t responseSize = sizeof(PTPContainerHeader) + (PTP_MAX_PARAMS * sizeof(u32));
        responseHeader = alloca(responseSize);

        readPipeDataSize = responseSize;
        result = (*deviceIokit->ioUsbInterface)->ReadPipeTO(deviceIokit->ioUsbInterface, deviceIokit->usbBulkIn,
                                                            responseHeader, &readPipeDataSize,
                                                            deviceIokit->timeoutMilliseconds,
                                                            deviceIokit->timeoutMilliseconds);
        if (result != kIOReturnSuccess) {
            PTP_ERROR_F("Failed to read final PTP response: %s (%08x)",
                IOReturnToString(result), result);
            return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
        }
    }

    *actualDataOutSize = dataBytesTransferred;

    // 4. Read response packet params
    response->ResponseCode = responseHeader->code;
    response->TransactionId = responseHeader->transactionId;
    MMemIO memIo;
    int paramsSize = responseHeader->length - sizeof(PTPContainerHeader);
    MMemInitRead(&memIo, ((u8 *) responseHeader) + sizeof(PTPContainerHeader), paramsSize);
    response->NumParams = paramsSize / sizeof(u32);
    for (int i = 0; i < response->NumParams; i++) {
        MMemReadU32BE(&memIo, response->Params + i);
    }

    if (response->ResponseCode == PTP_OK) {
        return (AwResult){.code=AW_RESULT_OK,.ptp=PTP_OK};
    } else {
        return (AwResult){.code=AW_RESULT_PTP_FAILURE,.ptp=(PTPResult)response->ResponseCode};
    }
}

static b32 PTPDeviceIokit_Reset(PTPDevice* dev) {
    if (!dev || !dev->device) {
        return FALSE;
    }
    PTPDeviceIOKit* d = (PTPDeviceIOKit*)dev->device;
    if (!d || !d->ioUsbDev) {
        return FALSE;
    }

    IOReturn r = (*d->ioUsbDev)->ResetDevice(d->ioUsbDev);
    if (r != kIOReturnSuccess) {
        PTP_LOG_WARNING_F(&dev->logger, "IOKit ResetDevice failed: %s (%08x)", IOReturnToString(r), r);
    }

    // Best-effort: clear any endpoint stalls to recover pipes
    if (d->ioUsbInterface) {
        (*d->ioUsbInterface)->ClearPipeStall(d->ioUsbInterface, d->usbBulkIn);
        (*d->ioUsbInterface)->ClearPipeStall(d->ioUsbInterface, d->usbBulkOut);
        if (d->usbInterrupt) {
            (*d->ioUsbInterface)->ClearPipeStall(d->ioUsbInterface, d->usbInterrupt);
        }
    }

    return (r == kIOReturnSuccess) ? TRUE : FALSE;
}

static b32 ReadEventFromBuffer(PTPDeviceIOKit* dev, UInt32 transferred, PTPEvent* outEvent) {
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

static void HandleInterruptResponse(void *refcon, IOReturn result, void *arg0);

static void NextInterruptRequest(PTPDeviceIOKit* dev) {
    // Perform interrupt transfer with a reasonable timeout
    UInt32 capacity = dev->eventMem.capacity;
    IOReturn r = (*dev->ioUsbInterface)->ReadPipeAsync(
        dev->ioUsbInterface, dev->usbInterrupt,
        dev->eventMem.mem, capacity,
        HandleInterruptResponse, dev);

    if (r != kIOReturnSuccess) {
        PTP_LOG_WARNING_F(&dev->logger, "Queuing next interrupt transfer failed: %s (%08x)",
            IOReturnToString(r), r);
    }
}

static void HandleInterruptResponse(void *refcon, IOReturn result, void *arg0) {
    PTPDeviceIOKit* dev = (PTPDeviceIOKit*)refcon;
    UInt32 transferred = (UInt32)arg0;

    // Add event to event list
    if (result == kIOReturnSuccess && transferred > 0) {
        PTPEvent tempEvent = {};
        if (ReadEventFromBuffer(dev, transferred, &tempEvent)) {
            pthread_mutex_lock(&dev->eventLock);
            PTPEvent* event = MArrayAddPtr(dev->allocator, dev->eventList);
            *event = tempEvent;
            pthread_mutex_unlock(&dev->eventLock);
        }
        NextInterruptRequest(dev);
    } else {
        PTP_LOG_WARNING_F(&dev->logger, "Interrupt transfer failed: %s (%08x)",
            IOReturnToString(result), result);

        if (result != kIOReturnNoDevice && result != kIOReturnNotAttached && result != kIOReturnAborted) {
            NextInterruptRequest(dev);
        }
    }
}

static AwResult PTPDeviceIokit_ReadEvents(PTPDevice* self, int timeoutMilliseconds, MAllocator* alloc,
                                          PTPEvent** outEvents) {
    PTPDeviceIOKit* dev = self->device;

    if (outEvents == NULL) {
        return (AwResult){.code=AW_RESULT_PARAM_ERROR};
    }

    pthread_mutex_lock(&dev->eventLock);
    // Copy all events to output
    if (MArraySize(dev->eventList) > 0) {
        for (int i = 0; i < MArraySize(dev->eventList); i++) {
            PTPEvent* event = MArrayAddPtr(alloc, *outEvents);
            *event = dev->eventList[i];
        }
        // Clear the stored events
        MArrayClear(dev->eventList);
    }
    pthread_mutex_unlock(&dev->eventLock);
    return (AwResult){.code=AW_RESULT_OK};
}

static char* USBTransferTypeAsStr(u8 transferType) {
    switch (transferType) {
        case kUSBControl:
            return "kUSBControl";
        case kUSBIsoc:
            return "kUSBIsoc";
        case kUSBBulk:
            return "kUSBBulk";
        case kUSBInterrupt:
            return "kUSBInterrupt";
        default:
            return "Unknown";
    }
}

static char* USBDirectionAsStr(u8 direction) {
    switch (direction) {
        case kUSBIn:
            return "kUSBIn";
        case kUSBOut:
            return "kUSBOut";
        default:
            return "Unknown";
    }
}

AwResult PTPIokitDeviceList_OpenDevice(PTPIokitDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPIokitDeviceList_OpenDevice");
    IOKitDeviceInfo* device = deviceInfo->device;

    IOCFPlugInInterface** usbDevicePlugin = NULL;
    io_service_t ioService = 0;
    SInt32 score = 0;
    IOReturn kr = 0;
    HRESULT hr = 0;

    kr = IOCreatePlugInInterfaceForService(
        device->deviceId,
        kIOUSBDeviceUserClientTypeID,
        kIOCFPlugInInterfaceID,
        &usbDevicePlugin,
        &score);

    if ((kr != kIOReturnSuccess) || !usbDevicePlugin) {
        PTP_ERROR_F("Failed to connect to device: Unable to open IOKit USB Device: %s (%08x)",
            IOReturnToString(kr), kr);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    IOUSBDeviceInterface** ioUsbDev = NULL;
    hr = (*usbDevicePlugin)->QueryInterface(
        usbDevicePlugin,
        CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
        (LPVOID*)&ioUsbDev
    );
    (*usbDevicePlugin)->Release(usbDevicePlugin);
    if (hr != S_OK || !ioUsbDev) {
        PTP_ERROR_F("Failed to connect to device: Unable to open IOKit USB Device: %s (%08x)",
            IOReturnToString(kr), kr);
        (*ioUsbDev)->Release(ioUsbDev);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    kr = (*ioUsbDev)->USBDeviceOpen(ioUsbDev);
    if (kr != kIOReturnSuccess) {
        PTP_ERROR_F("Failed to connect to device: Unable to open IOKit USB Device: %s (%08x)",
            IOReturnToString(kr), kr);
        goto fail;
    }

    UInt8 config;
    (*ioUsbDev)->GetConfiguration(ioUsbDev, &config);
    if (config == 0) {
        (*ioUsbDev)->SetConfiguration(ioUsbDev, 1);
    }

    // Find PTP interface for device
    IOUSBFindInterfaceRequest request;
    request.bInterfaceClass = USB_CLASS_STILL_IMAGE;
    request.bInterfaceSubClass = USB_SUBCLASS_STILL_IMAGE;
    request.bInterfaceProtocol = USB_PROTOCOL_PTP;
    request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

    io_iterator_t interfaceIter = 0;
    kr = (*ioUsbDev)->CreateInterfaceIterator(ioUsbDev, &request, &interfaceIter);
    if (kr != kIOReturnSuccess) {
        PTP_ERROR_F("Failed to connect to device: Unable to open IOKit USB Interface Iter: %s (%08x)",
            IOReturnToString(kr), kr);
        goto fail;
    }

    while ((ioService = IOIteratorNext(interfaceIter))) {
        IOCFPlugInInterface** plugInInterface = NULL;
        IOUSBInterfaceInterface** ioUsbInterface = NULL;

        // Create a plugin for the interface
        kr = IOCreatePlugInInterfaceForService(
            ioService,
            kIOUSBInterfaceUserClientTypeID,
            kIOCFPlugInInterfaceID,
            &plugInInterface,
            &score);

        if ((kr != kIOReturnSuccess) || !plugInInterface) {
            PTP_ERROR_F("Failed to connect to device: Unable to get IOKit plugin interface for device: %s (%08x)",
                IOReturnToString(kr), kr);
            goto fail;
        }

        hr = (*plugInInterface)->QueryInterface(
            plugInInterface,
            CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
            (LPVOID*)&ioUsbInterface);

        (*plugInInterface)->Release(plugInInterface);

        if (hr != S_OK || !ioUsbInterface) {
            PTP_ERROR_F("Failed to connect to device: Unable to find IOKit USB Interface: %s (%08x)",
                IOReturnToString(kr), kr);
            (*ioUsbInterface)->Release(ioUsbInterface);
            goto fail;
        }

        IOObjectRelease(ioService); ioService = 0;

        if (ioUsbInterface) {
            // Open the interface
            IOReturn r = (*ioUsbInterface)->USBInterfaceOpen(ioUsbInterface);
            if (r != kIOReturnSuccess) {
                PTP_ERROR_F("Failed to connect to device: Unable to get USB endpoints: %s (%08x)",
                    IOReturnToString(r), r);
                (*ioUsbInterface)->Release(ioUsbInterface);
                continue;
            }

            UInt8 numEndpoints = 0;
            r = (*ioUsbInterface)->GetNumEndpoints(ioUsbInterface, &numEndpoints);
            if (r != kIOReturnSuccess) {
                PTP_ERROR_F("Failed to connect to device: Unable to get USB endpoints: %s (%08x)",
                    IOReturnToString(r), r);
                (*ioUsbInterface)->Release(ioUsbInterface);
                continue;
            }

            u8 bulkIn = 0, bulkOut = 0, interruptOut = 0;

            for (UInt8 i=1; i<=numEndpoints; i++) {
                UInt8 direction = 0, number = 0, transferType = 0, interval = 0;
                UInt16 maxPacketSize = 0;
                r = (*ioUsbInterface)->GetPipeProperties(ioUsbInterface, i, &direction, &number,
                    &transferType, &maxPacketSize, &interval);
                if (r != kIOReturnSuccess) {
                    PTP_ERROR_F("Failed to connect to device: Unable to get USB endpoint properties: %s (%08x)",
                        IOReturnToString(r), r);
                    break;
                }

                PTP_TRACE_F("Pipe %d: Transfer Type: %s (%02x) Direction: %s (%02x)", number,
                    USBTransferTypeAsStr(transferType), transferType, USBDirectionAsStr(direction), direction);

                if (transferType == kUSBBulk) {
                    if (direction == kUSBIn) {
                        bulkIn = number;
                    } else if (direction == kUSBOut) {
                        bulkOut = number;
                    }
                } else if (transferType == kUSBInterrupt && direction == kUSBIn) {
                    interruptOut = number;
                }
            }

            if (!bulkIn || !bulkOut || !interruptOut) {
                PTP_WARNING("Failed to connect to device: Unable to get USB endpoints");
                (*ioUsbInterface)->Release(ioUsbInterface);
                continue;
            }

            // Store the interface pointer
            PTPDeviceIOKit *ioKitDevice = MArrayAddPtrZ(self->allocator, self->openDevices);
            ioKitDevice->ioUsbDev = ioUsbDev;
            ioKitDevice->ioUsbInterface = ioUsbInterface;
            ioKitDevice->usbBulkIn = bulkIn;
            ioKitDevice->usbBulkOut = bulkOut;
            ioKitDevice->usbInterrupt = interruptOut;
            ioKitDevice->disconnected = FALSE;
            ioKitDevice->timeoutMilliseconds = self->timeoutMilliseconds;
            ioKitDevice->logger = self->logger;
            ioKitDevice->allocator = self->allocator;
            ioKitDevice->eventList = NULL;

            (*deviceOut)->transport.reallocBuffer = PTPDeviceIokit_ReallocBuffer;
            (*deviceOut)->transport.freeBuffer = PTPDeviceIokit_FreeBuffer;
            (*deviceOut)->transport.sendAndRecv = PTPDeviceIokit_SendAndRecv;
            (*deviceOut)->transport.reset = PTPDeviceIokit_Reset;
            (*deviceOut)->transport.readEvents = PTPDeviceIokit_ReadEvents;
            (*deviceOut)->transport.requiresSessionOpenClose = TRUE;
            (*deviceOut)->transport.allocator = self->allocator;
            (*deviceOut)->logger = self->logger;
            (*deviceOut)->device = ioKitDevice;
            (*deviceOut)->backendType = PTP_BACKEND_IOKIT;
            (*deviceOut)->disconnected = FALSE;

            // Start event processor
            pthread_mutex_init(&ioKitDevice->eventLock, NULL);
            (*ioUsbInterface)->CreateInterfaceAsyncEventSource(ioUsbInterface, &ioKitDevice->asyncEventSource);
            CFRunLoopAddSource(CFRunLoopGetCurrent(), ioKitDevice->asyncEventSource, kCFRunLoopDefaultMode);

            if (!ioKitDevice->eventMem.allocator) {
                MMemInitAlloc(&ioKitDevice->eventMem, ioKitDevice->allocator, 1024);
            } else {
                ioKitDevice->eventMem.size = 0;
            }
            NextInterruptRequest(ioKitDevice);

            if (interfaceIter) {
                IOObjectRelease(interfaceIter);
            }

            return (AwResult){.code=AW_RESULT_OK};
        }
    }

fail:
    if (interfaceIter) {
        IOObjectRelease(interfaceIter);
    }
    if (ioService) {
        IOObjectRelease(ioService);
    }
    if (ioUsbDev) {
        (*ioUsbDev)->USBDeviceClose(ioUsbDev);
    }
    return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
}

AwResult PTPIokitDeviceList_CloseDevice(PTPIokitDeviceList* self, PTPDevice* device) {
    PTP_TRACE("PTPIokitDeviceList_CloseDevice");
    PTPDeviceIOKit* deviceIokit = device->device;

    if (deviceIokit->ioUsbInterface) {
        (*deviceIokit->ioUsbInterface)->AbortPipe(deviceIokit->ioUsbInterface, deviceIokit->usbInterrupt);
        (*deviceIokit->ioUsbInterface)->USBInterfaceClose(deviceIokit->ioUsbInterface);
        deviceIokit->ioUsbInterface = NULL;
    }

    if (deviceIokit->ioUsbDev) {
        (*deviceIokit->ioUsbDev)->USBDeviceClose(deviceIokit->ioUsbDev);
        deviceIokit->ioUsbDev = NULL;
    }

    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), deviceIokit->asyncEventSource, kCFRunLoopDefaultMode);
    CFRelease(deviceIokit->asyncEventSource);

    // Clean up memory used for recv events
    MMemFree(&deviceIokit->eventMem);

    // Clean up event list
    if (deviceIokit->eventList) {
        MArrayFree(deviceIokit->allocator, deviceIokit->eventList);
        deviceIokit->eventList = NULL;
    }

    pthread_mutex_destroy(&deviceIokit->eventLock);

    MArrayEachPtr(self->openDevices, it) {
        if (it.p == deviceIokit) {
            MArrayRemoveIndex(self->openDevices, it.i);
            break;
        }
    }

    return (AwResult){.code=AW_RESULT_OK};
}

AwResult PTPIokitDeviceList_Close_(PTPBackend* backend) {
    PTPIokitDeviceList* self = backend->self;
    AwResult r = PTPIokitDeviceList_Close(self);
    MFree(self->allocator, self, sizeof(PTPIokitDeviceList));
    return r;
}

AwResult PTPIokitDeviceList_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPIokitDeviceList* self = backend->self;
    return PTPIokitDeviceList_RefreshList(self, deviceList);
}

b32 PTPIokitDeviceList_NeedsRefresh_(PTPBackend* backend) {
    PTPIokitDeviceList* self = backend->self;
    return PTPIokitDeviceList_NeedsRefresh(self);
}

AwResult PTPIokitDeviceList_ReleaseList_(PTPBackend* backend) {
    PTPIokitDeviceList* self = backend->self;
    return PTPIokitDeviceList_ReleaseList(self);
}

AwResult PTPIokitDeviceList_OpenDevice_(PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPIokitDeviceList* self = backend->self;
    return PTPIokitDeviceList_OpenDevice(self, deviceInfo, deviceOut);
}

AwResult PTPIokitDeviceList_CloseDevice_(PTPBackend* backend, PTPDevice* device) {
    PTPIokitDeviceList* self = backend->self;
    return PTPIokitDeviceList_CloseDevice(self, device);
}

AwResult PTPIokitDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds) {
    PTP_LOG_TRACE(&backend->logger, "PTPIokitDeviceList_OpenBackend");

    if (timeoutMilliseconds == 0) {
        timeoutMilliseconds = USB_TIMEOUT_DEFAULT_MILLISECONDS;
    }

    // Initialize IOKit mach port
    mach_port_t mainPort;
    kern_return_t kr = IOMainPort(MACH_PORT_NULL, &mainPort);
    if (kr != KERN_SUCCESS) {
        PTP_LOG_ERROR_F(&backend->logger, "Failed to get IOKit master port %s (%08x)",
            IOReturnToString(kr), kr);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    PTPIokitDeviceList* deviceList = MMallocZ(backend->allocator, sizeof(PTPIokitDeviceList));
    backend->self = deviceList;
    backend->close = PTPIokitDeviceList_Close_;
    backend->refreshList = PTPIokitDeviceList_RefreshList_;
    backend->needsRefresh = PTPIokitDeviceList_NeedsRefresh_;
    backend->releaseList = PTPIokitDeviceList_ReleaseList_;
    backend->openDevice = PTPIokitDeviceList_OpenDevice_;
    backend->closeDevice = PTPIokitDeviceList_CloseDevice_;
    deviceList->timeoutMilliseconds = timeoutMilliseconds;
    deviceList->allocator = backend->allocator;
    deviceList->logger = backend->logger;
    deviceList->backend = backend;
    return PTPIokitDeviceList_Open(deviceList);
}
