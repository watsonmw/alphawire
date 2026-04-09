#include "aw-device-list.h"

#ifdef AW_ENABLE_IOKIT
#include "platform/osx/ptp-backend-iokit.h"
#endif
#ifdef AW_ENABLE_WIA
#include "platform/windows/aw-backend-wia.h"
#endif
#ifdef AW_ENABLE_LIBUSBK
#include "platform/windows/aw-backend-libusbk.h"
#endif
#ifdef AW_ENABLE_LIBUSB
#include "platform/libusb/ptp-backend-libusb.h"
#endif
#ifdef AW_ENABLE_IP
#include "platform/ip/aw-backend-ip.h"
#endif

static AwBackendType sBackends[] = {
#ifdef AW_ENABLE_IOKIT
    AW_BACKEND_IOKIT,
#endif
#ifdef AW_ENABLE_WIA
    AW_BACKEND_WIA,
#endif
#ifdef AW_ENABLE_LIBUSBK
    AW_BACKEND_LIBUSBK,
#endif
#ifdef AW_ENABLE_LIBUSB
    AW_BACKEND_LIBUSB,
#endif
#ifdef AW_ENABLE_IP
    AW_BACKEND_IP
#endif
};

static AwBackend* AddBackendSlot(AwDeviceList* self, AwBackendType backendType) {
    AwBackend* backend = MArrayAddPtrZ(self->allocator, self->backends);
    backend->type = backendType;
    backend->logger = self->logger;
    backend->allocator = self->allocator;
    backend->config = self->backendConfig;
    return backend;
}

b32 AwDeviceList_Open(AwDeviceList* self, MAllocator* allocator) {
    self->allocator = allocator;

    b32 backendsOpened = FALSE;
    if (self->logger.logFunc == NULL) {
        self->logger.logFunc = AwLog_LogDefault;
    }

    AW_TRACE("AwDeviceList_Open");

    for (int i = 0; i < MStaticArraySize(sBackends); ++i) {
        AwBackendType backendType = sBackends[i];
        switch (backendType) {
            case AW_BACKEND_LIBUSBK: {
#ifdef AW_ENABLE_LIBUSBK
                AwBackend *backend = AddBackendSlot(self, backendType);
                AwResult result = AwUsbkDeviceList_OpenBackend(backend, self->timeoutMilliseconds);
                if (result.code == AW_RESULT_OK) {
                    backendsOpened = TRUE;
                }
                break;
#endif
            }
            case AW_BACKEND_LIBUSB: {
#ifdef AW_ENABLE_LIBUSB
                AwBackend *backend = AddBackendSlot(self, backendType);
                AwResult result = AwLibusbDeviceList_OpenBackend(backend, self->timeoutMilliseconds);
                if (result.code == AW_RESULT_OK) {
                    backendsOpened = TRUE;
                }
#endif
                break;
            }
            case AW_BACKEND_WIA: {
#ifdef AW_ENABLE_WIA
                AwBackend *backend = AddBackendSlot(self, backendType);
                AwResult result = AwWiaDeviceList_OpenBackend(backend);
                if (result.code == AW_RESULT_OK) {
                    backendsOpened = TRUE;
                }
#endif
                break;
            }
            case AW_BACKEND_IOKIT: {
#ifdef AW_ENABLE_IOKIT
                AwBackend *backend = AddBackendSlot(self, backendType);
                AwResult result = AwIokitDeviceList_OpenBackend(backend, self->timeoutMilliseconds);
                if (result.code == AW_RESULT_OK) {
                    backendsOpened = TRUE;
                }
#endif
                break;
            }
            case AW_BACKEND_IP: {
#ifdef AW_ENABLE_IP
                AwBackend *backend = AddBackendSlot(self, backendType);
                AwResult result = AwIpDeviceList_OpenBackend(backend, self->timeoutMilliseconds);
                if (result.code == AW_RESULT_OK) {
                    backendsOpened = TRUE;
                }
#endif
                break;
            }
        }
    }
    return backendsOpened;
}

AwBackend* AwDeviceList_GetBackend(AwDeviceList* self, AwBackendType backendType) {
    MArrayEachPtr(self->backends, backend) {
        if (backend.p->type == backendType) {
            return backend.p;
        }
    }
    return NULL;
}

