#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* device; // libusb_device*
} LibusbDeviceInfo;

typedef struct {
    void* device; // libusb_device*
    void* handle; // libusb_device_handle*
    u8 usbBulkIn;
    u8 usbBulkOut;
    u8 usbInterrupt;
    b32 disconnected;
    // Request timeout - can be adjust before requests
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
} PTPDeviceLibusb;

typedef struct {
    LibusbDeviceInfo* devices;
    PTPDeviceLibusb* openDevices;
    void* context; // libusb_context*
    int timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
} PTPLibusbDeviceList;

PTP_EXPORT b32 PTPLibusbDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
PTP_EXPORT b32 PTPLibusbDeviceList_Open(PTPLibusbDeviceList* self);
PTP_EXPORT b32 PTPLibusbDeviceList_Close(PTPLibusbDeviceList* self);
PTP_EXPORT b32 PTPLibusbDeviceList_RefreshList(PTPLibusbDeviceList* self, PTPDeviceInfo** devices);
PTP_EXPORT void PTPLibusbDeviceList_ReleaseList(PTPLibusbDeviceList* self);
PTP_EXPORT b32 PTPLibusbDeviceList_ConnectDevice(PTPLibusbDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);
PTP_EXPORT b32 PTPLibusbDeviceList_DisconnectDevice(PTPLibusbDeviceList* self, PTPDevice* device);

PTP_EXPORT b32 PTPLibusbDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
