#
# Alphawire Python extension build script.
# Compiles alphawire C source files directly into a Python CFFI extension.
#

import argparse
import cffi
import os
import sys
import glob


def get_latest_mod_time(files):
    latest = 0
    for f in files:
        if os.path.exists(f):
            latest = max(latest, os.path.getmtime(f))
    return latest


def main(build_args):
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

typedef enum {
    AW_LOG_LEVEL_TRACE = 4,
    AW_LOG_LEVEL_DEBUG = 3,
    AW_LOG_LEVEL_INFO = 2,
    AW_LOG_LEVEL_WARNING = 1,
    AW_LOG_LEVEL_ERROR = 0
} AwLogLevel;

struct AwLog;
typedef void (*AwPLog_Log_Func)(struct AwLog* logger, AwLogLevel level, const char *message);

typedef struct AwLog {
    AwLogLevel level;
    AwPLog_Log_Func logFunc;
    void* userData;
    char msgBuffer[1024];
} AwLog;

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
    b32 disallowSpawnEventThread;
} AwBackendConfig;

typedef struct {
    struct { size_t size; size_t capacity; AwDeviceInfo* data; } devices;
    struct { size_t size; size_t capacity; AwBackend* data; } backends;
    struct { size_t size; size_t capacity; AwDevice* data; } openDevices;
    u32 timeoutMilliseconds;
    AwBackendConfig backendConfig;
    MAllocator* allocator;
    AwLog logger;
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

typedef enum {
    AW_DIAL_MODE_CAMERA = 0x00,
    AW_DIAL_MODE_REMOTE = 0x01,
} AwDialMode;

typedef enum {
    AW_EXPOSURE_PROGRAM_MANUAL = 0x0001,
    AW_EXPOSURE_PROGRAM_AUTOMATIC = 0x0002,
    AW_EXPOSURE_PROGRAM_APERTURE_PRIORITY = 0x0003,
    AW_EXPOSURE_PROGRAM_SHUTTER_PRIORITY = 0x0004,
    AW_EXPOSURE_PROGRAM_PROGRAM_CREATIVE = 0x0005,
    AW_EXPOSURE_PROGRAM_PROGRAM_ACTION = 0x0006,
    AW_EXPOSURE_PROGRAM_PORTRAIT = 0x0007,
    AW_EXPOSURE_PROGRAM_AUTO = 0x8000,
    AW_EXPOSURE_PROGRAM_AUTO_PLUS = 0x8001,
    AW_EXPOSURE_PROGRAM_P_A = 0x8008,
    AW_EXPOSURE_PROGRAM_P_S = 0x8009,
    AW_EXPOSURE_PROGRAM_SPORTS_ACTION = 0x8011,
    AW_EXPOSURE_PROGRAM_SUNSET = 0x8012,
    AW_EXPOSURE_PROGRAM_NIGHT_SCENE = 0x8013,
    AW_EXPOSURE_PROGRAM_LANDSCAPE = 0x8014,
    AW_EXPOSURE_PROGRAM_MACRO = 0x8015,
    AW_EXPOSURE_PROGRAM_HAND_HELD_TWILIGHT = 0x8016,
    AW_EXPOSURE_PROGRAM_NIGHT_PORTRAIT = 0x8017,
    AW_EXPOSURE_PROGRAM_ANTI_MOTION_BLUR = 0x8018,
    AW_EXPOSURE_PROGRAM_PET = 0x8019,
    AW_EXPOSURE_PROGRAM_GOURMET = 0x801A,
    AW_EXPOSURE_PROGRAM_FIREWORKS = 0x801B,
    AW_EXPOSURE_PROGRAM_HIGH_SENSITIVITY = 0x801C,
    AW_EXPOSURE_PROGRAM_MEMORY_RECALL = 0x8020,
    AW_EXPOSURE_PROGRAM_MOVIE_P = 0x8050,
    AW_EXPOSURE_PROGRAM_MOVIE_A = 0x8051,
    AW_EXPOSURE_PROGRAM_MOVIE_S = 0x8052,
    AW_EXPOSURE_PROGRAM_MOVIE_M = 0x8053,
    AW_EXPOSURE_PROGRAM_MOVIE_AUTO = 0x8054,
} AwExposureProgramMode;

typedef enum {
    AW_CAPTURE_MODE_NORMAL = 0x0001,
    AW_CAPTURE_MODE_CONTINUOUS_HI = 0x0002,
    AW_CAPTURE_MODE_TIMELAPSE = 0x0003,
    AW_CAPTURE_MODE_SELF_TIMER_5S = 0x8003,
    AW_CAPTURE_MODE_SELF_TIMER_10S = 0x8004,
    AW_CAPTURE_MODE_SELF_TIMER_2S = 0x8005,
    AW_CAPTURE_MODE_CONTINUOUS_HI_PLUS = 0x8010,
    AW_CAPTURE_MODE_CONTINUOUS_LO = 0x8012,
    AW_CAPTURE_MODE_CONTINUOUS_MID = 0x8015,
} AwCaptureMode;

typedef enum {
    AW_WHITE_BALANCE_MANUAL = 0x0001,
    AW_WHITE_BALANCE_AWB = 0x0002,
    AW_WHITE_BALANCE_ONE_PUSH_AUTO = 0x0003,
    AW_WHITE_BALANCE_DAYLIGHT = 0x0004,
    AW_WHITE_BALANCE_FLUORESCENT = 0x0005,
    AW_WHITE_BALANCE_TUNGSTEN = 0x0006,
    AW_WHITE_BALANCE_FLASH = 0x0007,
    AW_WHITE_BALANCE_CLOUDY = 0x8010,
    AW_WHITE_BALANCE_SHADE = 0x8011,
    AW_WHITE_BALANCE_CUSTOM_TEMP = 0x8012,
    AW_WHITE_BALANCE_CUSTOM_1 = 0x8020,
    AW_WHITE_BALANCE_CUSTOM_2 = 0x8021,
    AW_WHITE_BALANCE_CUSTOM_3 = 0x8022,
    AW_WHITE_BALANCE_CUSTOM = 0x8023,
} AwWhiteBalance;

typedef enum {
    AW_FOCUS_MODE_MANUAL = 0x0001,
    AW_FOCUS_MODE_AF_S = 0x0002,
    AW_FOCUS_MODE_AF_C = 0x8004,
    AW_FOCUS_MODE_AF_AUTO = 0x8005,
    AW_FOCUS_MODE_DMF = 0x8006,
} AwFocusMode;

typedef enum {
    AW_FOCUS_AREA_WIDE = 0x0001,
    AW_FOCUS_AREA_ZONE = 0x0002,
    AW_FOCUS_AREA_CENTER = 0x0003,
    AW_FOCUS_AREA_FLEXIBLE_SPOT_S = 0x0101,
    AW_FOCUS_AREA_FLEXIBLE_SPOT_M = 0x0102,
    AW_FOCUS_AREA_FLEXIBLE_SPOT_L = 0x0103,
    AW_FOCUS_AREA_EXPAND_FLEXIBLE_SPOT = 0x0104,
    AW_FOCUS_AREA_TRACKING_WIDE = 0x0201,
    AW_FOCUS_AREA_TRACKING_ZONE = 0x0202,
    AW_FOCUS_AREA_TRACKING_CENTER = 0x0203,
    AW_FOCUS_AREA_TRACKING_FLEXIBLE_SPOT_S = 0x0204,
    AW_FOCUS_AREA_TRACKING_FLEXIBLE_SPOT_M = 0x0205,
    AW_FOCUS_AREA_TRACKING_FLEXIBLE_SPOT_L = 0x0206,
    AW_FOCUS_AREA_TRACKING_EXPAND_FLEXIBLE_SPOT = 0x0207,
} AwFocusArea;

typedef enum {
    AW_AUTO_FOCUS_STATUS_UNLOCK = 0x01,
    AW_AUTO_FOCUS_STATUS_AFS_LOCKED = 0x02,
    AW_AUTO_FOCUS_STATUS_AFS_FAILED = 0x03,
    AW_AUTO_FOCUS_STATUS_AFC_TRACKING = 0x05,
    AW_AUTO_FOCUS_STATUS_AFC_FOCUSED = 0x06,
    AW_AUTO_FOCUS_STATUS_AFC_FAILED = 0x07,
} AwAutoFocusStatus;

typedef enum {
    AW_ASPECT_RATIO_3_2 = 0x01,
    AW_ASPECT_RATIO_16_9 = 0x02,
    AW_ASPECT_RATIO_4_3 = 0x03,
    AW_ASPECT_RATIO_1_1 = 0x04,
} AwAspectRatio;

typedef enum {
    AW_SHUTTER_TYPE_AUTO = 0x01,
    AW_SHUTTER_TYPE_MECHANICAL = 0x02,
    AW_SHUTTER_TYPE_ELECTRONIC = 0x03,
} AwShutterType;

typedef enum {
    AW_SILENT_MODE_OFF = 0x01,
    AW_SILENT_MODE_ON = 0x02,
} AwSilentMode;

typedef enum {
    AW_ISO_AUTO = 0x00ffffff,
} AwIso;

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
    struct { size_t size; size_t capacity; AwPtpPropValue* data; } set;
    struct { size_t size; size_t capacity; AwPtpPropValue* data; } getSet;
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
    struct { size_t size; size_t capacity; AwPtpPropValueEnum* data; } values;
} AwPtpPropValueEnums;

