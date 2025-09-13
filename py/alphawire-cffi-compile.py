import cffi
import os
import sys

ffi = cffi.FFI()

# Declare only what you need. Keep complex structs opaque for ease of wrapping.
ffi.cdef("""
typedef char i8;
typedef unsigned char u8;
typedef short i16; 
typedef unsigned short u16; 
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;
typedef unsigned int b32;

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
} MStr;

typedef struct {
    u8* mem;
    u32 size;
    u32 capacity;
    MAllocator* allocator;
} MMemIO;

typedef int PTPBackendType;

typedef struct {
    PTPBackendType backendType;
    MStr manufacturer;
    MStr product;
    MStr serial;
    u16 usbVID;
    u16 usbPID;
    u16 usbVersion;
    void* device;
} PTPDeviceInfo;

typedef struct PTPBackend PTPBackend;

typedef struct PTPDevice {
    ...;
    PTPBackendType backendType;
    b32 disconnected;
    void* device; // concrete backend device - contains backend specific device data
    PTPDeviceInfo* deviceInfo;
} PTPDevice;

typedef struct {
    PTPDeviceInfo* devices;
    PTPBackend* backends;
    PTPDevice* openDevices;
    u32 timeoutMilliseconds;
    MAllocator* allocator;
    ...;
} PTPDeviceList;

void PTP_InitDefaultAllocator(MAllocator* allocator);

b32 PTPDeviceList_Open(PTPDeviceList* self, MAllocator* allocator);
b32 PTPDeviceList_Close(PTPDeviceList* self);
b32 PTPDeviceList_RefreshList(PTPDeviceList* self);
b32 PTPDeviceList_NeedsRefresh(PTPDeviceList* self);
size_t PTPDeviceList_NumDevices(PTPDeviceList* self);
b32 PTPDeviceList_OpenDevice(PTPDeviceList* self, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);

PTPBackend* PTPDeviceList_GetBackend(PTPDeviceList* self, PTPBackendType backend);

typedef enum {
    SDI_EXTENSION_VERSION_200 = 200,
    SDI_EXTENSION_VERSION_300 = 300,
} SonyProtocolVersion;

typedef struct {
    PTPDevice* device;
    u16 protocolVersion;
    MStr manufacturer;
    MStr model;
    MStr deviceVersion;
    MStr serialNumber;
    u32 vendorExtensionId;
    i32 vendorExtensionVersion;
    MStr vendorExtension;
    u16* supportedEvents;
    u16* supportedControls;
    u16* supportedOperations;
    u16* supportedProperties;
    u16 standardVersion;
    u16* captureFormats;
    u16* imageFormats;
    ...;
} PTPControl;

typedef enum {
    PTP_OK = 0x2001,
    PTP_GENERAL_ERROR = 0x2002,
    PTP_SESSION_NOT_OPEN = 0x2003,
    PTP_INVALID_TRANSACTION_ID = 0x2004,
    PTP_OP_NOT_SUPPORTED = 0x2005,
    PTP_PARAM_NOT_SUPPORTED = 0x2006,
    PTP_INCOMPLETE_TRANSFER = 0x2007,
    PTP_INVALID_STORAGE_ID = 0x2008,
    PTP_INVALID_OBJECT_HANDLE = 0x2009,
    PTP_PROP_NOT_SUPPORTED = 0x200A,
    PTP_STORE_FULL = 0x200C,
    PTP_OBJECT_WRITE_PROTECTED = 0x200C,
    PTP_STORE_READ_ONLY = 0x200E,
    PTP_ACCESS_DENIED = 0x200F,
    PTP_NO_THUMBNAIL_PRESENT = 0x2010,
    PTP_SELF_TEST_FAILED = 0x2011,
    PTP_PARTIAL_DELETION = 0x2012,
    PTP_STORE_NOT_AVAILABLE = 0x2013,
    PTP_SPEC_BY_FORMAT_UNSUPPORTED = 0x2014,
    PTP_NO_VALID_OBJECT_INFO = 0x2015,
    PTP_INVALID_CODE_FORMAT = 0x2016,
    PTP_UNKNOWN_VENDOR_CODE = 0x2017,
    PTP_CAPTURE_ALREADY_TERMINATED = 0x2018,
    PTP_DEVICE_BUSY = 0x2019,
    PTP_INVALID_PARENT_OBJECT = 0x201A,
    PTP_INVALID_DEVICE_PROP_FORMAT = 0x201B,
    PTP_INVALID_DEVICE_PROP_VALUE = 0x201C,
    PTP_INVALID_PARAMETER = 0x201D,
    PTP_SESSION_ALREADY_OPEN = 0x201E,
    PTP_TRANSACTION_CANCELLED = 0x201F,
    PTP_SPEC_DEST_UNSUPPORTED = 0x2020,

    PTP_SD_AUTH_FAILED = 0xA101,
    PTP_SD_PASSWD_TOO_LONG = 0xA102,
    PTP_SD_PASSWD_INVALID_CHAR = 0xA103,
    PTP_SD_FEATURE_VERSION_INVALID = 0xA104,
    PTP_SD_TEMP_STORAGE_FULL = 0xA105,
    PTP_SD_CAMERA_STATUS_ERR = 0xA106
} PTPResult;

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
} PTPPropValue;

typedef struct {
    PTPPropValue min;
    PTPPropValue max;
    PTPPropValue step;
} PTPRange;

typedef struct {
    PTPPropValue* set;
    PTPPropValue* getSet;
} PTPPropertyEnum;

typedef struct {
    u16 propCode;
    u16 dataType;
    PTPPropValue defaultValue;
    PTPPropValue value;
    u8 getSet;
    u8 isEnabled;
    u8 formFlag;
    u8 isNotch; // Property can only be changed by 'notching' - needed for some properties

    union {
        PTPRange range;
        PTPPropertyEnum enums;
    } form;
} PTPProperty;

typedef struct {
    PTPPropValue propValue;
    MStr str;
    u16 flags;
} PTPPropValueEnum;

typedef struct {
    PTPPropValueEnum* values;
} PTPPropValueEnums;

typedef struct {
    PTPPropValueEnum* values;
    size_t size;
    b32 owned;
} PTPPropValueEnumArray;

typedef struct {
    u16 controlCode;
    u16 dataType;
    u8 controlType;
    u8 formFlag;
    char* name;

    union {
        PTPRange range;
        PTPPropValueEnumArray enums;
    } form;
} PtpControl;

typedef struct {
    u16 frameType; // SD_FocusFrameType
    u16 focusFrameState; // SD_FocusFrameState
    u8 priority;
    u32 height;
    u32 width;
} FocusFrame;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    FocusFrame* frames;
} FocusFrames;

typedef struct {
    u16 faceFrameType; // SD_FaceFrameType
    u16 faceFocusFrameState; // SD_FaceFrameState
    u16 selectionState; // SD_SelectionState
    u8 priority;
    u32 xNumerator;
    u32 yNumerator;
    u32 height;
    u32 width;
} FocusFrameFace;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    FocusFrameFace* frames;
} FaceFrames;

typedef struct {
    u16 trackingFrameType; // SD_TrackingFrameType
    u16 trackingFrameState; // SD_TrackingFrameState
    u8 priority;
    u32 xNumerator;
    u32 yNumerator;
    u32 height;
    u32 width;
} FocusFrameTracking;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    FocusFrameTracking* frames;
} TrackingFrames;

typedef struct {
    u16 version;
    FocusFrames focus;
    FaceFrames face;
    TrackingFrames tracking;
} LiveViewFrames;

PTPResult PTPControl_Init(PTPControl* self, PTPDevice* device, MAllocator* allocator);
PTPResult PTPControl_Connect(PTPControl* self, SonyProtocolVersion version);
PTPResult PTPControl_Cleanup(PTPControl* self);
PTPResult PTPControl_UpdateProperties(PTPControl* self);
size_t PTPControl_NumProperties(PTPControl* self);
PTPProperty* PTPControl_GetPropertyAtIndex(PTPControl* self, u16 index);
PTPProperty* PTPControl_GetProperty(PTPControl* self, u16 propertyCode);
b32 PTPControl_GetPropertyAsStr(PTPControl* self, u16 propertyCode, MStr* strOut);
PTPResult PTPControl_GetLiveViewImage(PTPControl* self, MMemIO* fileOut, LiveViewFrames* liveViewFramesOut);
void PTP_FreeLiveViewFrames(MAllocator* mem, LiveViewFrames* liveViewFrames);

size_t PTPControl_NumControls(PTPControl* self);
PtpControl* PTPControl_GetControlAtIndex(PTPControl* self, u16 index);

char* PTP_GetOperationStr(u16 operationCode);
char* PTP_GetControlStr(u16 controlCode);
char* PTP_GetEventStr(u16 eventCode);
char* PTP_GetPropertyStr(u16 propCode);

void PTP_MemIOFree(MMemIO* memIO);

""")

