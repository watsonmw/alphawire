#pragma once

#include "ptp/ptp-const.h"
#include "mlib/mlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void ListUsbK();

typedef struct sUsbkDeviceInfo {
    void* deviceId;
} UsbkDeviceInfo;

typedef struct sPTPDeviceUsbk {
    void* deviceId;
    void* usbHandle;
    u8 usbBulkIn;
    u8 usbBulkOut;
    u8 usbInterrupt;
    b32 disconnected;
} PTPDeviceUsbk;

typedef struct sPTPUsbkDeviceList {
    UsbkDeviceInfo* devices;
    PTPDeviceUsbk* openDevices;
    void* deviceList;
} PTPUsbkDeviceList;

b32 PTPUsbkDeviceList_OpenBackend(PTPBackend* backend);
b32 PTPUsbkDeviceList_Open(PTPUsbkDeviceList* self);
b32 PTPUsbkDeviceList_Close(PTPUsbkDeviceList* self);
b32 PTPUsbkDeviceList_RefreshList(PTPUsbkDeviceList* self, PTPDeviceInfo** devices);
void PTPUsbkDeviceList_ReleaseList(PTPUsbkDeviceList* self);
b32 PTPUsbkDeviceList_ConnectDevice(PTPUsbkDeviceList* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
b32 PTPUsbkDeviceList_DisconnectDevice(PTPUsbkDeviceList* self, PTPDevice* device);

void PTPUsbkDevice_ReadEvent(PTPDevice* device);

#ifdef __cplusplus
} // extern "C"
#endif
