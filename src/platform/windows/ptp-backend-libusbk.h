#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#ifdef __cplusplus
extern "C" {
#endif

void ListUsbK();

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
    PTPLog logger;
} PTPDeviceUsbk;

typedef struct {
    UsbkDeviceInfo* devices;
    PTPDeviceUsbk* openDevices;
    void* deviceList;
    int timeoutMilliseconds;
    PTPLog logger;
} PTPUsbkDeviceList;

b32 PTPUsbkDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
b32 PTPUsbkDeviceList_Open(PTPUsbkDeviceList* self);
b32 PTPUsbkDeviceList_Close(PTPUsbkDeviceList* self);
b32 PTPUsbkDeviceList_RefreshList(PTPUsbkDeviceList* self, PTPDeviceInfo** devices);
void PTPUsbkDeviceList_ReleaseList(PTPUsbkDeviceList* self);
b32 PTPUsbkDeviceList_ConnectDevice(PTPUsbkDeviceList* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
b32 PTPUsbkDeviceList_DisconnectDevice(PTPUsbkDeviceList* self, PTPDevice* device);

b32 PTPUsbkDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
