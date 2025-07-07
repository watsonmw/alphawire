#pragma once

#include "mlib/mlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef __int128 i128;
typedef unsigned __int128 u128;

typedef enum ePTPDataType {
    PTP_DT_UNDEF = 0x0000,
    PTP_DT_INT8 = 0x0001,
    PTP_DT_UINT8 = 0x0002,
    PTP_DT_INT16 = 0x0003,
    PTP_DT_UINT16 = 0x0004,
    PTP_DT_INT32 = 0x0005,
    PTP_DT_UINT32 = 0x0006,
    PTP_DT_INT64 = 0x0007,
    PTP_DT_UINT64 = 0x0008,
    PTP_DT_INT128 = 0x0009,
    PTP_DT_UINT128 = 0x000A,
    PTP_DT_AINT8 = 0x4001,
    PTP_DT_AUINT8 = 0x4002,
    PTP_DT_AINT16 = 0x4003,
    PTP_DT_AUINT16 = 0x4004,
    PTP_DT_AINT32 = 0x4005,
    PTP_DT_AUINT32 = 0x4006,
    PTP_DT_AINT64 = 0x4007,
    PTP_DT_AUINT64 = 0x4008,
    PTP_DT_AINT128 = 0x4009,
    PTP_DT_AUINT128 = 0x400A,
    PTP_DT_STR = 0xFFFF,
} PTPDataType;

typedef enum ePTPFormFlag {
    PTP_FORM_FLAG_NONE = 0x00,
    PTP_FORM_FLAG_RANGE = 0x01,
    PTP_FORM_FLAG_ENUM = 0x02,
} PTPFormFlag;

typedef union uPropValue {
    u8 u8;
    i8 i8;
    u16 u16;
    i16 i16;
    u32 u32;
    i32 i32;
    u64 u64;
    i64 i64;
    u128 u128;
    i128 i128;
    MStr str;
} PropValue;

typedef struct sPtpRange {
    PropValue min;
    PropValue max;
    PropValue step;
} PtpRange;

typedef struct sPtpPropertyEnum {
    PropValue* set;
    PropValue* getSet;
} PtpPropertyEnum;

enum EnumValueFlags {
    ENUM_VALUE_READ = 0x1,
    ENUM_VALUE_WRITE = 0x2,
    ENUM_VALUE_STR_CONST = 0x4
};

typedef struct sPropValueEnum {
    PropValue propValue;
    MStr str;
    u16 flags;
} PropValueEnum;

typedef struct sPropValueEnums {
    PropValueEnum* values;
} PropValueEnums;

typedef struct sPropValueEnums1 {
    PropValueEnum* values;
    size_t size;
    b32 owned;
} PropValueEnums1;

typedef struct sPtpProperty {
    u16 propCode;
    u16 dataType;
    u8 getSet;
    u8 isEnabled;
    PropValue defaultValue;
    PropValue value;
    u8 formFlag;

    union {
        PtpRange range;
        PtpPropertyEnum enums;
    } form;
} PtpProperty;

typedef enum eSDI_ControlType {
    SDI_CONTROL_BUTTON = 0x81,
    SDI_CONTROL_NOTCH = 0x82,
    SDI_CONTROL_VARIABLE = 0x84,
} SDI_ControlType;

typedef struct sPtpControl {
    u16 controlCode;
    u16 dataType;
    u8 controlType;
    u8 formFlag;
    char* name;

    union {
        PtpRange range;
        PropValueEnums1 enums;
    } form;
} PtpControl;

enum PTPOperationCode {
    PTP_OC_GetDeviceInfo = 0x1001,
    PTP_OC_OpenSession = 0x1002,
    PTP_OC_CloseSession = 0x1003,
    PTP_OC_GetStorageID = 0x1004,
    PTP_OC_GetStorageInfo = 0x1005,
    PTP_OC_GetNumObjects = 0x1006,
    PTP_OC_GetObjectHandles = 0x1007,
    PTP_OC_GetObjectInfo = 0x1008,
    PTP_OC_GetObject = 0x1009,
    PTP_OC_GetThumb = 0x100A,
    PTP_OC_DeleteObject = 0x100B,
    PTP_OC_SendObject = 0x100D,
};

