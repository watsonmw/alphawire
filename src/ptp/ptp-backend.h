#pragma once

#include "ptp/ptp-const.h"
#include "ptp/ptp-log.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PTPDevice;

typedef void* (*PTP_Device_ReallocBuffer_t)(struct PTPDevice* device, PTPBufferType type, void* dataMem,
                                            size_t dataOldSize, size_t dataNewSize);
typedef void (*PTP_Device_FreeBuffer_t)(struct PTPDevice* device, PTPBufferType type, void* dataMem,
                                        size_t dataOldSize);
typedef AwResult (*PTP_Device_SendAndRecv_t)(struct PTPDevice* device, PTPRequestHeader* request, u8* dataIn,
                                             size_t dataInSize, PTPResponseHeader* response, u8* dataOut,
                                             size_t dataOutSize, size_t* actualDataOutSize);
typedef b32 (*PTP_Device_Reset_t)(struct PTPDevice* device);
typedef AwResult (*PTP_Device_ReadEvents_Func_t)(struct PTPDevice* device, int timeoutMilliseconds, MAllocator* alloc,
                                                 PTPEvent** outEventList);

// Device transport for communicating with a device
typedef struct {
    PTP_Device_ReallocBuffer_t reallocBuffer;
    PTP_Device_FreeBuffer_t freeBuffer;
    PTP_Device_SendAndRecv_t sendAndRecv;
    PTP_Device_Reset_t reset;
    PTP_Device_ReadEvents_Func_t readEvents;
    b32 requiresSessionOpenClose;
    MAllocator* allocator;
} PTPDeviceTransport;

typedef enum {
    PTP_BACKEND_WIA,
    PTP_BACKEND_LIBUSBK,
    PTP_BACKEND_IOKIT,
    PTP_BACKEND_LIBUSB,
    PTP_BACKEND_IP,
} PTPBackendType;

// Generic device info - describing an available device
typedef struct PTPDeviceInfo {
    PTPBackendType backendType;
    MStr manufacturer;
    MStr product;
    MStr serial;
    MStr ipAddress;
    u16 usbVID;
    u16 usbPID;
    u16 usbVersion;
    void* device; // concrete backend device info - this is used to uniquely identify a connected device
} PTPDeviceInfo;

// Generic device and transport for sending commands to
typedef struct PTPDevice {
    PTPDeviceTransport transport;
    PTPLog logger;
    PTPBackendType backendType;
    b32 disconnected;
    void* device; // concrete backend device - contains backend specific device data
    PTPDeviceInfo* deviceInfo;
} PTPDevice;

typedef struct MBackendConfig {
    b32 disallowSpawnEventThread;
} MBackendConfig;

struct PTPBackend;

typedef b32 (*PTPBackend_Close_Func)(struct PTPBackend* backend);
typedef b32 (*PTPBackend_NeedsRefresh_Func)(struct PTPBackend* backend);
typedef b32 (*PTPBackend_RefreshList_Func)(struct PTPBackend* backend, PTPDeviceInfo** deviceList);
typedef b32 (*PTPBackend_PollListUpdates_Func)(struct PTPBackend* backend, PTPDeviceInfo** deviceList);
typedef void (*PTPBackend_ReleaseList_Func)(struct PTPBackend* backend);
typedef AwResult (*PTPBackend_OpenDevice_Func)(struct PTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);
typedef b32 (*PTPBackend_CloseDevice_Func)(struct PTPBackend* backend, PTPDevice* device);

// Generic backend
typedef struct PTPBackend {
    PTPBackend_Close_Func close;
    PTPBackend_NeedsRefresh_Func needsRefresh;
    PTPBackend_RefreshList_Func refreshList;
    PTPBackend_NeedsRefresh_Func isRefreshingList;
    PTPBackend_PollListUpdates_Func pollListUpdates;
    PTPBackend_ReleaseList_Func releaseList;
    PTPBackend_OpenDevice_Func openDevice;
    PTPBackend_CloseDevice_Func closeDevice;
    PTPBackendType type;
    MBackendConfig config;
    PTPLog logger;
    MAllocator* allocator;
    void* self; // Pointer to concrete backend
} PTPBackend;

PTP_EXPORT const char* PTPBackend_GetTypeAsStr(PTPBackendType type);

#ifdef __cplusplus
} // extern "C"
#endif
