#
# Alphawire Python extension build script.
# Compiles alphawire C source files directly into a Python CFFI extension.
#

import cffi
import os
import sys

ffi_builder = cffi.FFI()

# src/ is in the parent directory when building from within pyawire/ and in the current directory when building from
# within sdist
_script_dir = os.path.dirname(os.path.abspath(__file__))

if os.path.basename(_script_dir) == "pyawire":
    _project_root = os.path.dirname(_script_dir)
else:
    _project_root = _script_dir
print(f"Project root: {_project_root}")
ffi_builder.cdef("""
typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;
typedef int b32;

typedef void* (*M_malloc_t)(void* alloc, size_t size);
typedef void* (*M_realloc_t)(void* alloc, void* mem, size_t oldSize, size_t newSize);
typedef void (*M_free_t)(void* alloc, void* mem, size_t size);

typedef struct {
    M_malloc_t mallocFunc;
    M_realloc_t reallocFunc;
    M_free_t freeFunc;
    char* name;
    ...;
} MAllocator;

typedef struct {
    char* str;
    u32 size;
    u32 capacity;
} MStr;

typedef struct {
    u8* mem;
    u32 size;
    u32 capacity;
    MAllocator* allocator;
} MMemIO;

typedef int AwBackendType;

typedef struct {
    AwBackendType backendType;
    MStr manufacturer;
    MStr product;
    MStr serial;
    MStr ipAddress;
    u16 usbVID;
    u16 usbPID;
    u16 usbVersion;
    void* device;
} AwDeviceInfo;

typedef struct AwBackend AwBackend;

typedef struct AwDevice {
    ...;
    AwBackendType backendType;
    b32 disconnected;
    void* device;
    AwDeviceInfo* deviceInfo;
} AwDevice;

typedef struct {
    AwDeviceInfo* devices;
    AwBackend* backends;
    AwDevice* openDevices;
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    ...;
} AwDeviceList;

typedef enum {
    AW_RESULT_OK = 0,
    AW_RESULT_TIMEOUT = 1,
    AW_RESULT_UNSUPPORTED = 2,
    AW_RESULT_CONNECTION_CLOSED = 3,
    AW_RESULT_MALFORMED_RESPONSE = 4,
    AW_RESULT_TRANSPORT_ERROR = 5,
    AW_RESULT_PTP_FAILURE = 6,
    AW_RESULT_PARAM_ERROR = 7,
    AW_RESULT_NOT_SUPPORTED = 8,
    AW_RESULT_DEVICE_INFO_FAILURE = 9,
} AwResultCode;

typedef struct {
    AwResultCode code;
    int ptp;
} AwResult;

void Aw_InitDefaultAllocator(MAllocator* allocator);

b32 AwDeviceList_Open(AwDeviceList* self, MAllocator* allocator);
b32 AwDeviceList_Close(AwDeviceList* self);
b32 AwDeviceList_RefreshList(AwDeviceList* self);
b32 AwDeviceList_NeedsRefresh(AwDeviceList* self);
b32 AwDeviceList_IsRefreshingList(AwDeviceList* self);
b32 AwDeviceList_PollUpdates(AwDeviceList* self);
size_t AwDeviceList_NumDevices(AwDeviceList* self);
AwResult AwDeviceList_OpenDevice(AwDeviceList* self, AwDeviceInfo* deviceInfo, AwDevice** deviceOut);
AwResult AwDeviceList_CloseDevice(AwDeviceList* self, AwDevice* device);

typedef enum {
    SDI_EXTENSION_VERSION_200 = 200,
    SDI_EXTENSION_VERSION_300 = 300,
} AwSonyProtocolVersion;

typedef union {
    u8 u8;
    i8 i8;
    u16 u16;
    i16 i16;
    u32 u32;
    i32 i32;
    u64 u64;
    i64 i64;
    char u128[16];
    char i128[16];
    MStr str;
} AwPtpPropValue;

typedef struct {
    AwPtpPropValue min;
    AwPtpPropValue max;
    AwPtpPropValue step;
} AwPtpRange;

typedef struct {
    AwPtpPropValue* set;
    AwPtpPropValue* getSet;
} AwPtpPropertyEnum;

typedef struct {
    u16 propCode;
    u16 dataType;
    AwPtpPropValue defaultValue;
    AwPtpPropValue value;
    u8 getSet;
    u8 isEnabled;
    u8 formFlag;
    union {
        AwPtpRange range;
        AwPtpPropertyEnum enums;
    } form;
    u8 isNotch;
    ...;
} AwPtpProperty;

typedef struct {
    AwPtpPropValue propValue;
    MStr str;
    u16 flags;
} AwPtpPropValueEnum;

typedef struct {
    AwPtpPropValueEnum* values;
} AwPtpPropValueEnums;

typedef struct {
    AwPtpPropValueEnum* values;
    size_t size;
    b32 owned;
} AwPtpPropValueEnumArray;

typedef struct {
    u16 controlCode;
    u16 dataType;
    u8 controlType;
    u8 formFlag;
    char* label;
    union {
        AwPtpRange range;
        AwPtpPropValueEnumArray enums;
    } form;
} AwPtpControl;

typedef struct {
    AwDevice* device;
    u16 protocolVersion;
    MStr manufacturer;
    MStr model;
    MStr deviceVersion;
    MStr serialNumber;
    ...;
} AwControl;

typedef struct {
    u16 frameType;
    u16 focusFrameState;
    u8 priority;
    u32 x;
    u32 y;
    u32 height;
    u32 width;
} AwFocusFrame;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    AwFocusFrame* frames;
} AwFocusFrames;

typedef struct {
    u16 faceFrameType;
    u16 faceFocusFrameState;
    u16 selectionState;
    u8 priority;
    u32 xNumerator;
    u32 yNumerator;
    u32 height;
    u32 width;
} AwFocusFrameFace;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    AwFocusFrameFace* frames;
} AwFaceFrames;

typedef struct {
    u16 trackingFrameType;
    u16 trackingFrameState;
    u8 priority;
    u32 xNumerator;
    u32 yNumerator;
    u32 height;
    u32 width;
} AwFocusFrameTracking;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    AwFocusFrameTracking* frames;
} AwTrackingFrames;

typedef struct {
    u16 version;
    AwFocusFrames focus;
    AwFaceFrames face;
    AwTrackingFrames tracking;
} AwLiveViewFrames;

typedef struct {
    MStr filename;
    int objectFormat;
    size_t size;
} AwPtpCapturedImageInfo;

AwResult AwControl_Init(AwControl* self, AwDevice* device, MAllocator* allocator);
AwResult AwControl_Connect(AwControl* self, AwSonyProtocolVersion version);
AwResult AwControl_Cleanup(AwControl* self);
AwResult AwControl_UpdateProperties(AwControl* self, b32 fullRefresh);

size_t AwControl_NumProperties(AwControl* self);
AwPtpProperty* AwControl_GetPropertyByIndex(AwControl* self, u16 index);
AwPtpProperty* AwControl_GetPropertyByCode(AwControl* self, u16 propertyCode);
AwPtpProperty* AwControl_GetPropertyById(AwControl* self, const char* id);
b32 AwControl_GetPropertyValueAsStr(AwControl* self, AwPtpProperty* property, MAllocator* alloc, MStr* strOut);
AwResult AwControl_SetPropertyValue(AwControl* self, AwPtpProperty* property, AwPtpPropValue value);
AwResult AwControl_SetPropertyStr(AwControl* self, AwPtpProperty* property, MStr value);
b32 AwControl_IsPropertyWritable(AwControl* self, AwPtpProperty* property);

size_t AwControl_NumControls(AwControl* self);
AwPtpControl* AwControl_GetControlByIndex(AwControl* self, u16 index);
AwPtpControl* AwControl_GetControlByCode(AwControl* self, u16 controlCode);
AwResult AwControl_SetControlValue(AwControl* self, u16 controlCode, AwPtpPropValue value);
AwResult AwControl_SetControlToggle(AwControl* self, u16 controlCode, b32 pressed);

AwResult AwControl_GetLiveViewImage(AwControl* self, MMemIO* outFile, AwLiveViewFrames* outLiveViewFrames);
void AwControl_FreeLiveViewFrames(AwControl* self, AwLiveViewFrames* liveViewFrames);

int AwControl_GetPendingFiles(AwControl* self);
AwResult AwControl_GetCapturedImage(AwControl* self, MMemIO* outFile, AwPtpCapturedImageInfo* outCii);

char* AwGetPropertyLabel(u16 propCode);
char* AwGetControlLabel(u16 controlCode);
char* AwGetEventLabel(u16 eventCode);
char* AwGetOperationLabel(u16 operationCode);

void Aw_StrFree(MAllocator* allocator, MStr* str);
void Aw_MemIOFree(MMemIO* memIO);
""")

