#pragma once

#include "ptp/ptp-const.h"
#include "Wia.h"
#include "mlib/mlib.h"

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
} PTPDeviceWia;

typedef struct {
    IWiaDevMgr* deviceMgr;
    WiaDeviceInfo* devices;
    IUnknown** eventListeners;
    b32 deviceListUpToDate;
    PTPDeviceWia* openDevices;
} PTPWiaDeviceList;

b32 PTPWiaDeviceList_OpenBackend(PTPBackend* backend);

b32 PTPWiaDeviceList_Open(PTPWiaDeviceList* self);
b32 PTPWiaDeviceList_Close(PTPWiaDeviceList* self);
b32 PTPWiaDeviceList_Reset(PTPWiaDeviceList* self, PTPDeviceWia* device);
b32 PTPWiaDeviceList_RefreshList(PTPWiaDeviceList* self, PTPDeviceInfo** devices);
void PTPWiaDeviceList_ReleaseList(PTPWiaDeviceList* self);
b32 PTPWiaDeviceList_ConnectDevice(PTPWiaDeviceList* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
b32 PTPWiaDeviceList_DisconnectDevice(PTPWiaDeviceList* self, PTPDevice* device);

// Errors can occur in the WIA Service restart it
void PTPWiaServiceRestart();

#ifdef __cplusplus
} // extern "C"
#endif
