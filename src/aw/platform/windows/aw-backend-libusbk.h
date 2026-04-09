#pragma once

#include "mlib/mlib.h"
#include "aw/aw-const.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#include <libusbk.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

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
    AwLog logger;
    // Event handling
    u32 usbInterruptInterval;
    MMemIO eventMem; // Event buffer for reading and parsing events (reused across calls)
    HANDLE eventsEvent;
    OVERLAPPED eventOverlapped;
    // Background event thread
    HANDLE eventThread;
    HANDLE eventThreadStopEvent;
    SRWLOCK eventLock;
    AwPtpEvent* eventList; // MArray of stored events
} PTPUsbkDeviceUsbk;

typedef struct {
    UsbkDeviceInfo* deviceList;
    KLIB_VERSION libkVersion;
    PTPUsbkDeviceUsbk* openDevices;
    void* deviceListHandle; // USBK Device list handle
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    AwLog logger;
    struct AwBackend* backend; // Reference to parent backend
} AwUsbkBackend;

AW_EXPORT AwResult AwUsbkDeviceList_OpenBackend(AwBackend* backend, u32 timeoutMilliseconds);
AW_EXPORT AwResult AwUsbkDeviceList_Open(AwUsbkBackend* self);
AW_EXPORT AwResult AwUsbkDeviceList_Close(AwUsbkBackend* self);
AW_EXPORT AwResult AwUsbkDeviceList_RefreshList(AwUsbkBackend* self, AwDeviceInfo** devices);
AW_EXPORT AwResult AwUsbkDeviceList_ReleaseList(AwUsbkBackend* self);
AW_EXPORT AwResult AwUsbkDeviceList_OpenDevice(AwUsbkBackend* self, AwDeviceInfo* deviceId, AwDevice** deviceOut);
AW_EXPORT AwResult AwUsbkDeviceList_CloseDevice(AwUsbkBackend* self, AwDevice* device);

#ifdef __cplusplus
} // extern "C"
#endif
