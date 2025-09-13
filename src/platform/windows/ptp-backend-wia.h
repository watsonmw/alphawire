#pragma once

#include "ptp/ptp-const.h"
#include "mlib/mlib.h"

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

b32 PTP_EXPORT PTPWiaDeviceList_Open(PTPWiaDeviceList* self);
b32 PTP_EXPORT PTPWiaDeviceList_Close(PTPWiaDeviceList* self);
b32 PTP_EXPORT PTPWiaDeviceList_Reset(PTPWiaDeviceList* self, PTPDeviceWia* device);
b32 PTP_EXPORT PTPWiaDeviceList_RefreshList(PTPWiaDeviceList* self, PTPDeviceInfo** devices);
void PTP_EXPORT PTPWiaDeviceList_ReleaseList(PTPWiaDeviceList* self);
b32 PTP_EXPORT PTPWiaDeviceList_ConnectDevice(PTPWiaDeviceList* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
b32 PTP_EXPORT PTPWiaDeviceList_DisconnectDevice(PTPWiaDeviceList* self, PTPDevice* device);

// Errors can occur in the WIA Service restart it
void PTP_EXPORT PTPWiaServiceRestart();

#ifdef __cplusplus
} // extern "C"
#endif