enum SDIOOperationCode {
    PTP_OC_SDIO_Connect = 0x9201,
    PTP_OC_SDIO_GetExtDeviceInfo = 0x9202,
    PTP_OC_SDIO_SetExtDevicePropValue = 0x9205,
    PTP_OC_SDIO_ControlDevice = 0x9207,
    PTP_OC_SDIO_GetAllExtDevicePropInfo = 0x9209,
    PTP_OC_SDIO_SetFTPSettingFilePassword = 0x920F,
    PTP_OC_SDIO_OpenSession = 0x9210,
    PTP_OC_SDIO_GetPartialLargeObject = 0x9211,
    PTP_OC_SDIO_SetContentsTransferMode = 0x9212,
    PTP_OC_SDIO_GetDisplayStringList = 0x9215,
    PTP_OC_SDIO_GetVendorCodeVersion = 0x9216,
    PTP_OC_SDIO_GetFTPJobList = 0x9217,
    PTP_OC_SDIO_ControlFTPJobList = 0x9218,
    PTP_OC_SDIO_UploadData = 0x921A,
    PTP_OC_SDIO_ControlUploadData = 0x921B,
    PTP_OC_SDIO_GetFTPSettingList = 0x921F,
    PTP_OC_SDIO_SetFTPSettingList = 0x9220,
    PTP_OC_SDIO_GetLensInformation = 0x9223,
    PTP_OC_SDIO_OperationResultsSupported = 0x922F
};

