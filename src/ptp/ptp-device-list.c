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

b32 PTPDeviceList_Open(PTPDeviceList* self) {
    b32 backendsOpened = FALSE;
    for (int i = 0; i < MStaticArraySize(sBackends); ++i) {
        PTPBackendType backendType = sBackends[i];
        switch (backendType) {
            case PTP_BACKEND_LIBUSBK: {
                PTPBackend* backend = MArrayAddPtr(self->backends);
                backend->type = backendType;
                if (PTPUsbkDeviceList_OpenBackend(backend)) {
                    backendsOpened = TRUE;
                }
                break;
            }
            case PTP_BACKEND_WIA: {
                PTPBackend* backend = MArrayAddPtr(self->backends);
                backend->type = backendType;
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
    if (self->devices) {
        for (int i = 0; i < MArraySize(self->devices); i++) {
            PTPDeviceInfo* deviceInfo = self->devices + i;
            MStrFree(deviceInfo->manufacturer);
            MStrFree(deviceInfo->deviceName);
        }
        if (free) {
            MArrayFree(self->devices);
        } else {
            MArrayInit(self->devices, 0);
        }
    }

    MArrayEachPtr(self->backends, backend) {
        backend.p->releaseList(backend.p);
    }
}

b32 PTPDeviceList_Close(PTPDeviceList* self) {
    PTPDeviceList_ReleaseList(self, TRUE);
    MArrayEachPtr(self->backends, backend) {
        backend.p->close(backend.p);
    }
    MArrayFree(self->backends);
    MArrayFree(self->openDevices);
    return TRUE;
}

b32 PTPDeviceList_RefreshList(PTPDeviceList* self) {
    PTPDeviceList_ReleaseList(self, FALSE);
    self->listUpToDate = TRUE;
    MArrayEachPtr(self->backends, backend) {
        backend.p->refreshList(backend.p, &self->devices);
    }
    return TRUE;
}

b32 PTPDeviceList_ConnectDevice(PTPDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut) {
    PTPBackend* backend = PTPDeviceList_GetBackend(self, deviceInfo->backendType);
    PTPDevice* device = MArrayAddPtr(self->openDevices);
    b32 r = backend->openDevice(backend, deviceInfo, &device);
    if (r) {
        *deviceOut = device;
    } else {
        MArrayPop(self->openDevices);
    }
    return r;
}

b32 PTPDeviceList_DisconnectDevice(PTPDeviceList* self, PTPDevice* device) {
    PTPBackend* backend = PTPDeviceList_GetBackend(self, device->backendType);

    MArrayEach(self->openDevices, i) {
        if (self->openDevices + i == device) {
            MArrayRemoveIndex(self->openDevices, i);
            break;
        }
    }

    return backend->closeDevice(backend, device);
}
