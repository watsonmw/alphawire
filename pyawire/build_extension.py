#
# Alphawire Python extension build script.
# Compiles alphawire C source files directly into a Python CFFI extension.
#

import argparse
import subprocess

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

def get_script_dir():
    return os.path.dirname(os.path.abspath(__file__))


PROJECT_ROOT = None

def project_root():
    global PROJECT_ROOT
    if PROJECT_ROOT is None:
        script_dir = get_script_dir()
        if os.path.basename(script_dir) == "pyawire":
            PROJECT_ROOT = os.path.dirname(script_dir)
        else:
            PROJECT_ROOT = script_dir
        print(f"Project root: {PROJECT_ROOT}")
    return PROJECT_ROOT


def root_path(path: str) -> str:
    return os.path.join(project_root(), path)


def get_pkg_config_flags(package: str):
    try:
        cflags = subprocess.check_output(
            ["pkg-config", "--cflags-only-I", package],
            text=True
        ).strip().split()
        # Strip '-I' prefix
        inc_dirs = [flag[2:] for flag in cflags if flag.startswith("-I")]

        libs = subprocess.check_output(
            ["pkg-config", "--libs", package],
            text=True
        ).strip().split()

        return inc_dirs, libs
    except (subprocess.SubprocessError, FileNotFoundError):
        print(f"pkg-config not found, unable to locate {package} headers and libraries")
        return None, None


def setup(debug: bool=False):
    ffi_builder = cffi.FFI()

    # src/ is in the parent directory when building from within pyawire/ and in the current directory when building from
    # within sdist

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

enum AwPropertyEnumFlags {
    AwPropertyEnumFlags_KNOWN_ONLY = 1 << 0,
    AwPropertyEnumFlags_ALWAYS_STRINGIFY = 1 << 1,
};

AwResult AwControl_Init(AwControl* self, AwDevice* device, MAllocator* allocator);
AwResult AwControl_Connect(AwControl* self, AwSonyProtocolVersion version);
AwResult AwControl_Cleanup(AwControl* self);
AwResult AwControl_RefreshProperties(AwControl* self, b32 fullRefresh);

size_t AwControl_NumProperties(AwControl* self);
AwPtpProperty* AwControl_GetPropertyByIndex(AwControl* self, u16 index);
AwPtpProperty* AwControl_GetPropertyByCode(AwControl* self, u16 propertyCode);
AwPtpProperty* AwControl_GetPropertyById(AwControl* self, const char* id);
b32 AwControl_GetEnumsForProperty(AwControl* self, MAllocator* allocator, AwPtpProperty* property, i32 flags,
                                  AwPtpPropValueEnums* outEnums);
void AwControl_FreePropValueEnums(AwControl* self, AwPtpPropValueEnums* outEnums);
b32 AwControl_GetPropertyValueAsStr(AwControl* self, MAllocator* alloc, AwPtpProperty* property, MStr* strOut);
b32 AwControl_GetPropertyValueAsKnownStr(AwControl* self, MAllocator* alloc, AwPtpProperty* property, MStr* strOut);
AwResult AwControl_SetPropertyValue(AwControl* self, AwPtpProperty* property, AwPtpPropValue value);
AwResult AwControl_SetPropertyStr(AwControl* self, AwPtpProperty* property, MStr value);
b32 AwControl_IsPropertyWritable(AwControl* self, AwPtpProperty* property);
b32 AwControl_GetPropertyId(AwControl* self, AwPtpProperty* property, MStr* idOut);

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
    include_dirs = [root_path("src")]

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
        libusb_includes, libusb_libs = get_pkg_config_flags("libusb-1.0")
        if libusb_includes is not None:
            include_dirs.extend(libusb_includes)
            extra_link_args.extend(libusb_libs)
        else:
            fallback = "/usr/include/libusb-1.0"
            print(f"libusb-1.0 not found, using hardcoded fallback location for headers: {fallback}")
            include_dirs.append(fallback)
            extra_link_args.append("-lusb-1.0")
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
        include_dirs.append(root_path("libs\\libusbk"))
        extra_link_args = [root_path('libs\\libusbk\\amd64\\libusbK.lib'), 'ws2_32.lib', 'Iphlpapi.lib', 'dbghelp.lib',
                           'ole32.lib', 'wiaguid.lib', 'shell32.lib', 'Oleaut32.lib']

    all_sources = [os.path.relpath(root_path(src), get_script_dir())
                   for src in common_sources + platform_sources]
    define_macros = [
                        ("M_THREADING", None),
                        ("AW_LOG_LEVEL", "3"),
                    ] + platform_defines

    if debug:
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
#include "aw/aw.h"
""",
        sources=all_sources,
        include_dirs=include_dirs,
        define_macros=define_macros,
        extra_link_args=extra_link_args,
        extra_compile_args=extra_compile_args,
        py_limited_api=True,
    )

    return ffi_builder, all_sources

def main(build_args):
    ffi_builder, all_sources = setup(debug=build_args.debug)

    output_dir = os.path.join(get_script_dir(), "awire")
    build_flags_file = os.path.join(output_dir, "_build_flags.txt")

    if build_args.only_if_changed:
        # Check if any source or header files have changed
        watch_files = all_sources[:]
        # Add this build script itself
        watch_files.append(__file__)
        # Add headers from src/ directory
        for r, d, f in os.walk(root_path("src")):
            for file in f:
                if file.endswith(".h"):
                    watch_files.append(os.path.join(r, file))
        
        # We'll check for _binding.* in the awire directory.
        output_files = glob.glob(os.path.join(output_dir, "_binding.*"))
        # Filter out .c, .o, .obj, .pdb files if any
        output_files = [f for f in output_files if not f.endswith((".c", ".o", ".obj", ".pdb", ".txt"))]
        
        build_flags_changed = True # default to assuming it changed, False only when we can be sure it didnt
        if os.path.exists(build_flags_file):
            try:
                with open(build_flags_file, "r") as f:
                    content = f.read().strip()
                    last_debug = (content == "debug=True")
                    build_flags_changed = (last_debug != build_args.debug)
            except OSError:
                pass

        if output_files and not build_flags_changed:
            latest_src_time = get_latest_mod_time(watch_files)
            latest_out_time = get_latest_mod_time(output_files)
            
            if latest_out_time > latest_src_time:
                print("No changes detected, skipping build.")
                return

    ffi_builder.compile(verbose=True)

    # Write build flags
    try:
        with open(build_flags_file, "w") as f:
            f.write(f"debug={build_args.debug}")
    except OSError:
        pass
    print(f"Built alphawire extension")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--debug", action="store_true",
                        help="Build the C extension with debug symbols and no optimization.")
    parser.add_argument("--only-if-changed", action="store_true",
                        help="Build the C extension only if the source files have changed.")
    build_args, _ = parser.parse_known_args()
    main(build_args)