typedef enum eDevicePropertiesCode {
    DPC_COMPRESSION_SETTING = 0x5004,
    DPC_WHITE_BALANCE = 0x5005,
    DPC_F_NUMBER = 0x5007,
    DPC_FOCUS_MODE = 0x500A,
    DPC_EXPOSURE_METERING_MODE = 0x500B,
    DPC_FLASH_MODE = 0x500C,
    DPC_EXPOSURE_PROGRAM_MODE = 0x500E,
    DPC_EXPOSURE_COMPENSATION = 0x5010,
    DPC_CAPTURE_MODE = 0x5013,
    DPC_IRIS_MODE = 0xD001,
    DPC_IRIS_DISPLAY_UNIT = 0xD003,
    DPC_FOCAL_DISTANCE_METER = 0xD004,
    DPC_FOCAL_DISTANCE_FEET = 0xD005,
    DPC_FOCAL_DISTANCE_UNIT = 0xD006,
    DPC_FOCUS_MODE_SETTING = 0xD007,
    DPC_FOCUS_SPEED_RANGE = 0xD008,
    DPC_DIGITAL_ZOOM_SCALE = 0xD00A,
    DPC_ZOOM_DISTANCE = 0xD00B,
    DPC_WHITE_BALANCE_MODE = 0xD00C,
    DPC_SHUTTER_SETTING = 0xD00F,
    DPC_SHUTTER_MODE = 0xD010,
    DPC_SHUTTER_MODE_STATUS = 0xD011,
    DPC_SHUTTER_MODE_SETTING = 0xD013,
    DPC_SHUTTER_SLOW = 0xD014,
    DPC_SHUTTER_SLOW_FRAMES = 0xD015,
    DPC_SHUTTER_SPEED_VALUE = 0xD016,
    DPC_SHUTTER_SPEED_CURRENT = 0xD017,
    DPC_ND_FILTER = 0xD018,
    DPC_ND_FILTER_MODE = 0xD019,
    DPC_ND_FILTER_MODE_SETTING = 0xD01A,
    DPC_ND_FILTER_VALUE = 0xD01B,
    DPC_GAIN_CONTROL = 0xD01C,
    DPC_GAIN_UNIT = 0xD01D,
    DPC_GAIN_DB_VALUE = 0xD01E,
    DPC_EXPOSURE_INDEX = 0xD022,
    DPC_ISO_CURRENT = 0xD023,
    DPC_MOVIE_FILE_FORMAT_PROXY = 0xD027,
    DPC_PLAYBACK_MEDIA = 0xD042,
    DPC_TOUCH_OPERATION = 0xD047,
    DPC_TIME_CODE_FORMAT = 0xD0D5,
    DPC_SILENT_MODE = 0xD0DB,
    DPC_SILENT_MODE_APERTURE_DRIVE_AF = 0xD0DC,
    DPC_SILENT_MODE_POWER_OFF = 0xD0DD,
    DPC_SILENT_MODE_AUTO_PIXEL_MAPPING = 0xD0DE,
    DPC_SHUTTER_TYPE = 0xD0DF,
    DPC_CREATIVE_LOOK = 0xD0FA,
    DPC_FLASH_COMPENSATION = 0xD200,
    DPC_DRO_HDR_MODE = 0xD201,
    DPC_IMAGE_SIZE = 0xD203,
    DPC_SHUTTER_SPEED = 0xD20D,
    DPC_BATTERY_LEVEL = 0xD20E,
    DPC_COLOR_TEMPERATURE = 0xD20F,
    DPC_WHITE_BALANCE_GM = 0xD210,
    DPC_ASPECT_RATIO = 0xD211,
    DPC_AF_STATUS = 0xD213,
    DPC_PREDICTED_MAX_FILE_SIZE = 0xD214,
    DPC_PENDING_FILES = 0xD215,
    DPC_AE_LOCK_STATUS = 0xD217,
    DPC_BATTERY_REMAINING = 0xD218,
    DPC_PICTURE_EFFECT = 0xD21B,
    DPC_WHITE_BALANCE_AB = 0xD21C,
    DPC_MOVIE_REC_STATE = 0xD21D,
    DPC_ISO = 0xD21E,
    DPC_FEL_LOCK_STATUS = 0xD21F,
    DPC_LIVE_VIEW_STATUS = 0xD221,
    DPC_IMAGE_SAVE_DESTINATION = 0xD222,
    DPC_FOCUS_AREA = 0xD22C,
    DPC_FOCUS_MAGNIFY_SCALE = 0xD22f,
    DPC_FOCUS_MAGNIFY_POS = 0xD230,
    DPC_LIVE_VIEW_SETTING_EFFECT = 0xD231,
    DPC_FOCUS_AREA_POS_OLD = 0xD232,
    DPC_MANUAL_FOCUS_ADJUST_ENABLED = 0xD235,
    DPC_PIXEL_SHIFT_SHOOTING_MODE = 0xD239,
    DPC_PIXEL_SHIFT_SHOOTING_NUMBER = 0xD23A,
    DPC_PIXEL_SHIFT_SHOOTING_INTERVAL = 0xD23B,
    DPC_PIXEL_SHIFT_SHOOTING_STATUS = 0xD23C,
    DPC_PIXEL_SHIFT_SHOOTING_PROGRESS = 0xD23D,
    DPC_PICTURE_PROFILE = 0xD23F,
    DPC_CREATIVE_STYLE = 0xD240,
    DPC_MOVIE_FILE_FORMAT = 0xD241,
    DPC_MOVIE_QUALITY = 0xD242,
    DPC_MEDIA_SLOT1_STATUS = 0xD248,
    DPC_FOCAL_POSITION = 0xD24C,
    DPC_AWB_LOCK_STATUS = 0xD24E,
    DPC_INTERVAL_RECORD_MODE = 0xD24F,
    DPC_INTERVAL_RECORD_STATUS = 0xD250,
    DPC_DEVICE_OVERHEATING_STATE = 0xD251,
    DPC_IMAGE_QUALITY = 0xD252,
    DPC_IMAGE_FILE_FORMAT = 0xD253,
    DPC_FOCUS_MAGNIFY = 0xD254,
    DPC_AF_TRACKING_SENS = 0xD255,
    DPC_MEDIA_SLOT2_STATUS = 0xD256,
    DPC_EXPOSURE_MODE_KEY = 0xD25A,
    DPC_ZOOM_OPERATION_ENABLED = 0xD25B,
    DPC_ZOOM_SCALE = 0xD25C,
    DPC_ZOOM_BAR_INFO = 0xD25D,
    DPC_ZOOM_SETTING = 0xD25F,
    DPC_ZOOM_TYPE_STATUS = 0xD260,
    DPC_WIRELESS_FLASH = 0xD262,
    DPC_RED_EYE_REDUCTION = 0xD263,
    DPC_REMOTE_RESTRICT_STATUS = 0xD264,
    DPC_IMAGE_TRANSFER_SIZE = 0xD268,
    DPC_PC_SAVE_IMAGE = 0xD269,
    DPC_LIVE_VIEW_QUALITY = 0xD26A,
    DPC_CAMERA_SETTING_SAVE_ENABLED = 0xD271,
    DPC_CAMERA_SETTING_READ_ENABLED = 0xD272,
    DPC_CAMERA_SETTING_SAVE_READ_STATE = 0xD273,
    DPC_FORMAT_MEDIA_SLOT1_ENABLED = 0xD279,
    DPC_FORMAT_MEDIA_SLOT2_ENABLED = 0xD27A,
    DPC_FORMAT_MEDIA_PROGRESS = 0xD27B,
    DPC_TOUCH_OPERATION_FUNCTION = 0xD283,
    DPC_REMOTE_TOUCH_ENABLED = 0xD284,
    DPC_REMOTE_TOUCH_CANCEL_ENABLED = 0xD285,
    DPC_MOVIE_FRAME_RATE = 0xD286,
    DPC_COMPRESSED_IMAGE_FILE_FORMAT = 0xD287,
    DPC_RAW_FILE_TYPE = 0xD288,
    DPC_FORMAT_MEDIA_QUICK_SLOT1_ENABLED = 0xD292,
    DPC_FORMAT_MEDIA_QUICK_SLOT2_ENABLED = 0xD293,
    DPC_FORMAT_MEDIA_CANCEL_ENABLED = 0xD294,
    DPC_CONTENTS_TRANSFER_ENABLED = 0xD295,
    DPC_LENS_INFORMATION_ENABLED = 0XE086,

    // Controls
    DPC_S1_BUTTON = 0xD2C1,
    DPC_S2_BUTTON = 0xD2C2,
    DPC_AE_LOCK = 0xD2C3,
    DPC_AFL_BUTTON = 0xD2C4,
    DPC_RELEASE_LOCK = 0xD2C5,
    DPC_REQUEST_ONE_SHOOTING = 0xD2C7,
    DPC_MOVIE_RECORD = 0xD2C8,
    DPC_FEL_BUTTON = 0xD2C9,
    DPC_MEDIA_FORMAT = 0xD2CA,
    DPC_FOCUS_MAGNIFIER = 0XD2CB,
    DPC_FOCUS_MAGNIFIER_CANCEL = 0XD2CC,
    DPC_REMOTE_KEY_UP = 0XD2CD,
    DPC_REMOTE_KEY_DOWN = 0XD2CE,
    DPC_REMOTE_KEY_LEFT = 0XD2CF,
    DPC_REMOTE_KEY_RIGHT =  0XD2D0,
    DPC_MANUAL_FOCUS_ADJUST = 0xD2D1,
    DPC_AUTO_FOCUS_HOLD = 0xD2D2,
    DPC_PIXEL_SHIFT_SHOOT_CANCEL = 0xD2D3,
    DPC_PIXEL_SHIFT_SHOOT = 0xD2D4,
    DPC_HFR_STANDBY = 0xD2D5,
    DPC_HFR_RECORD_CANCEL = 0xD2D6,
    DPC_FOCUS_STEP_NEAR = 0xD2D7,
    DPC_FOCUS_STEP_FAR = 0xD2D8,
    DPC_AWB_LOCK = 0xD2D9,
    DPC_FOCUS_AREA_X_Y = 0xD2DC,
    DPC_ZOOM = 0xD2DD,
    DPC_CUSTOM_WB_CAPTURE_STANDBY = 0XD2DF,
    DPC_CUSTOM_WB_CAPTURE_STANDBY_CANCEL = 0XD2E0,
    DPC_CUSTOM_WB_CAPTURE = 0XD2E1,
    DPC_FORMAT_MEDIA = 0XD2E2,
    DPC_REMOTE_TOUCH_XY = 0XD2E4,
    DPC_REMOTE_TOUCH_CANCEL = 0XD2E5,
    DPC_S1_AND_S2_BUTTON = 0XD2E6,
    DPC_FORMAT_MEDIA_CANCEL = 0XD2E7,
    DPC_SAVE_ZOOM_AND_FOCUS_POSITION = 0XD2E9,
    DPC_LOAD_ZOOM_AND_FOCUS_POSITION = 0XD2EA,
    DPC_APS_C_FULL_TOGGLE = 0XD2EB,
    DPC_COLOR_TEMPERATURE_STEP = 0XD2EC,
    DPC_WHITE_BALANCE_TINT_STEP = 0XD2ED,
    DPC_FOCUS_OPERATION = 0XD2EF,
    DPC_FLICKER_SCAN = 0XD2F1,
    DPC_SETTINGS_RESET = 0XD2F3,
    DPC_PIXEL_MAPPING = 0XD300,
    DPC_POWER_OFF = 0XD301,
    DPC_TIME_CODE_PRESET_RESET = 0XD302,
    DPC_USER_BIT_PRESET_RESET = 0XD303,
    DPC_SENSOR_CLEANING = 0XD304,
    DPC_RESET_PICTURE_PROFILE = 0XD305,
    DPC_RESET_CREATIVE_LOOK = 0XD306,
    DPC_SHUTTER_ECS_NUMBER_STEP = 0XF000,
    DPC_MOVIE_RECORD_TOGGLE = 0XF001,
    DPC_FOCUS_POSITION_CANCEL = 0XF002,
} DevicePropertiesCode;

