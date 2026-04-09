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
    AwUsbEndPoints usb;
    b32 disconnected;
    // Request timeout - can be adjust before requests
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    AwLog logger;
    // Event handling
    u32 usbInterruptInterval;
    MMemIO eventMem; // Event buffer for reading and parsing events (reused across calls)
    // Background event thread
    pthread_t eventThread;
    b32 eventThreadStop : 1;
    b32 eventThreadStarted : 1;
    pthread_mutex_t eventLock;
    AwPtpEvent* eventList; // MArray of stored events
} AwDeviceLibusb;

typedef struct {
    LibusbDeviceInfo* devices;
    AwDeviceLibusb* openDevices;
    void* context; // libusb_context*
    int timeoutMilliseconds;
    MAllocator* allocator;
    AwLog logger;
    struct AwBackend* backend; // Reference to parent backend
} AwLibusbDeviceList;

AW_EXPORT AwResult AwLibusbDeviceList_OpenBackend(AwBackend* backend, u32 timeoutMilliseconds);
AW_EXPORT AwResult AwLibusbDeviceList_Open(AwLibusbDeviceList* self);
AW_EXPORT AwResult AwLibusbDeviceList_Close(AwLibusbDeviceList* self);
AW_EXPORT AwResult AwLibusbDeviceList_RefreshList(AwLibusbDeviceList* self, AwDeviceInfo** devices);
AW_EXPORT AwResult AwLibusbDeviceList_ReleaseList(AwLibusbDeviceList* self);
AW_EXPORT AwResult AwLibusbDeviceList_OpenDevice(AwLibusbDeviceList* self, AwDeviceInfo* deviceInfo, AwDevice** deviceOut);
AW_EXPORT AwResult AwLibusbDeviceList_CloseDevice(AwLibusbDeviceList* self, AwDevice* device);

AW_EXPORT AwResult PTPLibusbDevice_ReadEvent(AwDevice* device, AwPtpEvent* outEvent, int timeoutMilliseconds);

#ifdef __cplusplus
} // extern "C"
#endif
