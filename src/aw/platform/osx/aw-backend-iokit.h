#pragma once

#include "mlib/mlib.h"
#include "aw/aw-const.h"

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
    AwLog logger;
    MAllocator* allocator;
    // Event handling
    MMemIO eventMem; // Event buffer for reading and parsing events (reused across calls)
    // Background event thread
    CFRunLoopSourceRef asyncEventSource;
    pthread_mutex_t eventLock;
    AwPtpEvent* eventList;
} AwDeviceIOKit;

typedef struct {
    IOKitDeviceInfo* devices;
    AwDeviceIOKit* openDevices;
    int timeoutMilliseconds;
    // Device attach notification
    IONotificationPortRef notifyPort;
    io_iterator_t deviceAddedIter;
    io_iterator_t deviceRemovedIter;
    CFRunLoopSourceRef runLoopSource;
    b32 deviceListUpToDate;
    MAllocator* allocator;
    AwLog logger;
    struct AwBackend* backend; // Reference to parent backend
} AwIokitDeviceList;

AW_EXPORT AwResult AwIokitDeviceList_OpenBackend(AwBackend* backend, u32 timeoutMilliseconds);
AW_EXPORT AwResult AwIokitDeviceList_Open(AwIokitDeviceList* self);
AW_EXPORT AwResult AwIokitDeviceList_Close(AwIokitDeviceList* self);
AW_EXPORT AwResult AwIokitDeviceList_RefreshList(AwIokitDeviceList* self, AwDeviceInfo** devices);
AW_EXPORT AwResult AwIokitDeviceList_ReleaseList(AwIokitDeviceList* self);
AW_EXPORT AwResult AwIokitDeviceList_OpenDevice(AwIokitDeviceList* self, AwDeviceInfo* deviceId, AwDevice** deviceOut);
AW_EXPORT AwResult AwIokitDeviceList_CloseDevice(AwIokitDeviceList* self, AwDevice* device);

AW_EXPORT AwResult AwIokitDevice_ReadEvent(AwDevice* device, AwPtpEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