typedef enum eStringDisplayList {
    PTP_DL_ALL = 0x00000000,
    PTP_DL_BASE_LOOK_AE_LEVEL_OFFSET_EXPOSURE = 0x01,
    PTP_DL_BASE_LOOK_INPUT = 0x02,
    PTP_DL_BASE_LOOK_NAME = 0x03,
    PTP_DL_BASE_LOOK_OUTPUT = 0x04,
    PTP_DL_SCENE_FILE_NAME = 0x05,
    PTP_DL_SHOOTING_MODE_CINEMA_COLOR_GAMUT = 0x06,
    PTP_DL_SHOOTING_MODE_TARGET_DISPLAY = 0x07,
    PTP_DL_CAMERA_GAIN_BASE_ISO = 0x08,
    PTP_DL_VIDEO_EI_GAIN = 0x09,
    PTP_DL_BUTTON_ASSIGN = 0x0A,
    PTP_DL_BUTTON_ASSIGN_SHORT = 0x0B,
    PTP_DL_FTP_SERVER_NAME = 0x0C,
    PTP_DL_FTP_UPLOAD_DIRECTORY = 0x0D,
    PTP_DL_FTP_JOB_STATUS = 0x0E,
    PTP_DL_EXPOSURE_INDEX_PRESET1 = 0x0F,
    PTP_DL_MOVIE_TRANSFER_EXTENSION = 0x10,
    PTP_DL_MOVIE_TRANSFER_VIDEO_CODEC = 0x11,
    PTP_DL_MOVIE_TRANSFER_FRAMERATE = 0x12,
    PTP_DL_CREATIVE_LOOK_IMAGE_STYLE = 0x13,
    PTP_DL_IPTC_METADATA = 0x14,
    PTP_DL_SUBJECT_RECOGNITION_AF = 0x15,
    PTP_DL_BASE_LOOK_META_RECORD_SUPPORT = 0x16,
    PTP_DL_SELECT_REC_FOLDER_NAME = 0x17,
} PtpStringDisplayList;