typedef struct {
    struct { size_t size; size_t capacity; AwPtpPropValueEnum* data; } values;
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
    MAllocator* allocator;
    AwLog logger;
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
    struct { size_t size; size_t capacity; AwFocusFrame* data; } frames;
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
    struct { size_t size; size_t capacity; AwFocusFrameFace* data; } frames;
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
    struct { size_t size; size_t capacity; AwFocusFrameTracking* data; } frames;
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
b32 AwControl_GetEnumsForProperty(AwControl* self, MAllocator* allocator, AwPtpProperty* property, AwPtpPropValueEnums* outEnums);
void AwControl_FreePropValueEnums(AwControl* self, AwPtpPropValueEnums* outEnums);
b32 AwControl_GetPropertyValueAsStr(AwControl* self, MAllocator* alloc, AwPtpProperty* property, MStr* strOut);
b32 AwControl_GetPropertyValueAsKnownStr(AwControl* self, MAllocator* alloc, AwPtpProperty* property, MStr* strOut);
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

void AwLog_Log(AwLog* logger, AwLogLevel level, const char *fmt, ...);
void AwLog_LogDefault(AwLog* logger, AwLogLevel level, const char *message);

char* AwGetPropertyLabel(u16 propCode);
char* AwGetControlLabel(u16 controlCode);
char* AwGetEventLabel(u16 eventCode);
char* AwGetOperationLabel(u16 operationCode);

void Aw_StrFree(MAllocator* allocator, MStr* str);
void Aw_MemIOFree(MMemIO* memIO);
""")

    def root(path: str) -> str:
        return os.path.join(_project_root, path)

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

    platform_sources = []
    platform_defines = []
    extra_link_args = []
    extra_compile_args = []
    include_dirs = [root("src")]

    if sys.platform == "darwin":
        platform_sources = ["src/aw/platform/osx/aw-backend-iokit.c"] + ip_sources
        platform_defines = [
            ("AW_ENABLE_IOKIT", None),
            ("AW_ENABLE_IP", None),
            ("M_PTHREADS", None),
        ]
        extra_link_args = ["-framework", "IOKit", "-framework", "CoreFoundation"]
        extra_compile_args = ["-fvisibility=hidden"]
    elif sys.platform.startswith("linux"):
        platform_sources = ["src/aw/platform/libusb/aw-backend-libusb.c"] + ip_sources
        platform_defines = [
            ("AW_ENABLE_LIBUSB", None),
            ("AW_ENABLE_IP", None),
            ("M_PTHREADS", None),
        ]
        extra_link_args = ["-lusb-1.0"]
        extra_compile_args = ["-fvisibility=hidden"]
    elif sys.platform.startswith("win32"):
        platform_sources = [
            "src/aw/platform/windows/aw-backend-libusbk.c",
            "src/aw/platform/windows/aw-backend-wia.c",
            "src/aw/platform/windows/win-utils.c"
        ] + ip_sources
        platform_defines = [
            ("WINVER", "0x0A00"),
            ("_WIN32_WINNT", "0x0600"),
            ("AW_ENABLE_LIBUSBK", None),
            ("AW_ENABLE_WIA", None),
            ("AW_ENABLE_IP", None),
        ]
        include_dirs.append(root("libs\\libusbk"))
        extra_link_args = [root('libs\\libusbk\\amd64\\libusbK.lib'), 'ws2_32.lib', 'Iphlpapi.lib', 'dbghelp.lib',
                           'ole32.lib', 'wiaguid.lib', 'shell32.lib', 'Oleaut32.lib']

    all_sources = [os.path.relpath(os.path.join(_project_root, src), _script_dir)
                   for src in common_sources + platform_sources]
    define_macros = [
                        ("ALPHAWIRE_BUILDING_SHARED_LIB", None),
                        ("AW_LOG_LEVEL", "3"),
                    ] + platform_defines

    if build_args.debug:
        platform_defines.append(("_DEBUG", None))
        if sys.platform.startswith("win32"):
            # pdb_path = os.path.join(_script_dir, "awire", "_binding.pdb")
            # print(f"{pdb_path}")
            # extra_compile_args += ["/Zi", "/Od", "/FS"]
            extra_compile_args += ["/Zi", "/Od"]
            extra_link_args += ["/DEBUG:FULL"]
        else:
            extra_compile_args += ["-g", "-O0"]

    ffi_builder.set_source(
        "awire._binding",
        """
#include "aw/aw-const.h"
#include "aw/aw-control.h"
#include "aw/aw-device-list.h"
#include "aw/aw-util.h"
""",
        sources=all_sources,
        include_dirs=include_dirs,
        define_macros=define_macros,
        extra_link_args=extra_link_args,
        extra_compile_args=extra_compile_args,
        py_limited_api=True,
    )
    if build_args.only_if_changed:
        # Check if any source or header files have changed
        watch_files = all_sources[:]
        # Add headers from src/ directory
        for r, d, f in os.walk(root("src")):
            for file in f:
                if file.endswith(".h"):
                    watch_files.append(os.path.join(r, file))
        
        # We'll check for _binding.* in the awire directory.
        output_dir = os.path.join(_script_dir, "awire")
        output_files = glob.glob(os.path.join(output_dir, "_binding.*"))
        # Filter out .c, .o, .obj, .pdb files if any
        output_files = [f for f in output_files if not f.endswith((".c", ".o", ".obj", ".pdb", ".txt"))]
        
        if output_files:
            latest_src_time = get_latest_mod_time(watch_files)
            latest_out_time = get_latest_mod_time(output_files)
            
            if latest_out_time > latest_src_time:
                print("No changes detected, skipping build.")
                return

    ffi_builder.compile(verbose=True)
    print(f"Built alphawire extension")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--debug", action="store_true",
                        help="Build the C extension with debug symbols and no optimization.")
    parser.add_argument("--only-if-changed", action="store_true",
                        help="Build the C extension only if the source files have changed.")
    build_args, _ = parser.parse_known_args()
    main(build_args)
