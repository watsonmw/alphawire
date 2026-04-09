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
    u32 capacity;
} MStr;

typedef struct {
    char* str;
    u32 size;
} MStrView;

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
    void* device; // concrete backend device - contains backend specific device data
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

void PTP_InitDefaultAllocator(MAllocator* allocator);

b32 AwDeviceList_Open(AwDeviceList* self, MAllocator* allocator);
b32 AwDeviceList_Close(AwDeviceList* self);
b32 AwDeviceList_RefreshList(AwDeviceList* self);
b32 AwDeviceList_NeedsRefresh(AwDeviceList* self);
size_t AwDeviceList_NumDevices(AwDeviceList* self);
b32 AwDeviceList_OpenDevice(AwDeviceList* self, AwDeviceInfo* deviceInfo, AwDevice** deviceOut);

AwBackend* AwDeviceList_GetBackend(AwDeviceList* self, AwBackendType backend);

typedef enum {
    SDI_EXTENSION_VERSION_200 = 200,
    SDI_EXTENSION_VERSION_300 = 300,
} AwSonyProtocolVersion;

typedef struct {
    AwDevice* device;
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
} AwControl;

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
} PtpResult;

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
    
    u8 isNotch; // Property can only be changed by 'notching' - needed for some properties

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
} AwControl;

typedef struct {
    u16 frameType; // SD_FocusFrameType
    u16 focusFrameState; // SD_FocusFrameState
    u8 priority;
    u32 height;
    u32 width;
} AwFocusFrame;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    AwFocusFrame* frames;
} AwFocusFrames;

typedef struct {
    u16 faceFrameType; // SD_FaceFrameType
    u16 faceFocusFrameState; // SD_FaceFrameState
    u16 selectionState; // SD_SelectionState
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
    u16 trackingFrameType; // SD_TrackingFrameType
    u16 trackingFrameState; // SD_TrackingFrameState
    u8 priority;
    u32 xNumerator;
    u32 yNumerator;
    u32 height;
    u32 width;
} AwFocusFrameTracking;

typedef struct {
    u32 xDenominator;
    u32 yDenominator;
    FocusFrameTracking* frames;
} AwTrackingFrames;

typedef struct {
    u16 version;
    FocusFrames focus;
    FaceFrames face;
    TrackingFrames tracking;
} AwLiveViewFrames;

AwResult AwControl_Init(AwControl* self, AwDevice* device, MAllocator* allocator);
AwResult AwControl_Connect(AwControl* self, AwSonyProtocolVersion version);
AwResult AwControl_Cleanup(AwControl* self);
AwResult AwControl_UpdateProperties(AwControl* self);
size_t AwControl_NumProperties(AwControl* self);
AwPtpProperty* AwControl_GetPropertyAtIndex(AwControl* self, u16 index);
AwPtpProperty* AwControl_GetPropertyByCode(AwControl* self, u16 propertyCode);
AwPtpProperty* AwControl_GetPropertyById(AwControl* self, const char* propertyId);
b32 AwControl_GetPropertyValueAsStr(AwControl* self, AwPtpProperty* property, MAllocator* alloc, MStr* strOut);
AwResult AwControl_GetLiveViewImage(AwControl* self, MMemIO* fileOut, AwLiveViewFrames* liveViewFramesOut);
void AwControl_FreeLiveViewFrames(AwControl* self, AwLiveViewFrames* liveViewFrames);

size_t AwControl_NumControls(AwControl* self);
AwControl* AwControl_GetControlAtIndex(AwControl* self, u16 index);

char* PTP_GetOperationLabel(u16 operationCode);
char* PTP_GetControlLabel(u16 controlCode);
char* PTP_GetEventLabel(u16 eventCode);
char* PTP_GetPropertyLabel(u16 propCode);

void PTP_MemIOFree(MMemIO* memIO);
void PTP_StrFree(MAllocator* allocator, MStr* str);

""")

lib_source_code = """
#define M_USE_SDL
#define M_USE_STDLIB
#define M_ASSERT
#define M_STACKTRACE
#define M_MEM_DEBUG
#define ALPHAWIRE_BUILDING_SHARED_LIB

#include "aw/aw-const.h"
#include "aw/aw-control.h"
#include "aw/aw-device-list.h"
#include "aw/aw-log.h"
#include "aw/aw-util.h"
#include "aw/platform/usb-const.h"
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

    ffi.compile(verbose=True, debug=False, tmpdir=build_dir)
