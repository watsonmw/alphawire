#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#include <IOKit/usb/IOUSBLib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    io_object_t deviceId;
} IOKitDeviceInfo;

typedef struct {
    IOUSBDeviceInterface** ioUsbDev;
    IOUSBInterfaceInterface** ioUsbInterface;
    u8 usbBulkIn;
    u8 usbBulkOut;
    u8 usbInterrupt;
    b32 disconnected;
    u32 timeoutMilliseconds;
    PTPLog logger;
} PTPDeviceIOKit;

typedef struct {
    IOKitDeviceInfo* devices;
    PTPDeviceIOKit* openDevices;
    int timeoutMilliseconds;
    // Device attach notification
    IONotificationPortRef notifyPort;
    io_iterator_t deviceAddedIter;
    io_iterator_t deviceRemovedIter;
    CFRunLoopSourceRef runLoopSource;
    b32 deviceListUpToDate;
    MAllocator* allocator;
    PTPLog logger;
} PTPIokitDeviceList;

PTP_EXPORT b32 PTPIokitDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
PTP_EXPORT b32 PTPIokitDeviceList_Open(PTPIokitDeviceList* self);
PTP_EXPORT b32 PTPIokitDeviceList_Close(PTPIokitDeviceList* self);
PTP_EXPORT b32 PTPIokitDeviceList_RefreshList(PTPIokitDeviceList* self, PTPDeviceInfo** devices);
PTP_EXPORT void PTPIokitDeviceList_ReleaseList(PTPIokitDeviceList* self);
PTP_EXPORT b32 PTPIokitDeviceList_ConnectDevice(PTPIokitDeviceList* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
PTP_EXPORT b32 PTPIokitDeviceList_DisconnectDevice(PTPIokitDeviceList* self, PTPDevice* device);

PTP_EXPORT b32 PTPIokitDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
