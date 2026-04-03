#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"
#include "platform/usb-const.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* device; // libusb_device*
} LibusbDeviceInfo;

typedef struct {
    void* device; // libusb_device*
    void* handle; // libusb_device_handle*
    PTPUsbEndPoints usb;
    b32 disconnected;
    // Request timeout - can be adjust before requests
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
    // Event handling
    u32 usbInterruptInterval;
    MMemIO eventMem; // Event buffer for reading and parsing events (reused across calls)
    // Background event thread
    pthread_t eventThread;
    b32 eventThreadStop : 1;
    b32 eventThreadStarted : 1;
    pthread_mutex_t eventLock;
    PTPEvent* eventList; // MArray of stored events
} PTPDeviceLibusb;

typedef struct {
    LibusbDeviceInfo* devices;
    PTPDeviceLibusb* openDevices;
    void* context; // libusb_context*
    int timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
    struct PTPBackend* backend; // Reference to parent backend
} PTPLibusbDeviceList;

PTP_EXPORT AwResult PTPLibusbDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
PTP_EXPORT AwResult PTPLibusbDeviceList_Open(PTPLibusbDeviceList* self);
PTP_EXPORT AwResult PTPLibusbDeviceList_Close(PTPLibusbDeviceList* self);
PTP_EXPORT AwResult PTPLibusbDeviceList_RefreshList(PTPLibusbDeviceList* self, PTPDeviceInfo** devices);
PTP_EXPORT AwResult PTPLibusbDeviceList_ReleaseList(PTPLibusbDeviceList* self);
PTP_EXPORT AwResult PTPLibusbDeviceList_OpenDevice(PTPLibusbDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);
PTP_EXPORT AwResult PTPLibusbDeviceList_CloseDevice(PTPLibusbDeviceList* self, PTPDevice* device);

PTP_EXPORT AwResult PTPLibusbDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
