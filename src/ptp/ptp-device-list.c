#include "ptp-device-list.h"

#ifdef WIN32
#include "platform/windows/ptp-backend-wia.h"
#include "platform/windows/ptp-backend-libusbk.h"
#endif

static PTPBackendType sBackends[] = {
#ifdef WIN32
    PTP_BACKEND_WIA,
    PTP_BACKEND_LIBUSBK
#endif
};

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
                PTPBackend* backend = MArrayAddPtr(self->allocator, self->backends);
                backend->type = backendType;
                backend->logger = self->logger;
                backend->allocator = self->allocator;
                if (PTPUsbkDeviceList_OpenBackend(backend, self->timeoutMilliseconds)) {
                    backendsOpened = TRUE;
                }
                break;
            }
            case PTP_BACKEND_WIA: {
                PTPBackend* backend = MArrayAddPtr(self->allocator, self->backends);
                backend->type = backendType;
                backend->logger = self->logger;
                backend->allocator = self->allocator;
                if (PTPWiaDeviceList_OpenBackend(backend)) {
                    backendsOpened = TRUE;
                }
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
            MStrFree(self->allocator, deviceInfo->deviceName);
        }
        if (free) {
            MArrayFree(self->allocator, self->devices);
        } else {
            MArrayInit(self->allocator, self->devices, 0);
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
    self->listUpToDate = TRUE;
    PTP_INFO("Refreshing device list...");
    MArrayEachPtr(self->backends, backend) {
        PTP_INFO_F("Checking backend '%s'...", PTP_GetBackendTypeStr(backend.p->type));
        backend.p->refreshList(backend.p, &self->devices);
    }
    return TRUE;
}

b32 PTPDeviceList_OpenDevice(PTPDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTP_TRACE("PTPDeviceList_OpenDevice");
    PTP_INFO_F("Opening device %s (%s)...", deviceInfo->deviceName.str, deviceInfo->manufacturer.str);

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

const char* PTP_GetBackendTypeStr(PTPBackendType type) {
    switch (type) {
        case PTP_BACKEND_WIA:
            return "WIA";
        case PTP_BACKEND_LIBUSBK:
            return "libusbk";
    }
    MBreakpointf("PTP_GetBackendTypeStr: Unknown PTPBackendType %d", type);
    return "Unknown";
}