void AwDeviceList_ReleaseList(AwDeviceList* self, b32 free) {
    AW_TRACE("AwDeviceList_ReleaseList");
    if (self->devices) {
        for (int i = 0; i < MArraySize(self->devices); i++) {
            AwDeviceInfo* deviceInfo = self->devices + i;
            MStrFree(self->allocator, deviceInfo->manufacturer);
            MStrFree(self->allocator, deviceInfo->product);
            MStrFree(self->allocator, deviceInfo->serial);
            MStrFree(self->allocator, deviceInfo->ipAddress);
        }
        if (free) {
            MArrayFree(self->allocator, self->devices);
        } else {
            MArrayClear(self->devices);
        }
    }

    MArrayEachPtr(self->backends, backend) {
        backend.p->releaseList(backend.p);
    }
}

b32 AwDeviceList_Close(AwDeviceList* self) {
    AW_TRACE("AwDeviceList_Close");
    AwDeviceList_ReleaseList(self, TRUE);
    MArrayEachPtr(self->backends, backend) {
        backend.p->close(backend.p);
    }
    MArrayFree(self->allocator, self->backends);
    MArrayFree(self->allocator, self->openDevices);
    return TRUE;
}

b32 AwDeviceList_RefreshList(AwDeviceList* self) {
    AW_TRACE("AwDeviceList_RefreshList");
    AwDeviceList_ReleaseList(self, FALSE);
    AW_INFO("Refreshing device list...");
    MArrayEachPtr(self->backends, backend) {
        AW_INFO_F("Checking backend '%s'...", AwBackend_GetTypeAsStr(backend.p->type));
        backend.p->refreshList(backend.p, &self->devices);
    }
    return TRUE;
}

size_t AwDeviceList_NumDevices(AwDeviceList* self) {
    return MArraySize(self->devices);
}

b32 AwDeviceList_NeedsRefresh(AwDeviceList* self) {
    AW_TRACE("AwDeviceList_NeedsRefresh");
    MArrayEachPtr(self->backends, backend) {
        if (backend.p->needsRefresh && backend.p->needsRefresh(backend.p)) {
            return TRUE;
        }
    }
    return FALSE;
}

b32 AwDeviceList_PollUpdates(AwDeviceList* self) {
    AW_TRACE("AwDeviceList_PollUpdates");
    b32 ret = FALSE;
    MArrayEachPtr(self->backends, backend) {
        if (backend.p->isRefreshingList && backend.p->pollListUpdates) {
            if (backend.p->isRefreshingList(backend.p)) {
                AW_TRACE_F("PollListUpdates for backend '%s'...", AwBackend_GetTypeAsStr(backend.p->type));
                if (backend.p->pollListUpdates(backend.p,  &self->devices)) {
                    ret = TRUE;
                }
            }
        }
    }
    return ret;
}

b32 AwDeviceList_IsRefreshingList(AwDeviceList* self) {
    AW_TRACE("AwDeviceList_IsRefreshingList");
    MArrayEachPtr(self->backends, backend) {
        if (backend.p->isRefreshingList && backend.p->isRefreshingList(backend.p)) {
            return TRUE;
        }
    }
    return FALSE;
}

AwResult AwDeviceList_OpenDevice(AwDeviceList* self, AwDeviceInfo* deviceInfo, AwDevice** deviceOut) {
    AW_TRACE("AwDeviceList_OpenDevice");
    AW_INFO_F("Opening device %.*s (%.*s)...", deviceInfo->product.size, deviceInfo->product.str,
        deviceInfo->manufacturer.size, deviceInfo->manufacturer.str);

    AwBackend* backend = AwDeviceList_GetBackend(self, deviceInfo->backendType);
    AwDevice* device = MArrayAddPtrZ(self->allocator, self->openDevices);
    AwResult r = backend->openDevice(backend, deviceInfo, &device);
    if (r.code == AW_RESULT_OK) {
        *deviceOut = device;
    } else {
        MArrayPop(self->openDevices);
    }
    return r;
}

AwResult AwDeviceList_CloseDevice(AwDeviceList* self, AwDevice* device) {
    AW_TRACE_F("AwDeviceList_CloseDevice: %p", device->device);
    AwBackend* backend = AwDeviceList_GetBackend(self, device->backendType);

    MArrayEachPtr(self->openDevices, it) {
        if (it.p == device) {
            MArrayRemoveIndex(self->openDevices, it.i);
            break;
        }
    }

    return backend->closeDevice(backend, device);
}
