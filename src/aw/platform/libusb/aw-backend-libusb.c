#include <libusb.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "mlib/mlib.h"
#include "aw/aw-control.h"
#include "aw/platform/usb-const.h"
#include "aw/platform/libusb/aw-backend-libusb.h"

static b32 CheckDeviceHasPtpEndPoints(AwLibusbDeviceList* self, libusb_device* device, AwUsbEndPoints* outEndPoints) {
    struct libusb_config_descriptor* config;
    int r = libusb_get_active_config_descriptor(device, &config);
    if (r != 0) {
        r = libusb_get_config_descriptor(device, 0, &config);
    }
    if (r != 0) {
        return FALSE;
    }

    b32 hasPTP = FALSE;
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface* iface = &config->interface[i];
        for (int j = 0; j < iface->num_altsetting; j++) {
            const struct libusb_interface_descriptor* iface_desc = &iface->altsetting[j];
            if (iface_desc->bInterfaceClass == USB_CLASS_STILL_IMAGE &&
                iface_desc->bInterfaceSubClass == USB_SUBCLASS_STILL_IMAGE &&
                iface_desc->bInterfaceProtocol == USB_PROTOCOL_PTP) {


                hasPTP = TRUE;
                for (int k = 0; k < iface_desc->bNumEndpoints; k++) {
                    const struct libusb_endpoint_descriptor* ep = &iface_desc->endpoint[k];
                    if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK) {
                        if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                            outEndPoints->bulkIn = ep->bEndpointAddress;
                            outEndPoints->bulkInMaxPacketSize = ep->wMaxPacketSize;
                        } else {
                            outEndPoints->bulkOut = ep->bEndpointAddress;
                            outEndPoints->bulkOutPacketSize = ep->wMaxPacketSize;
                        }
                    } else if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT) {
                        if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                            outEndPoints->interruptIn = ep->bEndpointAddress;
                            outEndPoints->interruptInterval = ep->bInterval;
                        }
                    }
                }
                break;
            }
        }
        if (hasPTP) {
            break;
        }
    }

    libusb_free_config_descriptor(config);
    return hasPTP;
}

AwResult AwLibusbDeviceList_Open(AwLibusbDeviceList* self) {
    AW_TRACE("AwLibusbDeviceList_Open");
    int r = libusb_init((libusb_context**)&self->context);
    if (r < 0) {
        AW_ERROR("libusb_init failed");
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }
    return (AwResult){.code=AW_RESULT_OK};
}

AwResult AwLibusbDeviceList_Close(AwLibusbDeviceList* self) {
    AW_TRACE("AwLibusbDeviceList_Close");
    AwLibusbDeviceList_ReleaseList(self);
    if (self->openDevices) {
        MArrayFree(self->allocator, self->openDevices);
    }
    if (self->context) {
        libusb_exit((libusb_context*)self->context);
        self->context = NULL;
    }
    return (AwResult){.code=AW_RESULT_OK};
}

b32 AwLibusbDeviceList_NeedsRefresh(AwLibusbDeviceList* self) {
    return FALSE;
}

