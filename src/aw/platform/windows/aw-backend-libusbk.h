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

typedef struct AwUsbkBackend AwUsbkBackend;

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
    AwPtpEventArray eventList; // MArray of stored events
    AwUsbkBackend* backend;
} PTPUsbkDeviceUsbk;

struct AwUsbkBackend {
    MArray(UsbkDeviceInfo) deviceList;
    KLIB_VERSION libkVersion;
    MArray(PTPUsbkDeviceUsbk) openDevices;
    void* deviceListHandle; // USBK Device list handle
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    AwLog logger;
    struct AwBackend* backend; // Reference to parent backend

    HMODULE hLib;
    struct {
        VOID (KUSB_API *LibK_GetVersion)(PKLIB_VERSION Version);
        BOOL (KUSB_API *LstK_Init)(KLST_HANDLE* DeviceList, KLST_FLAG Flags);
        BOOL (KUSB_API *LstK_Free)(KLST_HANDLE DeviceList);
        BOOL (KUSB_API *LstK_MoveNext)(KLST_HANDLE DeviceList, KLST_DEVINFO_HANDLE* DeviceInfo);
        BOOL (KUSB_API *UsbK_Init)(KUSB_HANDLE* InterfaceHandle, KLST_DEVINFO_HANDLE DevInfo);
        BOOL (KUSB_API *UsbK_Free)(KUSB_HANDLE InterfaceHandle);
        BOOL (KUSB_API *UsbK_GetDescriptor)(KUSB_HANDLE InterfaceHandle, UCHAR DescriptorType, UCHAR Index, USHORT LanguageID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred);
        BOOL (KUSB_API *UsbK_GetOverlappedResult)(KUSB_HANDLE InterfaceHandle, LPOVERLAPPED Overlapped, PUINT lpNumberOfBytesTransferred, BOOL bWait);
        BOOL (KUSB_API *UsbK_WritePipe)(KUSB_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
        BOOL (KUSB_API *UsbK_ReadPipe)(KUSB_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, UINT BufferLength, PUINT LengthTransferred, LPOVERLAPPED Overlapped);
        BOOL (KUSB_API *UsbK_AbortPipe)(KUSB_HANDLE InterfaceHandle, UCHAR PipeID);
        BOOL (KUSB_API *UsbK_ResetDevice)(KUSB_HANDLE InterfaceHandle);
    } libk;
};

AW_EXPORT AwResult AwUsbkDeviceList_OpenBackend(AwBackend* backend, u32 timeoutMilliseconds);
AW_EXPORT AwResult AwUsbkDeviceList_Open(AwUsbkBackend* self);
AW_EXPORT AwResult AwUsbkDeviceList_Close(AwUsbkBackend* self);
AW_EXPORT AwResult AwUsbkDeviceList_RefreshList(AwUsbkBackend* self, AwDeviceInfoArray* devices);
AW_EXPORT AwResult AwUsbkDeviceList_ReleaseList(AwUsbkBackend* self);
AW_EXPORT AwResult AwUsbkDeviceList_OpenDevice(AwUsbkBackend* self, AwDeviceInfo* deviceId, AwDevice** deviceOut);
AW_EXPORT AwResult AwUsbkDeviceList_CloseDevice(AwUsbkBackend* self, AwDevice* device);

#ifdef __cplusplus
} // extern "C"
#endif
