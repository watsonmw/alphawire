#pragma once

#include "mlib/mlib.h"

#include "aw/aw-const.h"
#include "aw/aw-backend.h"

#include "Wia.h"
struct IWiaItemExtras;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* deviceId;
} WiaDeviceInfo;

typedef struct {
    AwDeviceTransport transport;
    IWiaItemExtras* device;
    void* deviceId;
    b32 disconnected;
    AwLog logger;
} AwDeviceWia;

typedef struct {
    IWiaDevMgr* deviceMgr;
    WiaDeviceInfo* devices;
    IUnknown** eventListeners;
    b32 deviceListUpToDate;
    AwDeviceWia* openDevices;
    MAllocator* allocator;
    b32 comInitialized;
    AwLog logger;
} AwWiaDeviceList;

AwResult AW_EXPORT AwWiaDeviceList_OpenBackend(AwBackend* backend);

AW_EXPORT AwResult AwWiaDeviceList_Open(AwWiaDeviceList* self);
AW_EXPORT AwResult AwWiaDeviceList_Close(AwWiaDeviceList* self);
AW_EXPORT AwResult AwWiaDeviceList_Reset(AwWiaDeviceList* self, AwDeviceWia* device);
AW_EXPORT AwResult AwWiaDeviceList_RefreshList(AwWiaDeviceList* self, AwDeviceInfo** devices);
AW_EXPORT AwResult AwWiaDeviceList_ReleaseList(AwWiaDeviceList* self);
AW_EXPORT AwResult AwWiaDeviceList_OpenDevice(AwWiaDeviceList* self, AwDeviceInfo* deviceId, AwDevice** deviceOut);
AW_EXPORT AwResult AwWiaDeviceList_CloseDevice(AwWiaDeviceList* self, AwDevice* device);

// Errors can occur in the WIA Service restart it
AW_EXPORT void PTPWiaServiceRestart();

#ifdef __cplusplus
} // extern "C"
#endif