AwResult AwLibusbDeviceList_RefreshList(AwLibusbDeviceList* self, AwDeviceInfo** devices) {
    AW_TRACE("AwLibusbDeviceList_RefreshList");
    libusb_device** list;
    ssize_t count = libusb_get_device_list((libusb_context*)self->context, &list);
    if (count < 0) {
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    for (ssize_t i = 0; i < count; i++) {
        libusb_device* dev = list[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            continue;
        }

        if (desc.idVendor != USB_SONY_VID) {
            continue;
        }

        AwUsbEndPoints endPoints;
        if (CheckDeviceHasPtpEndPoints(self, dev, &endPoints)) {
            libusb_device_handle* handle = NULL;
            r = libusb_open(dev, &handle);
            if (r < 0) {
                PTP_WARNING("Failed to open Sony device for string retrieval");
                continue;
            }

            char product[256] = {0};
            char manufacturer[256] = {0};
            char serial[256] = {0};

            if (desc.iProduct > 0) {
                libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char*)product, sizeof(product));
            }
            if (desc.iManufacturer > 0) {
                libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char*)manufacturer, sizeof(manufacturer));
            }
            if (desc.iSerialNumber > 0) {
                libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, (unsigned char*)serial, sizeof(serial));
            }

            libusb_close(handle);

            LibusbDeviceInfo* libusbDeviceInfo = MArrayAddPtr(self->allocator, self->devices);
            libusbDeviceInfo->device = libusb_ref_device(dev);

            AwDeviceInfo* deviceInfo = MArrayAddPtrZ(self->allocator, *devices);
            deviceInfo->manufacturer = MStrMakeCopyCStr(self->allocator, manufacturer);
            deviceInfo->product = MStrMakeCopyCStr(self->allocator, product);
            deviceInfo->serial = MStrMakeCopyCStr(self->allocator, serial);
            deviceInfo->usbVID = desc.idVendor;
            deviceInfo->usbPID = desc.idProduct;
            deviceInfo->usbVersion = desc.bcdUSB;
            deviceInfo->backendType = AW_BACKEND_LIBUSB;
            deviceInfo->device = libusbDeviceInfo;

            AW_INFO_F("Found device: %.*s (%.*s)", deviceInfo->product.size, deviceInfo->product.str,
                deviceInfo->manufacturer.size, deviceInfo->manufacturer.str);
        }
    }

    libusb_free_device_list(list, 1);
    return (AwResult){.code=AW_RESULT_OK};
}

AwResult AwLibusbDeviceList_ReleaseList(AwLibusbDeviceList* self) {
    AW_TRACE("AwLibusbDeviceList_ReleaseList");
    if (self->devices) {
        for (int i = 0; i < MArraySize(self->devices); i++) {
            libusb_unref_device((libusb_device*)self->devices[i].device);
        }
        MArrayFree(self->allocator, self->devices);
        self->devices = NULL;
    }
    return (AwResult){.code=AW_RESULT_OK};
}

static void* AwDeviceLibusb_ReallocBuffer(AwDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataOldSize + headerSize);
    }
    u8* data = MMallocZ(self->transport.allocator, dataSize);
    return data + headerSize;
}

static void AwDeviceLibusb_FreeBuffer(AwDevice* self, PTPBufferType type, void* dataMem, size_t dataOldSize) {
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataSize);
    }
}

static int BulkTransferOut(AwDeviceLibusb* device, u8* data, size_t transferLen, int* outTransferred)
{
    int totalTransferred = 0;
    int toTransfer = (int)transferLen;
    u8 endPoint = device->usb.bulkOut;
    u32 timeoutMilliseconds = device->timeoutMilliseconds;
    libusb_device_handle* handle = device->handle;
    do
    {
        int transferred = 0;
        int chunkSize = toTransfer > (1 << 15) ? (1 << 15) : toTransfer;
        // AW_INFO_F("libusb_bulk_transfer 1 usbBulkIn: %d", chunkSize);
        int r = libusb_bulk_transfer(handle, endPoint, data, chunkSize, &transferred, timeoutMilliseconds);
        // AW_INFO_F("libusb_bulk_transfer 1 usbBulkIn -> %d", transferred);
        if (r != 0) {
            *outTransferred = totalTransferred;
            return r;
        }
        data += transferred;
        totalTransferred += transferred;
        toTransfer -= transferred;
    } while (toTransfer > 0);

    *outTransferred = totalTransferred;
    return 0;
}

