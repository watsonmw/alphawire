#include <libusb.h>
#include <stdlib.h>
#include <string.h>

#include "mlib/mlib.h"
#include "ptp/ptp-control.h"
#include "platform/usb-const.h"
#include "platform/libusb/ptp-backend-libusb.h"

static b32 CheckDeviceHasPtpEndPoints(PTPLibusbDeviceList* self, libusb_device* device, u8* bulkIn, u8* bulkOut, u8* interruptIn) {
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
                            if (bulkIn) {
                                *bulkIn = ep->bEndpointAddress;
                            }
                        } else {
                            if (bulkOut) {
                                *bulkOut = ep->bEndpointAddress;
                            }
                        }
                    } else if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT) {
                        if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                            if (interruptIn) {
                                *interruptIn = ep->bEndpointAddress;
                            }
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

b32 PTPLibusbDeviceList_Open(PTPLibusbDeviceList* self) {
    PTP_TRACE("PTPLibusbDeviceList_Open");
    int r = libusb_init((libusb_context**)&self->context);
    if (r < 0) {
        PTP_ERROR("libusb_init failed");
        return FALSE;
    }
    return TRUE;
}

b32 PTPLibusbDeviceList_Close(PTPLibusbDeviceList* self) {
    PTP_TRACE("PTPLibusbDeviceList_Close");
    PTPLibusbDeviceList_ReleaseList(self);
    if (self->openDevices) {
        MArrayFree(self->allocator, self->openDevices);
    }
    if (self->context) {
        libusb_exit((libusb_context*)self->context);
        self->context = NULL;
    }
    return TRUE;
}

b32 PTPLibusbDeviceList_NeedsRefresh(PTPLibusbDeviceList* self) {
    return FALSE;
}

b32 PTPLibusbDeviceList_RefreshList(PTPLibusbDeviceList* self, PTPDeviceInfo** devices) {
    PTP_TRACE("PTPLibusbDeviceList_RefreshList");
    libusb_device** list;
    ssize_t count = libusb_get_device_list((libusb_context*)self->context, &list);
    if (count < 0) {
        return FALSE;
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

        u8 bulkIn = 0, bulkOut = 0, interruptIn = 0;
        if (CheckDeviceHasPtpEndPoints(self, dev, &bulkIn, &bulkOut, &interruptIn)) {
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

            PTPDeviceInfo* deviceInfo = MArrayAddPtrZ(self->allocator, *devices);
            deviceInfo->manufacturer = MStrMakeCopyCStr(self->allocator, manufacturer);
            deviceInfo->product = MStrMakeCopyCStr(self->allocator, product);
            deviceInfo->serial = MStrMakeCopyCStr(self->allocator, serial);
            deviceInfo->usbVID = desc.idVendor;
            deviceInfo->usbPID = desc.idProduct;
            deviceInfo->usbVersion = desc.bcdUSB;
            deviceInfo->backendType = PTP_BACKEND_LIBUSB;
            deviceInfo->device = libusbDeviceInfo;

            PTP_INFO_F("Found device: %.*s (%.*s)", deviceInfo->product.size, deviceInfo->product.str,
                deviceInfo->manufacturer.size, deviceInfo->manufacturer.str);
        }
    }

    libusb_free_device_list(list, 1);
    return TRUE;
}

void PTPLibusbDeviceList_ReleaseList(PTPLibusbDeviceList* self) {
    PTP_TRACE("PTPLibusbDeviceList_ReleaseList");
    if (self->devices) {
        for (int i = 0; i < MArraySize(self->devices); i++) {
            libusb_unref_device((libusb_device*)self->devices[i].device);
        }
        MArrayFree(self->allocator, self->devices);
        self->devices = NULL;
    }
}

void* PTPDeviceLibusb_ReallocBuffer(void* deviceSelf, PTPBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataNewSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataOldSize + headerSize);
    }
    u8* data = MMallocZ(self->transport.allocator, dataSize);
    return data + headerSize;
}