lib_source_code = """
#define M_USE_SDL
#define M_USE_STDLIB
#define M_ASSERT
#define M_STACKTRACE
#define M_MEM_DEBUG
#define ALPHAWIRE_BUILDING_SHARED_LIB

#include "platform/usb-const.h"
#include "ptp/ptp-const.h"
#include "ptp/ptp-control.h"
#include "ptp/ptp-device-list.h"
#include "ptp/ptp-log.h"
#include "ptp/ptp-util.h"
"""

if __name__ == "__main__":
    # Platform specific rpath settings (for debugging)
    if sys.platform == 'darwin':
        extra_link_args = ['-Wl,-rpath,@loader_path']
    elif sys.platform.startswith('linux'):
        extra_link_args = ['-Wl,-rpath,$ORIGIN']
    else:
        extra_link_args = []

    # Compile & link to <project>/build directory
    project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = project_dir + "/build"
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    src_dir = project_dir + "/src"

    ffi.set_source(module_name="_alphawire_cffi",
                   source=lib_source_code,
                   extra_link_args=extra_link_args,
                   include_dirs=[src_dir],   # Add include path
                   library_dirs=[build_dir], # Add library path
                   libraries=['alphawire'])  # library name, for the linker

    ffi.compile(verbose=True, debug=True, tmpdir=build_dir)
