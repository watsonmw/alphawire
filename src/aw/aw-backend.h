#pragma once

#include "aw/aw-const.h"
#include "aw/aw-log.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AwDevice;

typedef void* (*AwDevice_ReallocBuffer_Func)(struct AwDevice* device, AwBufferType type, void* dataMem,
    size_t dataOldSize, size_t dataNewSize);
typedef void (*AwDevice_FreeBuffer_Func)(struct AwDevice* device, AwBufferType type, void* dataMem, size_t dataOldSize);
typedef AwResult (*AwDevice_SendAndRecv_Func)(struct AwDevice* device, AwPtpRequestHeader* request, u8* dataIn,
    size_t dataInSize, AwPtpResponseHeader* response, u8* dataOut, size_t dataOutSize, size_t* actualDataOutSize);
typedef b32 (*AwDevice_Reset_Func)(struct AwDevice* device);
typedef AwResult (*AwDevice_ReadEvents_Func)(struct AwDevice* device, int timeoutMilliseconds, MAllocator* alloc,
                                             AwPtpEvent** outEventList);

// Device transport for communicating with a device
typedef struct {
    AwDevice_ReallocBuffer_Func reallocBuffer;
    AwDevice_FreeBuffer_Func freeBuffer;
    AwDevice_SendAndRecv_Func sendAndRecv;
    AwDevice_Reset_Func reset;
    AwDevice_ReadEvents_Func readEvents;
    b32 requiresSessionOpenClose;
    MAllocator* allocator;
} AwDeviceTransport;

typedef enum {
    AW_BACKEND_WIA,
    AW_BACKEND_LIBUSBK,
    AW_BACKEND_IOKIT,
    AW_BACKEND_LIBUSB,
    AW_BACKEND_IP,
} AwBackendType;

// Generic device info - describing an available device
typedef struct AwDeviceInfo {
    AwBackendType backendType;
    MStr manufacturer;
    MStr product;
    MStr serial;
    MStr ipAddress;
    u16 usbVID;
    u16 usbPID;
    u16 usbVersion;
    void* device; // concrete backend device info - this is used to uniquely identify a connected device
} AwDeviceInfo;

// Generic device and transport for sending commands to
typedef struct AwDevice {
    AwDeviceTransport transport;
    AwLog logger;
    AwBackendType backendType;
    b32 disconnected;
    void* device; // concrete backend device - contains backend specific device data
    AwDeviceInfo* deviceInfo;
} AwDevice;

typedef struct MBackendConfig {
    b32 disallowSpawnEventThread;
} AwBackendConfig;

struct AwBackend;

typedef AwResult (*AwBackend_Close_Func)(struct AwBackend* backend);
typedef b32 (*AwBackend_NeedsRefresh_Func)(struct AwBackend* backend);
typedef AwResult (*AwBackend_RefreshList_Func)(struct AwBackend* backend, AwDeviceInfo** deviceList);
typedef b32 (*AwBackend_IsRefreshingList_Func)(struct AwBackend* backend);
typedef b32 (*AwBackend_PollListUpdates_Func)(struct AwBackend* backend, AwDeviceInfo** deviceList);
typedef AwResult (*AwBackend_ReleaseList_Func)(struct AwBackend* backend);
typedef AwResult (*AwBackend_OpenDevice_Func)(struct AwBackend* backend, AwDeviceInfo* deviceInfo, AwDevice** deviceOut);
typedef AwResult (*AwBackend_CloseDevice_Func)(struct AwBackend* backend, AwDevice* device);

// Generic backend
typedef struct AwBackend {
    AwBackend_Close_Func close;
    AwBackend_NeedsRefresh_Func needsRefresh;
    AwBackend_RefreshList_Func refreshList;
    AwBackend_IsRefreshingList_Func isRefreshingList;
    AwBackend_PollListUpdates_Func pollListUpdates;
    AwBackend_ReleaseList_Func releaseList;
    AwBackend_OpenDevice_Func openDevice;
    AwBackend_CloseDevice_Func closeDevice;
    AwBackendType type;
    AwBackendConfig config;
    AwLog logger;
    MAllocator* allocator;
    void* self; // Pointer to concrete backend
} AwBackend;

AW_EXPORT const char* AwBackend_GetTypeAsStr(AwBackendType type);

#ifdef __cplusplus
} // extern "C"
#endif