void PTPDeviceLibusb_FreeBuffer(void* deviceSelf, PTPBufferType type, void* dataMem, size_t dataOldSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    size_t headerSize = sizeof(PTPContainerHeader);
    size_t dataSize = dataOldSize + headerSize;
    if (dataMem) {
        u8* mem = ((u8*)dataMem)-headerSize;
        MFree(self->transport.allocator, mem, dataSize);
    }
}

PTPResult PTPDeviceLibusb_SendAndRecv(void* deviceSelf, PTPRequestHeader* request, u8* dataIn, size_t dataInSize,
                                     PTPResponseHeader* response, u8* dataOut, size_t dataOutSize,
                                     size_t* actualDataOutSize) {
    PTPDevice* self = (PTPDevice*)deviceSelf;
    PTPDeviceLibusb* deviceLibusb = self->device;
    libusb_device_handle* handle = deviceLibusb->handle;
    int transferred = 0;
    int r;

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

    r = libusb_bulk_transfer(handle, deviceLibusb->usbBulkOut, (unsigned char*)requestData,
        (int)requestSize, &transferred, deviceLibusb->timeoutMilliseconds);
    if (r != 0) {
        PTP_LOG_ERROR_F(&self->logger, "Failed to send PTP request: %s", libusb_error_name(r));
        return PTP_GENERAL_ERROR;
    }

    if (dataInSize) {
        size_t fullDataSize = sizeof(PTPContainerHeader) + dataInSize;
        PTPContainerHeader* dataHeader = (PTPContainerHeader*)(dataIn - sizeof(PTPContainerHeader));
        dataHeader->length = (u32)fullDataSize;
        dataHeader->type = PTP_CONTAINER_DATA;
        dataHeader->code = request->OpCode;
        dataHeader->transactionId = request->TransactionId;

        r = libusb_bulk_transfer(handle, deviceLibusb->usbBulkOut, (unsigned char*)dataHeader,
            (int)fullDataSize, &transferred, deviceLibusb->timeoutMilliseconds);
        if (r != 0) {
            PTP_LOG_ERROR_F(&self->logger, "Failed to send PTP data: %s", libusb_error_name(r));
            return PTP_GENERAL_ERROR;
        }
    }

    PTPContainerHeader* responseContainer = (PTPContainerHeader*)(dataOut - sizeof(PTPContainerHeader));
    r = libusb_bulk_transfer(handle, deviceLibusb->usbBulkIn, (unsigned char*)responseContainer,
        (int)(sizeof(PTPContainerHeader) + dataOutSize), &transferred, deviceLibusb->timeoutMilliseconds);
    if (r != 0) {
        PTP_LOG_ERROR_F(&self->logger, "Failed to read PTP response: %s", libusb_error_name(r));
        return PTP_GENERAL_ERROR;
    }

    u32 dataBytesTransferred = 0;
    if (responseContainer->type == PTP_CONTAINER_DATA) {
        u32 payloadLength = responseContainer->length;
        int actual = transferred;
        unsigned char* cp = ((unsigned char*)responseContainer) + actual;

        while ((u32)actual < payloadLength) {
            int chunk = 0;
            r = libusb_bulk_transfer(handle, deviceLibusb->usbBulkIn, cp, (int)(payloadLength - actual),
                &chunk, deviceLibusb->timeoutMilliseconds);
            if (r != 0) {
                PTP_LOG_ERROR_F(&self->logger, "Failed to read PTP response data: %s", libusb_error_name(r));
                return PTP_GENERAL_ERROR;
            }
            actual += chunk;
            cp += chunk;
        }
        dataBytesTransferred = (actual > (int)sizeof(PTPContainerHeader)) ? (u32)(actual - sizeof(PTPContainerHeader)) : 0;

        size_t finalResponseSize = sizeof(PTPContainerHeader) + (PTP_MAX_PARAMS * sizeof(u32));
        responseContainer = alloca(finalResponseSize);
        r = libusb_bulk_transfer(handle, deviceLibusb->usbBulkIn, (unsigned char*)responseContainer,
            (int)finalResponseSize, &transferred, deviceLibusb->timeoutMilliseconds);
        if (r != 0) {
            PTP_LOG_ERROR_F(&self->logger, "Failed to read final PTP response: %s", libusb_error_name(r));
            return PTP_GENERAL_ERROR;
        }
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

    return PTP_OK;
}

static b32 PTPDeviceLibusb_Reset(void* deviceSelf) {
    PTPDevice* dev = (PTPDevice*)deviceSelf;
    PTPDeviceLibusb* d = (PTPDeviceLibusb*)dev->device;
    int ok = 1;
    if (d->handle) {
        // Best-effort device reset; ignore errors on platforms that require re-enumeration
        int r = libusb_reset_device((libusb_device_handle*)d->handle);
        if (r != 0) {
            PTP_LOG_WARNING_F(&dev->logger, "libusb_reset_device failed: %s", libusb_error_name(r));
            ok = 0;
        }
        // Attempt to clear stalls on endpoints
        libusb_clear_halt((libusb_device_handle*)d->handle, d->usbBulkIn);
        libusb_clear_halt((libusb_device_handle*)d->handle, d->usbBulkOut);
        if (d->usbInterrupt) {
            libusb_clear_halt((libusb_device_handle*)d->handle, d->usbInterrupt);
        }
        // Attempt to (re)claim interface 0
        libusb_claim_interface((libusb_device_handle*)d->handle, 0);
    }
    return ok ? TRUE : FALSE;
}

b32 PTPLibusbDeviceList_ConnectDevice(PTPLibusbDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPLibusbDeviceList_ConnectDevice");
    LibusbDeviceInfo* info = deviceInfo->device;
    libusb_device* dev = info->device;
    libusb_device_handle* handle = NULL;

    int r = libusb_open(dev, &handle);
    if (r != 0) {
        PTP_LOG_ERROR_F(&self->logger, "Failed to open device: %s", libusb_error_name(r));
        return FALSE;
    }

    if (libusb_kernel_driver_active(handle, 0) == 1) {
        libusb_detach_kernel_driver(handle, 0);
    }

    r = libusb_claim_interface(handle, 0);
    if (r != 0) {
        PTP_LOG_ERROR_F(&self->logger, "Failed to claim interface: %s", libusb_error_name(r));
        libusb_close(handle);
        return FALSE;
    }

    u8 bulkIn = 0, bulkOut = 0, interruptIn = 0;
    CheckDeviceHasPtpEndPoints(self, dev, &bulkIn, &bulkOut, &interruptIn);

    PTPDeviceLibusb* deviceLibusb = MArrayAddPtrZ(self->allocator, self->openDevices);
    deviceLibusb->device = libusb_ref_device(dev);
    deviceLibusb->handle = handle;
    deviceLibusb->usbBulkIn = bulkIn;
    deviceLibusb->usbBulkOut = bulkOut;
    deviceLibusb->usbInterrupt = interruptIn;
    deviceLibusb->disconnected = FALSE;
    deviceLibusb->timeoutMilliseconds = self->timeoutMilliseconds;
    deviceLibusb->allocator = self->allocator;
    deviceLibusb->logger = self->logger;

    (*deviceOut)->transport.reallocBuffer = PTPDeviceLibusb_ReallocBuffer;
    (*deviceOut)->transport.freeBuffer = PTPDeviceLibusb_FreeBuffer;
    (*deviceOut)->transport.sendAndRecvEx = PTPDeviceLibusb_SendAndRecv;
    (*deviceOut)->transport.reset = PTPDeviceLibusb_Reset;
    (*deviceOut)->transport.requiresSessionOpenClose = TRUE;
    (*deviceOut)->transport.allocator = self->allocator;
    (*deviceOut)->logger = self->logger;
    (*deviceOut)->device = deviceLibusb;
    (*deviceOut)->backendType = PTP_BACKEND_LIBUSB;
    (*deviceOut)->disconnected = FALSE;
    (*deviceOut)->deviceInfo = deviceInfo;

    return TRUE;
}

b32 PTPLibusbDeviceList_DisconnectDevice(PTPLibusbDeviceList* self, PTPDevice* device) {
    PTP_TRACE("PTPLibusbDeviceList_DisconnectDevice");
    PTPDeviceLibusb* deviceLibusb = device->device;
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
    return TRUE;
}

b32 PTPLibusbDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds) {
    PTPDeviceLibusb* deviceLibusb = device->device;
    unsigned char buffer[512];
    int transferred = 0;
    int r = libusb_interrupt_transfer(deviceLibusb->handle, deviceLibusb->usbInterrupt, buffer,
        sizeof(buffer), &transferred, timeoutMilliseconds);

    if (r == 0 && transferred >= (int)sizeof(PTPContainerHeader)) {
        PTPContainerHeader* header = (PTPContainerHeader*)buffer;
        if (header->type == PTP_CONTAINER_EVENT) {
            outEvent->code = header->code;
            if (header->length >= sizeof(PTPContainerHeader) + 4) {
                outEvent->param1 = *(u32*)(header + 1);
            }
            if (header->length >= sizeof(PTPContainerHeader) + 8) {
                outEvent->param2 = *(u32*)(((u8*)(header + 1)) + 4);
            }
            if (header->length >= sizeof(PTPContainerHeader) + 12) {
                outEvent->param3 = *(u32*)(((u8*)(header + 1)) + 8);
            }
            return TRUE;
        }
    }
    return FALSE;
}