typedef enum ePtpFocusUnits {
    PTP_FOCUS_UNIT_FEET = 1,
    PTP_FOCUS_UNIT_METERS = 2,
} PtpFocusUnits;

typedef enum eSonyProtocolVersion {
    SDI_EXTENSION_VERSION_200 = 200,
    SDI_EXTENSION_VERSION_300 = 300,
} SonyProtocolVersion;

typedef enum ePTPResult {
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

typedef enum ePTPObjectFormatCode {
    PTP_OFC_FOLDER = 0x3001,
    PTP_OFC_TEXT = 0x3004,
    PTP_OFC_MPEG = 0x300B,
    PTP_OFC_JPEG = 0x3801,
    PTP_OFC_JFIF = 0x3808,
    PTP_OFC_RAW = 0xB101,
    PTP_OFC_HEIF = 0xB110,
    PTP_OFC_MPO = 0xB301,
    PTP_OFC_WMA = 0xB901,
    PTP_OFC_MP4 = 0xB982,
} PTPObjectFormatCode;

typedef enum eSD_EnabledDisabled
{
    SD_Disabled = 0x00,
    SD_Enabled = 0x01,
} SD_EnabledDisabled;

typedef enum sSD_ObjectHandle {
    SD_OH_CAPTURED_IMAGE = 0xffffc001,
    SD_OH_LIVE_VIEW_IMAGE = 0xffffc002,
    SD_OH_CAMERA_SETTINGS = 0xffffc004,
    SD_OH_FTP_SETTINGS = 0xffffc005
} SD_ObjectHandle;

typedef enum eSD_FocusFrameType
{
    SD_PhaseDetection_AFSensor = 0x0001,
    SD_PhaseDetection_ImageSensor = 0x0002,
    SD_Wide = 0x0003,
    SD_Zone = 0x0004,
    SD_CentralEmphasis = 0x0005,
    SD_ContrastFlexibleMain = 0x0006,
    SD_ContrastFlexibleAssist = 0x0007,
    SD_Contrast = 0x0008,
    SD_ContrastUpperHalf = 0x0009,
    SD_ContrastLowerHalf = 0x000A,
    SD_DualAFMain = 0x000B,
    SD_DualAFAssist = 0x000C,
    SD_NonDualAFMain = 0x000D,
    SD_NonDualAFAssist = 0x000E,
    SD_FrameSomewhere = 0x000F,
    SD_Cross = 0x0010,
} SD_FocusFrameType;

typedef enum eSD_FocusFrameState
{
    SD_NotFocused = 0x0001,
    SD_Focused = 0x0002,
    SD_FocusFrameSelection = 0x0003,
    SD_Moving = 0x0004,
    SD_RangeLimit = 0x0005,
    SD_RegistrationAF = 0x0006,
    SD_Island = 0x0007,
} SD_FocusFrameState;

typedef enum eSD_FaceFrameType
{
    SD_DetectedFace = 0x0001,
    SD_AF_TargetFace = 0x0002,
    SD_PersonalRecognitionFace = 0x0003,
    SD_SmileDetectionFace = 0x0004,
    SD_SelectedFace = 0x0005,
    SD_AF_TargetSelectionFace = 0x0006,
    SD_SmileDetectionSelectFace = 0x0007,
} SD_FaceFrameType;

typedef enum eSD_SelectionState
{
    SD_Unselected = 0x01,
    SD_Selected = 0x02,
} SD_SelectionState;

typedef enum eTrackingFrameType
{
    SD_NonTargetAF = 0x0001,
    SD_TargetAF = 0x0002,
} SD_TrackingFrameType;

typedef struct sFocusFrame {
    u16 frameType; // SD_FocusFrameType
    u16 focusFrameState; // SD_FocusFrameState
    u8 priority;
    u32 height;
    u32 width;
} FocusFrame;

typedef struct sFocusFrames {
    u32 xDenominator;
    u32 yDenominator;
    FocusFrame* frames;
} FocusFrames;

typedef struct sFocusFrameFace {
    u16 faceFrameType; // SD_FaceFrameType
    u16 faceFocusFrameState; // SD_FaceFrameState
    u16 selectionState; // SD_SelectionState
    u8 priority;
    u32 xNumerator;
    u32 yNumerator;
    u32 height;
    u32 width;
} FocusFrameFace;

typedef struct sFaceFrames {
    u32 xDenominator;
    u32 yDenominator;
    FocusFrameFace* frames;
} FaceFrames;

typedef struct sFocusFrameTracking {
    u16 trackingFrameType; // SD_TrackingFrameType
    u16 trackingFrameState; // SD_TrackingFrameState
    u8 priority;
    u32 xNumerator;
    u32 yNumerator;
    u32 height;
    u32 width;
} FocusFrameTracking;

typedef struct sTrackingFrames {
    u32 xDenominator;
    u32 yDenominator;
    FocusFrameTracking* frames;
} TrackingFrames;

typedef struct sLiveViewFrames {
    u16 version;
    FocusFrames focus;
    FaceFrames face;
    TrackingFrames tracking;
} LiveViewFrames;

typedef struct sPtpCapturedImageInfo {
    MStr filename;
    PTPObjectFormatCode objectFormat;
    size_t size;
} PtpCapturedImageInfo;

#define PTP_MAX_PARAMS 5

typedef struct sPtpRequest {
    u16 OpCode;
    u32 SessionId;
    u32 TransactionId;
    u32 Params[PTP_MAX_PARAMS];
    u32 NumParams;
    u32 NextPhase;
} PtpRequest;

typedef struct sPtpResponse {
    u16 ResponseCode;
    u32 SessionId;
    u32 TransactionId;
    u32 NumParams;
    u32 Params[PTP_MAX_PARAMS];
} PtpResponse;

typedef enum ePTPNextPhase {
    PTP_NEXT_PHASE_READ_DATA = 3,
    PTP_NEXT_PHASE_WRITE_DATA = 4,
    PTP_NEXT_PHASE_NO_DATA = 5
} PTPNextPhase;

typedef enum ePtpBufferType {
    PTP_BUFFER_IN,
    PTP_BUFFER_OUT,
} PtpBufferType;

typedef void* (*PTP_Device_ReallocBuffer_t)(void* deviceSelf, PtpBufferType type, void* dataMem, size_t dataOldSize, size_t dataNewSize);
typedef void (*PTP_Device_FreeBuffer_t)(void* deviceSelf, PtpBufferType type, void* dataMem, size_t dataOldSize);
typedef PTPResult (*PTP_Device_SendAndRecvEx_t)(void* deviceSelf, PtpRequest* request, u8* dataIn, size_t dataInSize,
                                                PtpResponse* response, u8* dataOut, size_t dataOutSize,
                                                size_t* actualDataOutSize);

typedef struct sPTPDeviceTransport {
    PTP_Device_ReallocBuffer_t reallocBuffer;
    PTP_Device_FreeBuffer_t freeBuffer;
    PTP_Device_SendAndRecvEx_t sendAndRecvEx;
    b32 requiresSessionOpenClose;
} PTPDeviceTransport;

typedef enum ePtpBackendType {
    PTP_BACKEND_WIA,
    PTP_BACKEND_LIBUSBK,
} PtpBackendType;

typedef struct sPTPDeviceInfo {
    PtpBackendType backendType;
    MStr manufacturer;
    MStr deviceName;
    void* device;
} PTPDeviceInfo;

typedef struct sPTPDevice {
    PTPDeviceTransport transport;
    PtpBackendType backendType;
    b32 disconnected;
    void* device;
} PTPDevice;

struct sPTPBackend;

typedef b32 (*PTP_Backend_Close_t)(struct sPTPBackend* backend);
typedef b32 (*PTP_Backend_RefreshList_t)(struct sPTPBackend* backend, PTPDeviceInfo** deviceList);
typedef void (*PTP_Backend_ReleaseList_t)(struct sPTPBackend* backend);
typedef b32 (*PTP_Backend_OpenDevice_t)(struct sPTPBackend* backend, PTPDeviceInfo* deviceInfo, PTPDevice** deviceOut);
typedef b32 (*PTP_Backend_CloseDevice_t)(struct sPTPBackend* backend, PTPDevice* device);

typedef struct sPTPBackend {
    PTP_Backend_Close_t close;
    PTP_Backend_RefreshList_t refreshList;
    PTP_Backend_ReleaseList_t releaseList;
    PTP_Backend_OpenDevice_t openDevice;
    PTP_Backend_CloseDevice_t closeDevice;
    PtpBackendType type;
    void* self;
} PTPBackend;

#ifdef __cplusplus
} // extern "C"
#endif