src_dir = os.path.join(_project_root, "src")
common_sources = [
    "src/mlib/mlib.c",
    "src/mlib/mlib-file-stdlib.c",
    "src/mlib/mlib-log-stdlib.c",
    "src/mlib/utf8.c",
    "src/aw/aw-backend.c",
    "src/aw/aw-control.c",
    "src/aw/aw-device-list.c",
    "src/aw/aw-log.c",
    "src/aw/aw-util.c",
    "src/aw/platform/usb-const.c",
]

ip_sources = [
    "src/mlib/msock.c",
    "src/mlib/mxml.c",
    "src/aw/platform/ip/http-client.c",
    "src/aw/platform/ip/aw-backend-ip.c",
]

if sys.platform == "darwin":
    platform_sources = ["src/aw/platform/osx/aw-backend-iokit.c"] + ip_sources
    platform_defines = [
        ("AW_ENABLE_IOKIT", None),
        ("AW_ENABLE_IP", None),
        ("M_PTHREADS", None),
    ]
    extra_link_args = ["-framework", "IOKit", "-framework", "CoreFoundation"]
elif sys.platform.startswith("linux"):
    platform_sources = ["src/aw/platform/libusb/aw-backend-libusb.c"] + ip_sources
    platform_defines = [
        ("AW_ENABLE_LIBUSB", None),
        ("AW_ENABLE_IP", None),
        ("M_PTHREADS", None),
    ]
    extra_link_args = ["-lusb-1.0"]
