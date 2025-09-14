#pragma once

#include "ptp/ptp-const.h"
#include "mlib/mlib.h"
#include "ptp/ptp-backend.h"

#include "Wia.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* deviceId;
} WiaDeviceInfo;

typedef struct {
    PTPDeviceTransport transport;
    IWiaItemExtras* device;
    void* deviceId;
    b32 disconnected;
    PTPLog logger;
} PTPDeviceWia;

typedef struct {
    IWiaDevMgr* deviceMgr;
    WiaDeviceInfo* devices;
    IUnknown** eventListeners;
    b32 deviceListUpToDate;
    PTPDeviceWia* openDevices;
    MAllocator* allocator;
    PTPLog logger;
} PTPWiaDeviceList;

b32 PTP_EXPORT PTPWiaDeviceList_OpenBackend(PTPBackend* backend);

PTP_EXPORT b32 PTPWiaDeviceList_Open(PTPWiaDeviceList* self);
PTP_EXPORT b32 PTPWiaDeviceList_Close(PTPWiaDeviceList* self);
PTP_EXPORT b32 PTPWiaDeviceList_Reset(PTPWiaDeviceList* self, PTPDeviceWia* device);
PTP_EXPORT b32 PTPWiaDeviceList_RefreshList(PTPWiaDeviceList* self, PTPDeviceInfo** devices);
PTP_EXPORT void PTPWiaDeviceList_ReleaseList(PTPWiaDeviceList* self);
PTP_EXPORT b32 PTPWiaDeviceList_ConnectDevice(PTPWiaDeviceList* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
PTP_EXPORT b32 PTPWiaDeviceList_DisconnectDevice(PTPWiaDeviceList* self, PTPDevice* device);

// Errors can occur in the WIA Service restart it
PTP_EXPORT void PTPWiaServiceRestart();

#ifdef __cplusplus
} // extern "C"
#endif