static b32 PTPLibusbDeviceList_Close_(PTPBackend* backend) {
    PTPLibusbDeviceList* self = backend->self;
    b32 r = PTPLibusbDeviceList_Close(self);
    MFree(self->allocator, self, sizeof(PTPLibusbDeviceList));
    return r;
}

static b32 PTPLibusbDeviceList_RefreshList_(PTPBackend* backend, PTPDeviceInfo** deviceList) {
    PTPLibusbDeviceList* self = backend->self;
    return PTPLibusbDeviceList_RefreshList(self, deviceList);
}

static b32 PTPLibusbDeviceList_NeedsRefresh_(PTPBackend* backend) {
    PTPLibusbDeviceList* self = backend->self;
    return PTPLibusbDeviceList_NeedsRefresh(self);
}

static void PTPLibusbDeviceList_ReleaseList_(PTPBackend* backend) {
    PTPLibusbDeviceList* self = backend->self;
    PTPLibusbDeviceList_ReleaseList(self);
}

static b32 PTPLibusbDeviceList_ConnectDevice_(PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPLibusbDeviceList* self = backend->self;
    return PTPLibusbDeviceList_ConnectDevice(self, deviceInfo, deviceOut);
}

static b32 PTPLibusbDeviceList_DisconnectDevice_(PTPBackend* backend, PTPDevice* device) {
    PTPLibusbDeviceList* self = backend->self;
    return PTPLibusbDeviceList_DisconnectDevice(self, device);
}

b32 PTPLibusbDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds) {
    PTP_LOG_TRACE(&backend->logger, "PTPLibusbDeviceList_OpenBackend");
    if (timeoutMilliseconds == 0) {
        timeoutMilliseconds = USB_TIMEOUT_DEFAULT_MILLISECONDS;
    }
    PTPLibusbDeviceList* self = MMallocZ(backend->allocator, sizeof(PTPLibusbDeviceList));
    self->allocator = backend->allocator;
    self->logger = backend->logger;
    self->timeoutMilliseconds = (int)timeoutMilliseconds;

    backend->self = self;
    backend->close = PTPLibusbDeviceList_Close_;
    backend->refreshList = PTPLibusbDeviceList_RefreshList_;
    backend->needsRefresh = PTPLibusbDeviceList_NeedsRefresh_;
    backend->releaseList = PTPLibusbDeviceList_ReleaseList_;
    backend->openDevice = PTPLibusbDeviceList_ConnectDevice_;
    backend->closeDevice = PTPLibusbDeviceList_DisconnectDevice_;

    return PTPLibusbDeviceList_Open(self);
}