static AwResult AwDeviceLibusb_SendAndRecv(AwDevice* self, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                           PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                           size_t* actualDataOutSize) {
    AwDeviceLibusb* deviceLibusb = self->device;
    libusb_device_handle* handle = deviceLibusb->handle;

    size_t requestSize = sizeof(PTPContainerHeader) + (request->NumParams * sizeof(u32));
    PTPContainerHeader* requestData = alloca(requestSize);
    requestData->length = (u32)requestSize;
    requestData->type = PTP_CONTAINER_COMMAND;
    requestData->code = request->OpCode;
    requestData->transactionId = request->TransactionId;
    u32* params = (u32*)(requestData + 1);
    for (int i = 0; i < request->NumParams; i++) {
        params[i] = request->Params[i];
    }

    int transferred = 0;
    int r = BulkTransferOut(deviceLibusb, (u8*)requestData, requestSize, &transferred);
    if (r != 0) {
        AW_ERROR_F("Failed to send PTP request: %s", libusb_error_name(r));
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    if (dataInSize) {
        size_t fullDataSize = sizeof(PTPContainerHeader) + dataInSize;
        PTPContainerHeader* dataHeader = (PTPContainerHeader*)(dataIn - sizeof(PTPContainerHeader));
        dataHeader->length = (u32)fullDataSize;
        dataHeader->type = PTP_CONTAINER_DATA;
        dataHeader->code = request->OpCode;
        dataHeader->transactionId = request->TransactionId;

        transferred = 0;
        r = BulkTransferOut(deviceLibusb, (u8*)dataHeader, fullDataSize, &transferred);
        if (r != 0) {
            AW_ERROR_F("Failed to send PTP request: %s", libusb_error_name(r));
            return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
        }
    }

    PTPContainerHeader* responseContainer = (PTPContainerHeader*)(dataOut - sizeof(PTPContainerHeader));
    transferred = 0;
    int chunkSize = (int)(sizeof(PTPContainerHeader) + dataOutSize);
    chunkSize = chunkSize > (1 << 15) ? (1 << 15) : chunkSize;
    r = libusb_bulk_transfer(handle, deviceLibusb->usb.bulkIn, (unsigned char*)responseContainer,
        chunkSize, &transferred, deviceLibusb->timeoutMilliseconds);
    if (r != 0) {
        AW_ERROR_F("Failed to read PTP response: %s", libusb_error_name(r));
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    if (transferred < sizeof(PTPContainerHeader)) {
        AW_ERROR_F("Incomplete PTP response received: got: %d expected >= %d", transferred, (int)sizeof(PTPContainerHeader));
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    u32 dataBytesTransferred = 0;
    if (responseContainer->type == PTP_CONTAINER_DATA) {
        u32 payloadLength = responseContainer->length;
        int actual = transferred;
        unsigned char* cp = ((unsigned char*)responseContainer) + actual;

        while ((u32)actual < payloadLength) {
            int chunk = 0;
            chunkSize = (int)(payloadLength - actual);
            chunkSize = chunkSize > (1 << 15) ? (1 << 15) : chunkSize;
            r = libusb_bulk_transfer(handle, deviceLibusb->usb.bulkIn, cp, chunkSize,
                &chunk, deviceLibusb->timeoutMilliseconds);
            if (r != 0) {
                AW_ERROR_F("Failed to read PTP response data: %s", libusb_error_name(r));
                return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
            }
            actual += chunk;

            cp += chunk;
        }
        dataBytesTransferred = (actual > (int)sizeof(PTPContainerHeader)) ? (u32)(actual - sizeof(PTPContainerHeader)) : 0;

        size_t finalResponseSize = sizeof(PTPContainerHeader) + (PTP_MAX_PARAMS * sizeof(u32));
        responseContainer = alloca(finalResponseSize);
        r = libusb_bulk_transfer(handle, deviceLibusb->usb.bulkIn, (unsigned char*)responseContainer,
            (int)finalResponseSize, &transferred, deviceLibusb->timeoutMilliseconds);
        if (r != 0) {
            AW_ERROR_F("Failed to read final PTP response: %s", libusb_error_name(r));
            return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
        }

        if (transferred == 0) {
            r = libusb_bulk_transfer(handle, deviceLibusb->usb.bulkIn, (unsigned char*)responseContainer,
                (int)finalResponseSize, &transferred, deviceLibusb->timeoutMilliseconds);
            if (r != 0) {
                AW_ERROR_F("Failed to read final PTP response: %s", libusb_error_name(r));
                return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
            }
        }

        if (transferred < (int)sizeof(PTPContainerHeader)) {
            AW_ERROR_F("Incomplete PTP response received: got: %d expected >= %d", transferred, (int)sizeof(PTPContainerHeader));
            return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
        }

        // TODO: verify PTP_CONTAINER_ is correct here
    }

    *actualDataOutSize = dataBytesTransferred;
    response->ResponseCode = responseContainer->code;
    response->TransactionId = responseContainer->transactionId;
    int paramsSize = (int)responseContainer->length - (int)sizeof(PTPContainerHeader);
    response->NumParams = paramsSize / (int)sizeof(u32);
    u32* resParams = (u32*)(responseContainer + 1);
    for (int i = 0; i < response->NumParams && i < PTP_MAX_PARAMS; i++) {
        response->Params[i] = resParams[i];
    }

    if (response->ResponseCode == PTP_OK) {
        return (AwResult){.code=AW_RESULT_OK,.ptp=PTP_OK};
    } else {
        return (AwResult){.code=AW_RESULT_PTP_FAILURE,.ptp=(PtpResult)response->ResponseCode};
    }
}

static b32 AwDeviceLibusb_Reset(AwDevice* self) {
    AwDeviceLibusb* d = (AwDeviceLibusb*)self->device;
    int ok = 1;
    if (d->handle) {
        // Best-effort device reset; ignore errors on platforms that require re-enumeration
        int r = libusb_reset_device((libusb_device_handle*)d->handle);
        if (r != 0) {
            AW_WARNING_F("libusb_reset_device failed: %s", libusb_error_name(r));
            ok = 0;
        }
        // Attempt to clear stalls on endpoints
        libusb_clear_halt((libusb_device_handle*)d->handle, d->usb.bulkIn);
        libusb_clear_halt((libusb_device_handle*)d->handle, d->usb.bulkOut);
        if (d->usb.interruptIn) {
            libusb_clear_halt((libusb_device_handle*)d->handle, d->usb.interruptIn);
        }
        // Attempt to (re)claim interface 0
        libusb_claim_interface((libusb_device_handle*)d->handle, 0);
    }
    return ok ? TRUE : FALSE;
}

static b32 ReadEventFromBuffer(AwDeviceLibusb* dev, int transferred, PTPEvent* outEvent) {
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
static void* EventThreadProc(void* lpParameter) {
    AwDeviceLibusb* dev = (AwDeviceLibusb*)lpParameter;

    while (!dev->eventThreadStop) {
        int transferred = 0;

        // Allocate event buffer if not already done
        if (!dev->eventMem.allocator) {
            MMemInitAlloc(&dev->eventMem, dev->allocator, 1024);
        } else {
            dev->eventMem.size = 0;
        }

        // Perform interrupt transfer with a reasonable timeout
        int r = libusb_interrupt_transfer(dev->handle, dev->usb.interruptIn,
            dev->eventMem.mem, dev->eventMem.capacity, &transferred, 1000);

        if (dev->eventThreadStop) {
            break;
        }

        if (r == 0 && transferred > 0) {
            AwPtpEvent tempEvent = {};
            if (ReadEventFromBuffer(dev, transferred, &tempEvent)) {
                pthread_mutex_lock(&dev->eventLock);
                AwPtpEvent* event = MArrayAddPtr(dev->allocator, dev->eventList);
                *event = tempEvent;
                pthread_mutex_unlock(&dev->eventLock);
            }
        } else if (r != LIBUSB_ERROR_TIMEOUT) {
            // Log error but continue running
            AW_LOG_WARNING_F(&dev->logger, "Interrupt transfer failed: %s", libusb_error_name(r));
            // Sleep briefly to avoid busy-looping on persistent errors
            struct timespec ts = {0, 100000000}; // 100ms
            nanosleep(&ts, NULL);
        }
    }

    return NULL;
}

static AwResult AwDeviceLibusb_ReadEvents(AwDevice* self, int timeoutMilliseconds, MAllocator* alloc,
                                          AwPtpEvent** outEvents) {
    AwDeviceLibusb* dev = self->device;

    if (outEvents == NULL) {
        return (AwResult){.code=AW_RESULT_UNSUPPORTED};
    }

    // If event thread is running, return stored events
    if (dev->eventThreadStarted) {
        pthread_mutex_lock(&dev->eventLock);

        // Copy all events to output
        if (dev->eventList && MArraySize(dev->eventList) > 0) {
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

    // Non-threaded mode - this may miss some events if ReadEvents is not called frequently enough
    if (!dev->eventMem.allocator) {
        MMemInitAlloc(&dev->eventMem, self->transport.allocator, 1024);
    } else {
        dev->eventMem.size = 0;
    }

    int transferred = 0;
    int r = libusb_interrupt_transfer(dev->handle, dev->usb.interruptIn,
        dev->eventMem.mem, dev->eventMem.capacity, &transferred, timeoutMilliseconds);

    if (r == 0 && transferred > 0) {
        PTPEvent outEvent = {};
        if (ReadEventFromBuffer(dev, transferred, &outEvent)) {
            PTPEvent* event = MArrayAddPtr(alloc, *outEvents);
            *event = outEvent;
        }
    }

    return (AwResult){.code=AW_RESULT_OK};
}

AwResult AwLibusbDeviceList_OpenDevice(AwLibusbDeviceList* self, AwDeviceInfo* deviceInfo, AwDevice** deviceOut) {
    AW_TRACE("AwLibusbDeviceList_OpenDevice");
    LibusbDeviceInfo* info = deviceInfo->device;
    libusb_device* dev = info->device;
    libusb_device_handle* handle = NULL;

    int r = libusb_open(dev, &handle);
    if (r != 0) {
        AW_ERROR_F("Failed to open device: %s", libusb_error_name(r));
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    if (libusb_kernel_driver_active(handle, 0) == 1) {
        libusb_detach_kernel_driver(handle, 0);
    }

    r = libusb_claim_interface(handle, 0);
    if (r != 0) {
        AW_ERROR_F("Failed to claim interface: %s", libusb_error_name(r));
        libusb_close(handle);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    AwUsbEndPoints endPoints;
    if (!CheckDeviceHasPtpEndPoints(self, dev, &endPoints)) {
        AW_ERROR("Failed to find PTP interfaces");
        libusb_close(handle);
        return (AwResult){.code=AW_RESULT_TRANSPORT_ERROR};
    }

    AwDeviceLibusb* deviceLibusb = MArrayAddPtrZ(self->allocator, self->openDevices);
    deviceLibusb->device = libusb_ref_device(dev);
    deviceLibusb->handle = handle;
    deviceLibusb->usb = endPoints;
    deviceLibusb->disconnected = FALSE;
    deviceLibusb->timeoutMilliseconds = self->timeoutMilliseconds;
    deviceLibusb->allocator = self->allocator;
    deviceLibusb->logger = self->logger;
    deviceLibusb->usbInterruptInterval = 0;
    deviceLibusb->eventThreadStop = FALSE;
    deviceLibusb->eventList = NULL;

    (*deviceOut)->transport.reallocBuffer = AwDeviceLibusb_ReallocBuffer;
    (*deviceOut)->transport.freeBuffer = AwDeviceLibusb_FreeBuffer;
    (*deviceOut)->transport.sendAndRecv = AwDeviceLibusb_SendAndRecv;
    (*deviceOut)->transport.reset = AwDeviceLibusb_Reset;
    (*deviceOut)->transport.readEvents = AwDeviceLibusb_ReadEvents;
    (*deviceOut)->transport.requiresSessionOpenClose = TRUE;
    (*deviceOut)->transport.allocator = self->allocator;
    (*deviceOut)->logger = self->logger;
    (*deviceOut)->device = deviceLibusb;
    (*deviceOut)->backendType = AW_BACKEND_LIBUSB;
    (*deviceOut)->disconnected = FALSE;
    (*deviceOut)->deviceInfo = deviceInfo;

    // Start event thread if allowed
    if (!self->backend->config.disallowSpawnEventThread) {
        pthread_mutex_init(&deviceLibusb->eventLock, NULL);

        pthread_mutex_lock(&deviceLibusb->eventLock);
        MArrayInit(deviceLibusb->allocator, deviceLibusb->eventList, 16);
        pthread_mutex_unlock(&deviceLibusb->eventLock);

        int r = pthread_create(&deviceLibusb->eventThread, NULL, EventThreadProc, deviceLibusb);
        if (r != 0) {
            AW_WARNING_F("Failed to create event thread: %d", r);
            deviceLibusb->eventThreadStop = -1;
            pthread_mutex_destroy(&deviceLibusb->eventLock);
        } else {
            deviceLibusb->eventThreadStarted = -1;
            AW_DEBUG("Started event thread");
        }
    }

    return (AwResult){.code=AW_RESULT_OK};
}

AwResult AwLibusbDeviceList_CloseDevice(AwLibusbDeviceList* self, AwDevice* device) {
    AW_TRACE("AwLibusbDeviceList_CloseDevice");
    AwDeviceLibusb* deviceLibusb = device->device;

    // Stop event thread if running
    if (deviceLibusb->eventThreadStarted) {
        // Signal thread to stop
        deviceLibusb->eventThreadStop = -1;

        // Wait for thread to exit
        pthread_join(deviceLibusb->eventThread, NULL);

        // Clean up event list
        if (deviceLibusb->eventList) {
            MArrayFree(deviceLibusb->allocator, deviceLibusb->eventList);
            deviceLibusb->eventList = NULL;
        }

        pthread_mutex_destroy(&deviceLibusb->eventLock);
    }

    MMemFree(&deviceLibusb->eventMem);

    if (deviceLibusb->handle) {
        libusb_release_interface(deviceLibusb->handle, 0);
        libusb_close(deviceLibusb->handle);
        deviceLibusb->handle = NULL;
    }
    if (deviceLibusb->device) {
        libusb_unref_device(deviceLibusb->device);
        deviceLibusb->device = NULL;
    }

    MArrayEachPtr(self->openDevices, it) {
        if (it.p == deviceLibusb) {
            MArrayRemoveIndex(self->openDevices, it.i);
            break;
        }
    }
    return (AwResult){.code=AW_RESULT_OK};
}

static AwResult AwLibusbDeviceList_Close_(AwBackend* backend) {
    AwLibusbDeviceList* self = backend->self;
    AwResult r = AwLibusbDeviceList_Close(self);
    MFree(self->allocator, self, sizeof(AwLibusbDeviceList));
    return r;
}

static AwResult AwLibusbDeviceList_RefreshList_(AwBackend* backend, AwDeviceInfo** deviceList) {
    AwLibusbDeviceList* self = backend->self;
    return AwLibusbDeviceList_RefreshList(self, deviceList);
}

static b32 AwLibusbDeviceList_NeedsRefresh_(AwBackend* backend) {
    AwLibusbDeviceList* self = backend->self;
    return AwLibusbDeviceList_NeedsRefresh(self);
}

static AwResult AwLibusbDeviceList_ReleaseList_(AwBackend* backend) {
    AwLibusbDeviceList* self = backend->self;
    return AwLibusbDeviceList_ReleaseList(self);
}

static AwResult AwLibusbDeviceList_OpenDevice_(AwBackend* backend, AwDeviceInfo* deviceInfo, AwDevice** deviceOut) {
    AwLibusbDeviceList* self = backend->self;
    return AwLibusbDeviceList_OpenDevice(self, deviceInfo, deviceOut);
}

static AwResult AwLibusbDeviceList_CloseDevice_(AwBackend* backend, AwDevice* device) {
    AwLibusbDeviceList* self = backend->self;
    return AwLibusbDeviceList_CloseDevice(self, device);
}

AwResult AwLibusbDeviceList_OpenBackend(AwBackend* backend, u32 timeoutMilliseconds) {
    AW_LOG_TRACE(&backend->logger, "AwLibusbDeviceList_OpenBackend");
    if (timeoutMilliseconds == 0) {
        timeoutMilliseconds = USB_TIMEOUT_DEFAULT_MILLISECONDS;
    }
    AwLibusbDeviceList* self = MMallocZ(backend->allocator, sizeof(AwLibusbDeviceList));
    self->allocator = backend->allocator;
    self->logger = backend->logger;
    self->timeoutMilliseconds = (int)timeoutMilliseconds;
    self->backend = backend;

    backend->self = self;
    backend->close = AwLibusbDeviceList_Close_;
    backend->refreshList = AwLibusbDeviceList_RefreshList_;
    backend->needsRefresh = AwLibusbDeviceList_NeedsRefresh_;
    backend->releaseList = AwLibusbDeviceList_ReleaseList_;
    backend->openDevice = AwLibusbDeviceList_OpenDevice_;
    backend->closeDevice = AwLibusbDeviceList_CloseDevice_;

    return AwLibusbDeviceList_Open(self);
}
