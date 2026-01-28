#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* deviceId;
} UsbkDeviceInfo;

typedef struct {
    void* deviceId;
    void* usbHandle;
    u8 usbBulkIn;
    u8 usbBulkOut;
    u8 usbInterrupt;
    b32 disconnected;
    // Request timeout - can be adjust before requests
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
} PTPUsbkDeviceUsbk;

typedef struct {
    UsbkDeviceInfo* deviceList;
    PTPUsbkDeviceUsbk* openDevices;
    void* deviceListHandle; // USBK Device list handle
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
} PTPUsbkBackend;

PTP_EXPORT b32 PTPUsbkDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
PTP_EXPORT b32 PTPUsbkDeviceList_Open(PTPUsbkBackend* self);
PTP_EXPORT b32 PTPUsbkDeviceList_Close(PTPUsbkBackend* self);
PTP_EXPORT b32 PTPUsbkDeviceList_RefreshList(PTPUsbkBackend* self, PTPDeviceInfo** devices);
PTP_EXPORT void PTPUsbkDeviceList_ReleaseList(PTPUsbkBackend* self);
PTP_EXPORT b32 PTPUsbkDeviceList_OpenDevice(PTPUsbkBackend* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
PTP_EXPORT b32 PTPUsbkDeviceList_CloseDevice(PTPUsbkBackend* self, PTPDevice* device);

PTP_EXPORT b32 PTPUsbkDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
