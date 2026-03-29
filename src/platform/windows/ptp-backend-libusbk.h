#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

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
    PTPLog logger;
    // Event handling
    u32 usbInterruptInterval;
    MMemIO eventMem; // Event buffer for reading and parsing events (reused across calls)
    HANDLE eventsEvent;
    OVERLAPPED eventOverlapped;
    // Background event thread
    HANDLE eventThread;
    HANDLE eventThreadStopEvent;
    SRWLOCK eventLock;
    PTPEvent* eventList; // MArray of stored events
} PTPUsbkDeviceUsbk;

typedef struct {
    UsbkDeviceInfo* deviceList;
    KLIB_VERSION libkVersion;
    PTPUsbkDeviceUsbk* openDevices;
    void* deviceListHandle; // USBK Device list handle
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    PTPLog logger;
    struct PTPBackend* backend; // Reference to parent backend
} PTPUsbkBackend;

PTP_EXPORT b32 PTPUsbkDeviceList_OpenBackend(PTPBackend* backend, u32 timeoutMilliseconds);
PTP_EXPORT b32 PTPUsbkDeviceList_Open(PTPUsbkBackend* self);
PTP_EXPORT b32 PTPUsbkDeviceList_Close(PTPUsbkBackend* self);
PTP_EXPORT b32 PTPUsbkDeviceList_RefreshList(PTPUsbkBackend* self, PTPDeviceInfo** devices);
PTP_EXPORT void PTPUsbkDeviceList_ReleaseList(PTPUsbkBackend* self);
PTP_EXPORT b32 PTPUsbkDeviceList_OpenDevice(PTPUsbkBackend* self, PTPDeviceInfo* deviceId, PTPDevice** deviceOut);
PTP_EXPORT b32 PTPUsbkDeviceList_CloseDevice(PTPUsbkBackend* self, PTPDevice* device);

#ifdef __cplusplus
} // extern "C"
#endif