elif sys.platform.startswith("win32"):
    platform_sources = [
        "src/aw/platform/windows/aw-backend-libusbk.c",
        "src/aw/platform/windows/aw-backend-wia.c"
        "src/aw/platform/windows/win-utils.c"
    ] + ip_sources
    platform_defines = [
        ("WINVER", "0x0A00"),
        ("_WIN32_WINNT", "0x0600"),
        ("AW_ENABLE_LIBUSBK", None),
        ("AW_ENABLE_WIA", None),
        ("AW_ENABLE_IP", None),
    ]
else:
    platform_sources = []
    platform_defines = []
    extra_link_args = []

all_sources = [os.path.relpath(os.path.join(_project_root, src), _script_dir)
               for src in common_sources + platform_sources]
define_macros = [
                    ("ALPHAWIRE_BUILDING_SHARED_LIB", None),
                    ("AW_LOG_LEVEL", "3"),
                ] + platform_defines

ffi_builder.set_source(
    "awire._binding",
    """
#include "aw/aw-const.h"
#include "aw/aw-control.h"
#include "aw/aw-device-list.h"
#include "aw/aw-util.h"
""",
    sources=all_sources,
    include_dirs=[src_dir],
    define_macros=define_macros,
    extra_link_args=extra_link_args,
    extra_compile_args=["-fvisibility=hidden"],
    py_limited_api=True,
)

if __name__ == "__main__":
    ffi_builder.compile(verbose=True)
    print(f"Built alphawire extension")
