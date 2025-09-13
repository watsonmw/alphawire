#include "ptp-device-list.h"

#ifdef PTP_ENABLE_IOKIT
#include "platform/osx/ptp-backend-iokit.h"
#endif
#ifdef PTP_ENABLE_WIA
#include "platform/windows/ptp-backend-wia.h"
#endif
#ifdef PTP_ENABLE_LIBUSBK
#include "platform/windows/ptp-backend-libusbk.h"
#endif

static PTPBackendType sBackends[] = {
#ifdef PTP_ENABLE_IOKIT
    PTP_BACKEND_IOKIT,
#endif
#ifdef PTP_ENABLE_WIA
    PTP_BACKEND_WIA,
#endif
#ifdef PTP_ENABLE_LIBUSBK
    PTP_BACKEND_LIBUSBK
#endif
};

static PTPBackend* AddBackendSlot(PTPDeviceList* self, PTPBackendType backendType) {
    PTPBackend* backend = MArrayAddPtr(self->allocator, self->backends);
    backend->type = backendType;
    backend->logger = self->logger;
    backend->allocator = self->allocator;
    return backend;
}

b32 PTPDeviceList_Open(PTPDeviceList* self, MAllocator* allocator) {
    self->allocator = allocator;

    b32 backendsOpened = FALSE;
    if (self->logger.logFunc == NULL) {
        self->logger.logFunc = PTPLog_LogDefault;
    }

    PTP_TRACE("PTPDeviceList_Open");

    for (int i = 0; i < MStaticArraySize(sBackends); ++i) {
        PTPBackendType backendType = sBackends[i];
        switch (backendType) {
            case PTP_BACKEND_LIBUSBK: {
#ifdef PTP_ENABLE_LIBUSBK
                PTPBackend *backend = AddBackendSlot(self, backendType);
                if (PTPUsbkDeviceList_OpenBackend(backend, self->timeoutMilliseconds)) {
                    backendsOpened = TRUE;
                }
                break;
#endif
            }
            case PTP_BACKEND_WIA: {
#ifdef PTP_ENABLE_WIA
                PTPBackend *backend = AddBackendSlot(self, backendType);
                if (PTPWiaDeviceList_OpenBackend(backend)) {
                    backendsOpened = TRUE;
                }
#endif
                break;
            }
            case PTP_BACKEND_IOKIT: {
#ifdef PTP_ENABLE_IOKIT
                PTPBackend *backend = AddBackendSlot(self, backendType);
                if (PTPIokitDeviceList_OpenBackend(backend, self->timeoutMilliseconds)) {
                    backendsOpened = TRUE;
                }
#endif
                break;
            }
        }
    }
    return backendsOpened;
}

PTPBackend* PTPDeviceList_GetBackend(PTPDeviceList* self, PTPBackendType backendType) {
    MArrayEachPtr(self->backends, backend) {
        if (backend.p->type == backendType) {
            return backend.p;
        }
    }
    return NULL;
}

void PTPDeviceList_ReleaseList(PTPDeviceList* self, b32 free) {
    PTP_TRACE("PTPDeviceList_ReleaseList");
    if (self->devices) {
        for (int i = 0; i < MArraySize(self->devices); i++) {
            PTPDeviceInfo* deviceInfo = self->devices + i;
            MStrFree(self->allocator, deviceInfo->manufacturer);
            MStrFree(self->allocator, deviceInfo->product);
            MStrFree(self->allocator, deviceInfo->serial);
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

b32 PTPDeviceList_Close(PTPDeviceList* self) {
    PTP_TRACE("PTPDeviceList_Close");
    PTPDeviceList_ReleaseList(self, TRUE);
    MArrayEachPtr(self->backends, backend) {
        backend.p->close(backend.p);
    }
    MArrayFree(self->allocator, self->backends);
    MArrayFree(self->allocator, self->openDevices);
    return TRUE;
}

b32 PTPDeviceList_RefreshList(PTPDeviceList* self) {
    PTP_TRACE("PTPDeviceList_RefreshList");
    PTPDeviceList_ReleaseList(self, FALSE);
    PTP_INFO("Refreshing device list...");
    MArrayEachPtr(self->backends, backend) {
        PTP_INFO_F("Checking backend '%s'...", PTPBackend_GetTypeAsStr(backend.p->type));
        backend.p->refreshList(backend.p, &self->devices);
    }
    return TRUE;
}

size_t PTPDeviceList_NumDevices(PTPDeviceList* self) {
    return MArraySize(self->devices);
}

b32 PTPDeviceList_NeedsRefresh(PTPDeviceList* self) {
    PTP_TRACE("PTPDeviceList_NeedsRefresh");
    MArrayEachPtr(self->backends, backend) {
        if (backend.p->needsRefresh && backend.p->needsRefresh(backend.p)) {
            return TRUE;
        }
    }
    return FALSE;
}

b32 PTPDeviceList_OpenDevice(PTPDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPDeviceList_OpenDevice");
    PTP_INFO_F("Opening device %s (%s)...", deviceInfo->product.str, deviceInfo->manufacturer.str);

    PTPBackend* backend = PTPDeviceList_GetBackend(self, deviceInfo->backendType);
    PTPDevice* device = MArrayAddPtr(self->allocator, self->openDevices);
    b32 r = backend->openDevice(backend, deviceInfo, &device);
    if (r) {
        *deviceOut = device;
    } else {
        MArrayPop(self->openDevices);
    }
    return r;
}

b32 PTPDeviceList_CloseDevice(PTPDeviceList* self, PTPDevice* device) {
    PTP_TRACE_F("PTPDeviceList_CloseDevice: %p", device->device);
    PTPBackend* backend = PTPDeviceList_GetBackend(self, device->backendType);

    MArrayEach(self->openDevices, i) {
        if (self->openDevices + i == device) {
            MArrayRemoveIndex(self->openDevices, i);
            break;
        }
    }

    return backend->closeDevice(backend, device);
}
