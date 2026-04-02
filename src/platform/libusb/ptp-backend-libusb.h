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

PTP_EXPORT b32 PTPLibusbDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
PTP_EXPORT b32 PTPLibusbDeviceList_Open(PTPLibusbDeviceList* self);
PTP_EXPORT b32 PTPLibusbDeviceList_Close(PTPLibusbDeviceList* self);
PTP_EXPORT b32 PTPLibusbDeviceList_RefreshList(PTPLibusbDeviceList* self, PTPDeviceInfo** devices);
PTP_EXPORT void PTPLibusbDeviceList_ReleaseList(PTPLibusbDeviceList* self);
PTP_EXPORT AwResult PTPLibusbDeviceList_OpenDevice(PTPLibusbDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);
PTP_EXPORT b32 PTPLibusbDeviceList_CloseDevice(PTPLibusbDeviceList* self, PTPDevice* device);

PTP_EXPORT b32 PTPLibusbDevice_ReadEvent(PTPDevice* device, PTPEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
