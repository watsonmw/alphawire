#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sPTPDeviceList {
    PTPDeviceInfo* devices;
    PTPBackend* backends;
    PTPDevice* openDevices;
    b32 listUpToDate;
} PTPDeviceList;

b32 PTPDeviceList_Open(PTPDeviceList* self);
b32 PTPDeviceList_Close(PTPDeviceList* self);
b32 PTPDeviceList_RefreshList(PTPDeviceList* self);
b32 PTPDeviceList_ConnectDevice(PTPDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);
b32 PTPDeviceList_DisconnectDevice(PTPDeviceList* self, PTPDevice* device);
PTPBackend* PTPDeviceList_GetBackend(PTPDeviceList* self, PtpBackendType backend);

#ifdef __cplusplus
} // extern "C"
#endif
