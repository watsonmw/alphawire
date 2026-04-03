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
    MAllocator* allocator;
    // Event handling
    MMemIO eventMem; // Event buffer for reading and parsing events (reused across calls)
    // Background event thread
    pthread_t eventThread;
    b32 eventThreadStop : 1;
    b32 eventThreadStarted : 1;
    pthread_mutex_t eventLock;
    PTPEvent* eventList;
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
    struct PTPBackend* backend; // Reference to parent backend
} PTPIokitDeviceList;

PTP_EXPORT AwResult PTPIokitDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
PTP_EXPORT AwResult PTPIokitDeviceList_Open(PTPIokitDeviceList* self);
PTP_EXPORT AwResult PTPIokitDeviceList_Close(PTPIokitDeviceList* self);
PTP_EXPORT AwResult PTPIokitDeviceList_RefreshList(PTPIokitDeviceList* self, PTPDeviceInfo** devices);
PTP_EXPORT AwResult PTPIokitDeviceList_ReleaseList(PTPIokitDeviceList* self);
PTP_EXPORT AwResult PTPIokitDeviceList_OpenDevice(PTPIokitDeviceList* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
PTP_EXPORT AwResult PTPIokitDeviceList_CloseDevice(PTPIokitDeviceList* self, PTPDevice* device);

PTP_EXPORT AwResult PTPIokitDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
