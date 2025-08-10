#include <stddef.h>
#include <stdarg.h>

#include "ptp/ptp-control.h"
#include "ptp/utf.h"


typedef struct {
    u16 code;
    char* name;
} PropMetadata;

static PropMetadata sPtpPropertiesMetadata[] = {
    {DPC_COMPRESSION_SETTING, "Compression Setting"},
    {DPC_WHITE_BALANCE, "White Balance"},
    {DPC_F_NUMBER, "F-Number"},
    {DPC_FOCUS_MODE, "Focus Mode"},
    {DPC_EXPOSURE_METERING_MODE, "Exposure Metering Mode"},
    {DPC_FLASH_MODE, "Flash Mode"},
    {DPC_EXPOSURE_PROGRAM_MODE, "Exposure Program Mode"},
    {DPC_EXPOSURE_COMPENSATION, "Exposure Bias Compensation"},
    {DPC_CAPTURE_MODE, "Capture Mode"},
    {0xD000, "T-Number"},
    {DPC_IRIS_MODE, "Iris Mode"},
    {DPC_IRIS_DISPLAY_UNIT, "Iris Display Unit"},
    {DPC_FOCAL_DISTANCE_METER, "Focal Distance (Meters)"},
    {DPC_FOCAL_DISTANCE_FEET, "Focal Distance (Feet)"},
    {DPC_FOCAL_DISTANCE_UNIT, "Focal Distance Unit Setting"},
    {DPC_FOCUS_MODE_SETTING, "Focus Mode Setting"},
    {DPC_FOCUS_SPEED_RANGE, "Focus Speed Range"},
    {DPC_DIGITAL_ZOOM_SCALE, "Digital Zoom Scale"},
    {DPC_ZOOM_DISTANCE, "Zoom Distance"},
    {DPC_WHITE_BALANCE_MODE, "White Balance Mode Setting"},
    {0xD00D, "White Balance Tint"},
    {0xD00E, "Shutter Angle"},
    {DPC_SHUTTER_SETTING, "Shutter Setting"},
    {DPC_SHUTTER_MODE, "Shutter Mode"},
    {DPC_SHUTTER_MODE_STATUS, "Shutter Mode Status"},
    {DPC_SHUTTER_MODE_SETTING, "Shutter Mode Setting"},
    {DPC_SHUTTER_SLOW, "Shutter Slow"},
    {DPC_SHUTTER_SLOW_FRAMES, "Shutter Slow Frames"},
    {DPC_SHUTTER_SPEED_VALUE, "Shutter Speed Value"},
    {DPC_SHUTTER_SPEED_CURRENT, "Shutter Speed Current Value"},
    {DPC_ND_FILTER, "ND Filter"},
    {DPC_ND_FILTER_MODE, "ND Filter Mode"},
    {DPC_ND_FILTER_MODE_SETTING, "ND Filter Mode Setting"},
    {DPC_ND_FILTER_VALUE, "ND Filter Value"},
    {DPC_GAIN_CONTROL, "Gain Control Setting"},
    {DPC_GAIN_UNIT, "Gain Unit Setting"},
    {DPC_GAIN_DB_VALUE, "Gain dB Value"},
    {0xD01F, "Gain dB Current Value"},
    {0xD020, "Gain Base ISO Sensitivity"},
    {0xD021, "Gain Base Sensitivity"},
    {DPC_EXPOSURE_INDEX, "Exposure Index"},
    {DPC_ISO_CURRENT, "ISO Current"},
    {0xD024, "Movie Recording Resolution"},
    {0xD025, "Movie Recording Resolution Proxy"},
    {DPC_MOVIE_FILE_FORMAT_PROXY, "Movie File Format Proxy"},
    {0xD028, "Movie Frame Rate Proxy"},
    {0xD029, "Zoom Distance Unit Setting"},
    {0xD02E, "Select FTP ServerID"},
    {0xD02F, "Movie Playing State"},
    {0xD030, "Movie Playing Speed"},
    {0xD031, "Media Slot 1 ProfileUrl"},
    {0xD032, "Media Slot 2 ProfileUrl"},
    {0xD035, "Media Slot 1 Player"},
    {0xD036, "Media Slot 2 Player"},
    {0xD037, "Battery Remain Display Unit"},
    {0xD038, "Battery Remaining in Minutes"},
    {0xD039, "Battery Remaining in Voltage"},
    {0xD03A, "Power Source"},
    {0xD03B, "AWB"},
    {0xD03C, "BaseLook Value"},
    {0xD03E, "DC Voltage"},
    {0xD040, "Software Version"},
    {0xD041, "FTP Function"},
    {0xD02A, "Sync ID for FTP Job List"},
    {DPC_PLAYBACK_MEDIA, "Playback Media"},
    {0xD043, "Settings Reset Enabled"},
    {0xD044, "Monitor DISP (Screen Display) Mode Candidates"},
    {0xD045, "Monitor DISP (Screen Display) Mode Setting"},
    {0xD046, "Monitor DISP (Screen Display) Mode"},
    {DPC_TOUCH_OPERATION, "Touch Operation"},
    {0xD048, "Select Finder/Monitor"},
    {0xD049, "Auto Power OFF Temperature"},
    {0xD04A, "Body Key Lock"},
    {0xD04B, "Image ID (Numerical Value)"},
    {0xD04C, "Image ID (String)"},
    {0xD04D, "Monitor LUT Setting (All Line)"},
    {0xD04E, "Auto FTP Transfer"},
    {0xD04F, "Auto FTP Transfer Target"},
    {0xD052, "S&Q Frame Rate"},
    {0xD055, "Interval REC (Movie) Time"},
    {0xD057, "Upload Dataset Version"},
    {0xD059, "BaseLookImport Command Version"},
    {0xD060, "Subject Recognition AF"},
    {0xD061, "AF Transition Speed"},
    {0xD062, "AF Subj Shift Sens"},
    {0xD073, "ND Filter Switching Setting"},
    {0xD079, "Monitoring Output Display HDMI"},
    {0xD07B, "Lens Model Name"},
    {0xD07D, "Lens Version Number"},
    {0xD08B, "BaseLookImport Operation Enabled"},
    {0xD092, "Image ID (Numerical Value) Setting"},
    {0xD099, "Exposure Ctrl Type"},
    {0xD09A, "FTPSettingList Operation Enabled"},
    {0xD0AB, "Focus Bracket Shooting Status"},
    {0xD0BC, "Camera Operating Mode"},
    {0xD0BD, "Playback View Mode"},
    {0xD0C5, "Type-C Accessory Mode"},
    {0xD0C6, "Pixel Mapping Enabled"},
    {0xD0C7, "Delete UserBaseLook"},
    {0xD0C8, "Select UserBaseLook to Edit"},
    {0xD0C9, "UserBaseLook Input"},
    {0xD0CA, "UserBaseLook AE Level Offset"},
    {0xD0CB, "Base ISO Switch EI"},
    {0xD0CC, "Select BaseLook to Set in PPLUT"},
    {0xD0CD, "E-framing Scale (Auto)"},
    {0xD0CE, "E-framing Speed (Auto)"},
    {0xD0CF, "Camera E-framing"},
    {0xD0D0, "S&Q Rec Frame Rate"},
    {0xD0D1, "S&Q Record Setting"},
    {0xD0D2, "Audio Recording"},
    {0xD0D3, "Time Code Preset"},
    {0xD0D4, "User Bit Preset"},
    {DPC_TIME_CODE_FORMAT, "Time Code Format"},
    {0xD0D6, "Time Code Run"},
    {0xD0D7, "Time Code Make"},
    {0xD0D8, "User Bit Time Rec"},
    {0xD0D9, "Image Stabilization Steady Shot"},
    {0xD0DA, "Movie Stabilization Steady Shot"},
    {DPC_SILENT_MODE, "Silent Mode"},
    {DPC_SILENT_MODE_APERTURE_DRIVE_AF, "Aperture Drive AF"},
    {DPC_SILENT_MODE_POWER_OFF, "Silent Mode Shutter Power Off"},
    {DPC_SILENT_MODE_AUTO_PIXEL_MAPPING, "Silent Mode Auto Pixel Mapping"},
    {DPC_SHUTTER_TYPE, "Shutter Type"},
    {0xD0E0, "Picture Profile BlackLevel"},
    {0xD0E1, "Picture Profile Gamma"},
    {0xD0E2, "Picture Profile BlackGamma Range"},
    {0xD0E3, "Picture Profile BlackGamma Level"},
    {0xD0E4, "Picture Profile Knee Mode"},
    {0xD0E5, "Picture Profile Knee AutoSet MaxPoint"},
    {0xD0E6, "Picture Profile Knee AutoSet Sensitivity"},
    {0xD0E7, "Picture Profile Knee ManualSet Point"},
    {0xD0E8, "Picture Profile Knee ManualSet Slope"},
    {0xD0E9, "Picture Profile Color Mode"},
    {0xD0EA, "Picture Profile Saturation"},
    {0xD0EB, "Picture Profile ColorPhase"},
    {0xD0EC, "Picture Profile Color Depth Red"},
    {0xD0ED, "Picture Profile Color Depth Green"},
    {0xD0EE, "Picture Profile Color Depth Blue"},
    {0xD0EF, "Picture Profile Color Depth Cyan"},
    {0xD0F0, "Picture Profile Color Depth Magenta"},
    {0xD0F1, "Picture Profile Color Depth Yellow"},
    {0xD0F2, "Picture Profile Detail Level"},
    {0xD0F3, "Picture Profile Detail Adjust Mode"},
    {0xD0F4, "Picture Profile Detail Adjust V/H Balance"},
    {0xD0F5, "Picture Profile Detail Adjust B/W Balance"},
    {0xD0F6, "Picture Profile Detail Adjust Limit"},
    {0xD0F7, "Picture Profile Detail Adjust Crispening"},
    {0xD0F8, "Picture Profile Detail Adjust Highlight Detail"},
    {0xD0F9, "Copy Picture Profile"},
    {DPC_CREATIVE_LOOK, "Creative Look"},
    {0xD0FB, "Creative Look Contrast"},
    {0xD0FC, "Creative Look Highlights"},
    {0xD0FD, "Creative Look Shadows"},
    {0xD0FE, "Creative Look Fade"},
    {0xD0FF, "Creative Look Saturation"},
    {0xD100, "Creative Look Sharpness"},
    {0xD101, "Creative Look Sharpness Range"},
    {0xD102, "Creative Look Clarity"},
    {0xD103, "Custom Look Image Style"},
    {0xD104, "Time Code Preset Reset Enabled"},
    {0xD105, "User Bit Preset Reset Enabled"},
    {0xD106, "Sensor Cleaning Enabled"},
    {0xD107, "Reset Picture Profile Enabled"},
    {0xD108, "Reset Creative Look Enabled"},
    {0xD109, "Proxy Record Setting"},
    {0xD11F, "Interval REC (Movie) Count Down Interval Time"},
    {0xD120, "Recording Duration"},
    {0xD124, "E-framing Mode (Auto)"},
    {0xD133, "Flicker Less Shooting"},
    {0xD15B, "Long Exposure NR"},
    {0xD15C, "High ISO NR"},
    {0xD15D, "HLG Image"},
    {0xD15E, "Color Space Image"},
    {0xD166, "Bracket order"},
    {0xD167, "Focus Bracket order"},
    {0xD168, "Focus Bracket Exposure Lock 1st Img"},
    {0xD169, "Focus Bracket Interval Until Next Shot"},
    {0xD16A, "Interval REC Shooting Start Time"},
    {0xD16B, "Interval REC Shooting Interval"},
    {0xD16C, "Interval REC Number of Shots"},
    {0xD16D, "Interval REC AE Tracking Sensitivity"},
    {0xD16E, "Interval REC Shutter Type"},
    {0xD16F, "Interval REC Shoot Interval Priority"},
    {0xD171, "Wind Noise Reduction"},
    {0xD173, "Auto Slow Shutter"},
    {0xD14D, "ISO Auto Min Shutter Speed Mode"},
    {0xD176, "ISO Auto Min Shutter Speed Manual"},
    {0xD177, "ISO Auto Min Shutter Speed Preset"},
    {0xD178, "Soft Skin Effect"},
    {0xD179, "Priority Set in AF-S"},
    {0xD17A, "Priority Set in AF-C"},
    {0xD17B, "Focus Magnification Time"},
    {0xD17C, "Playback Volume Settings"},
    {0xD17D, "Auto Review"},
    {0xD17E, "Audio Signals"},
    {0xD17F, "HDMI Resolution (Still/Play)"},
    {0xD180, "HDMI Output Rec Media (Movie)"},
    {0xD181, "HDMI Output Resolution (Movie)"},
    {0xD182, "HDMI Output 4K Set (Movie)"},
    {0xD183, "HDMI Output RAW (Movie)"},
    {0xD184, "HDMI Output Raw Setting (Movie)"},
    {0xD186, "HDMI Output Time Code (Movie)"},
    {0xD187, "HDMI Output REC Control (Movie)"},
    {0xD18E, "Media Slot 3 Status"},
    {0xD18F, "Media Slot 3 Remaining Shoot Time"},
    {0xD190, "Media Slot 3 Rec Available Type"},
    {0xD191, "Media Slot 3 ProfileUrl"},
    {0xD192, "Image Stabilization Steady Shot Adjust"},
    {0xD193, "Image Stabilization Steady Shot Focal Length"},
    {0xD199, "Auto FTP Transfer Target (Movie)"},
    {0xD19A, "FTP Transfer Target"},
    {0xD14B, "FTP Transfer Target (Proxy)"},
    {0xD14C, "FTP Power Save"},
    {0xD14E, "ND Filter Unit Setting"},
    {0xD14F, "ND Filter Optical Density Value"},
    {0xD150, "USB Power Supply"},
    {0xD151, "Interval REC (Movie) Frame Rate"},
    {0xD152, "Interval REC (Movie) Record Setting"},
    {0xD153, "E-framing Recording Image Crop"},
    {0xD154, "E-framing HDMI Crop"},
    {0xD157, "Subject Recognition in AF"},
    {0xD158, "Recognition Target"},
    {0xD159, "Right/Left Eye Select"},
    {0xD15F, "Recording Media - Image"},
    {0xD160, "Recording Media - Movie"},
    {0xD161, "Auto Switch Media"},
    {0xD194, "Camera Shake Status"},
    {0xD195, "Update Body Status"},
    {0xD197, "Media Slot 1 Writing State"},
    {0xD198, "Media Slot 2 Writing State"},
    {0xD19C, "Focus Driving Status (Absolute)"},
    {0xD19D, "Zoom Driving Status (Absolute)"},
    {0xD1B6, "ISO Auto Range Limit (min)"},
    {0xD1B7, "ISO Auto Range Limit (max)"},
    {DPC_FLASH_COMPENSATION, "Flash Compensation"},
    {DPC_DRO_HDR_MODE, "Dynamic Range Optimizer"},
    {DPC_IMAGE_SIZE, "Image Size"},
    {DPC_SHUTTER_SPEED, "Shutter Speed"},
    {DPC_BATTERY_LEVEL, "Battery Level Indicator"},
    {DPC_COLOR_TEMPERATURE, "Color Temperature"},
    {DPC_WHITE_BALANCE_GM, "White Balance - Fine-Tune G-M"},
    {DPC_ASPECT_RATIO, "Aspect Ratio"},
    {DPC_AUTO_FOCUS_STATUS, "Focus Indication"},
    {DPC_PREDICTED_MAX_FILE_SIZE, "Predicted Maximum File Size"},
    {DPC_PENDING_FILES, "Files Pending"},
    {DPC_AE_LOCK_STATUS, "AELock Indication"},
    {DPC_BATTERY_REMAINING, "Battery Remaining"},
    {DPC_PICTURE_EFFECT, "Picture Effect"},
    {DPC_WHITE_BALANCE_AB, "White Balance - Fine-Tune A-B"},
    {DPC_MOVIE_REC_STATE, "Movie Recording State"},
    {DPC_ISO, "ISO"},
    {DPC_FEL_LOCK_STATUS, "FELock Indication"},
    {DPC_LIVE_VIEW_STATUS, "Live View Status"},
    {DPC_IMAGE_SAVE_DESTINATION, "Image Save Destination"},
    {0xD223, "Date/Time Setting"},
    {DPC_FOCUS_AREA, "Focus Area"},
    {DPC_FOCUS_MAGNIFY_SCALE, "Focus Magnify Scale"},
    {DPC_FOCUS_MAGNIFY_POS, "Focus Magnify Position"},
    {DPC_LIVE_VIEW_SETTING_EFFECT, "Live View Display Effect"},
    {DPC_FOCUS_AREA_POS_OLD, "Focus Area Position"},
    {DPC_MANUAL_FOCUS_ADJUST_ENABLED, "Manual Focus Adjust Enabled"},
    {DPC_PIXEL_SHIFT_SHOOTING_MODE, "Pixel Shift Shooting Mode"},
    {DPC_PIXEL_SHIFT_SHOOTING_NUMBER, "Pixel Shift Shooting Shot Num"},
    {DPC_PIXEL_SHIFT_SHOOTING_INTERVAL, "Pixel Shift Shooting Interval"},
    {DPC_PIXEL_SHIFT_SHOOTING_STATUS, "Pixel Shift Shooting Status"},
    {DPC_PIXEL_SHIFT_SHOOTING_PROGRESS, "Pixel Shift Shooting Progress"},
    {DPC_PICTURE_PROFILE, "Picture Profile"},
    {DPC_CREATIVE_STYLE, "Creative Style"},
    {DPC_MOVIE_FILE_FORMAT, "Movie File Format"},
    {DPC_MOVIE_QUALITY, "Movie Recording Setting"},
    {DPC_MEDIA_SLOT1_STATUS, "Media Slot 1 Status"},
    {0xD249, "Media Slot 1 Remaining Shots"},
    {0xD24A, "Media Slot 1 Remaining Record Time"},
    {DPC_FOCAL_POSITION, "Focal position"},
    {DPC_AWB_LOCK_STATUS, "AWBLock Indication"},
    {DPC_INTERVAL_RECORD_MODE, "Interval Record Mode"},
    {DPC_INTERVAL_RECORD_STATUS, "Interval Record Status"},
    {DPC_DEVICE_OVERHEATING_STATE, "Device Overheating State"},
    {DPC_IMAGE_QUALITY, "Image Quality"},
    {DPC_IMAGE_FILE_FORMAT, "Image File Format"},
    {DPC_FOCUS_MAGNIFY, "Focus Magnifier"},
    {DPC_AF_TRACKING_SENS, "AF Tracking Sensitivity"},
    {DPC_MEDIA_SLOT2_STATUS, "Media Slot 2 Status"},
    {0xD257, "Media Slot 2 Remaining Shots"},
    {0xD258, "Media Slot 2 Remaining Record Time"},
    {DPC_EXPOSURE_MODE_KEY, "Exposure Mode Key"},
    {DPC_ZOOM_OPERATION_ENABLED, "Zoom Operation Enabled"},
    {DPC_ZOOM_SCALE, "Zoom Scale"},
    {DPC_ZOOM_BAR_INFO, "Zoom Bar Information"},
    {0xD25E, "Zoom Speed Range"},
    {DPC_ZOOM_SETTING, "Zoom Setting"},
    {DPC_ZOOM_TYPE_STATUS, "Zoom Type Status"},
    {DPC_WIRELESS_FLASH, "Wireless Flash Setting"},
    {DPC_RED_EYE_REDUCTION, "Red Eye Reduction"},
    {DPC_REMOTE_RESTRICT_STATUS, "Remote Control Restriction Status"},
    {0xD267, "Live View Area (x, y)"},
    {DPC_IMAGE_TRANSFER_SIZE, "Image Transfer Size"},
    {DPC_PC_SAVE_IMAGE, "RAW+J PC Save Image"},
    {DPC_LIVE_VIEW_QUALITY, "Live View Image Quality"},
    {0xD26B, "Custom WB Capturable Area (x, y)"},
    {0xD26C, "Custom WB Capture Frame Size (x, y)"},
    {0xD26D, "Custom WB Capture Standby Operation"},
    {0xD26E, "Custom WB Capture Standby Cancel Operation"},
    {0xD26F, "Custom WB Capture Operation"},
    {0xD270, "Custom WB Execution State"},
    {DPC_CAMERA_SETTING_SAVE_ENABLED, "Camera-Setting Save Operation"},
    {DPC_CAMERA_SETTING_READ_ENABLED, "Camera-Setting Read Operation"},
    {DPC_CAMERA_SETTING_SAVE_READ_STATE, "Camera-Setting Save/Read State"},
    {0xD274, "FTP-Setting Save Operation"},
    {0xD275, "FTP-Setting Read Operation"},
    {0xD276, "FTP-Setting Save/Read State"},
    {DPC_FORMAT_MEDIA_SLOT1_ENABLED, "Format Media Slot 1 Enabled"},
    {DPC_FORMAT_MEDIA_SLOT2_ENABLED, "Format Media Slot 2 Enabled"},
    {DPC_FORMAT_MEDIA_PROGRESS, "Format Media Progress Rate"},
    {0xD27C, "Select FTP Server"},
    {0xD27F, "FTP Connection Status"},
    {0xD280, "FTP Connection Error Info"},
    {0xD281, "High Resolution SS Setting"},
    {0xD282, "High Resolution Shutter Speed"},
    {DPC_TOUCH_OPERATION_FUNCTION, "Touch Operation Function"},
    {DPC_REMOTE_TOUCH_ENABLED, "Remote Touch Enabled"},
    {DPC_REMOTE_TOUCH_CANCEL_ENABLED, "Remote Touch Cancel Enabled"},
    {DPC_MOVIE_FRAME_RATE, "Movie Frame Rate"},
    {DPC_COMPRESSED_IMAGE_FILE_FORMAT, "Image Compression File Format"},
    {DPC_RAW_FILE_TYPE, "RAW File Type"},
    {0xD289, "Media Slot 1 RAW File Type"},
    {0xD28A, "Media Slot 2 RAW File Type"},
    {0xD28B, "Media Slot 1 Image File Format"},
    {0xD28C, "Media Slot 2 Image File Format"},
    {0xD28D, "Media Slot 1 Image Quality"},
    {0xD28E, "Media Slot 2 Image Quality"},
    {0xD28F, "Media Slot 1 Image Size"},
    {0xD290, "Media Slot 2 Image Size"},
    {DPC_FORMAT_MEDIA_QUICK_SLOT1_ENABLED, "Format Media Quick Slot 1 Enabled"},
    {DPC_FORMAT_MEDIA_QUICK_SLOT2_ENABLED, "Format Media Quick Slot 2 Enabled"},
    {DPC_FORMAT_MEDIA_CANCEL_ENABLED, "Format Media Cancel Enabled"},
    {DPC_CONTENTS_TRANSFER_ENABLED, "Contents Transfer Enabled"},
    {0xD297, "Save Zoom and Focus Position"},
    {0xD298, "Load Zoom and Focus Position"},
    {0xD299, "Remote Control Zoom Speed Type"},
    {0xD29A, "APS-C / Full Toggle Enabled"},
    {0xD29B, "APS-C / Full Toggle State"},
    {0xD29C, "Movie Record Self Timer"},
    {0xD29D, "Movie Record Self Timer Count Time"},
    {0xD29F, "Movie Record Self Timer Continuous"},
    {0xD2A0, "Movie Record Self Timer Status"},
    {0xD2A1, "Focus Bracket Shot Num"},
    {0xD2A2, "Focus Bracket Focus Range"},
    {0xD2A4, "Bulb Timer Setting"},
    {0xD2A5, "Bulb Exposure Time Setting"},
    {0xD2BA, "Flicker Scan Status"},
    {0xD2BB, "Flicker Scan Enabled"},
    {0xE000, "Movie Shooting Mode"},
    {0xE001, "Movie Shooting Mode Color Gamut"},
    {0xE002, "Movie Shooting Mode Target Display"},
    {0xE004, "Focus TouchSpot Status"},
    {0xE005, "Focus Tracking Status"},
    {0xE006, "Shutter ECS Setting"},
    {0xE007, "Shutter ECS Number"},
    {0xE008, "Shutter ECS Frequency"},
    {0xE009, "Depth of Field Adjustment Mode"},
    {0xE00A, "Depth of Field Adjustment Interlocking Mode State"},
    {0xE00B, "Recorder Clip Name"},
    {0xE00C, "Recorder Control Main Setting"},
    {0xE00D, "Recorder Control Proxy Setting"},
    {0xE00E, "Recorder Start Main"},
    {0xE00F, "Recorder Start Proxy"},
    {0xE010, "Recorder Main Status"},
    {0xE011, "Recorder Proxy Status"},
    {0xE012, "Recorder Ext Raw Status"},
    {0xE013, "Recorder Save Destination"},
    {0xE014, "Button Assignment Assignable.1"},
    {0xE015, "Button Assignment Assignable.2"},
    {0xE016, "Button Assignment Assignable.3"},
    {0xE017, "Button Assignment Assignable.4"},
    {0xE018, "Button Assignment Assignable.5"},
    {0xE019, "Button Assignment Assignable.6"},
    {0xE01A, "Button Assignment Assignable.7"},
    {0xE01B, "Button Assignment Assignable.8"},
    {0xE01C, "Button Assignment Assignable.9"},
    {0xE01D, "Button Assignment Assignable.10"},
    {0xE01E, "Button Assignment LensAssignable.1"},
    {0xE01F, "SceneFile Index"},
    {0xE020, "Current SceneFile Edited"},
    {0xE021, "Movie Play Button"},
    {0xE022, "Movie Play Pause Button"},
    {0xE023, "Movie Play Stop Button"},
    {0xE024, "Movie Forward Button"},
    {0xE025, "Movie Rewind Button"},
    {0xE026, "Movie Next Button"},
    {0xE027, "Movie Prev Button"},
    {0xE028, "Movie RecReview Button"},
    {0xE029, "Assignable Button 1"},
    {0xE02A, "Assignable Button 2"},
    {0xE02B, "Assignable Button 3"},
    {0xE02C, "Assignable Button 4"},
    {0xE02D, "Assignable Button 5"},
    {0xE02E, "Assignable Button 6"},
    {0xE02F, "Assignable Button 7"},
    {0xE030, "Assignable Button 8"},
    {0xE031, "Assignable Button 9"},
    {0xE032, "Assignable Button 10"},
    {0xE033, "LensAssignable Button 1"},
    {0xE035, "Assignable Button Indicator 1"},
    {0xE036, "Assignable Button Indicator 2"},
    {0xE037, "Assignable Button Indicator 3"},
    {0xE038, "Assignable Button Indicator 4"},
    {0xE039, "Assignable Button Indicator 5"},
    {0xE03A, "Assignable Button Indicator 6"},
    {0xE03B, "Assignable Button Indicator 7"},
    {0xE03C, "Assignable Button Indicator 8"},
    {0xE03D, "Assignable Button Indicator 9"},
    {0xE03E, "Assignable Button Indicator 10"},
    {0xE03F, "LensAssignable Button Indicator 1"},
    {0xE042, "Focus Position Setting"},
    {0xE043, "Focus Position Current Value"},
    {0xE050, "Audio Input Master Level"},
    {0xE059, "Audio Output HDMI Monitor CH"},
    {0xE061, "Movie Rec Button (Toggle) Enabled"},
    {0xE080, "Movie Stabilization Level"},
    {0xE082, "Movie Trimming Transfer Support Information"},
    {0xE083, "Remote Touch Operation Function"},
    {0xE084, "AF Assist"},
    {DPC_LENS_INFORMATION_ENABLED, "Lens Information Enabled"},
    {0xE088, "Follow Focus Position Setting"},
    {0xE08F, "Assignable Button Indicator 11"},
    {0xE089, "Follow Focus Position Current Value"},
    {0xE08D, "Button Assignment Assignable.11"},
    {0xE08E, "Assignable Button 11"},
};

typedef struct {
    u16 code;
    char* name;
} EventsMetadata;

static EventsMetadata sPtpEventsMetadata[] = {
    {0x4004, "StoreAdded"},
    {0x4005, "StoreRemoved"},
    {0xC201, "SDIE_ObjectAdded"},
    {0xC202, "SDIE_ObjectRemoved"},
    {0xC203, "SDIE_DevicePropChanged"},
    {0xC205, "SDIE_DateTimeSettingResult"},
    {0xC206, "SDIE_CapturedEvent"},
    {0xC208, "SDIE_CWBCapturedResult"},
    {0xC209, "SDIE_CameraSettingReadResult"},
    {0xC20A, "SDIE_FTPSettingReadResult"},
    {0xC20B, "SDIE_MediaFormatResult"},
    {0xC20C, "SDIE_FTPDisplayNameListChanged"},
    {0xC20D, "SDIE_ContentsTransferEvent"},
    {0xC20E, "SDIE_ZoomAndFocusPositionEvent"},
    {0xC20F, "SDIE_DisplayListChangedEvent"},
    {0xC210, "SDIE_MediaProfileChanged"},
    {0xC211, "SDIE_ControlJobListEvent"},
    {0xC214, "SDIE_ControlUploadDataResult"},
    {0xC218, "SDIE_FocusPositionResult"},
    {0xC21B, "SDIE_LensInformationChanged"},
    {0xC222, "SDIE_OperationResults"},
    {0xC223, "SDIE_AFStatus"},
    {0xC224, "SDIE_MovieRecOperationResults"}
};

static PTPPropValueEnum sControl_UpDown[] = {
    {.propValue.u16=0x0001, .str.str="Up", .flags=ENUM_VALUE_WRITE|ENUM_VALUE_STR_CONST},
    {.propValue.u16=0x0002, .str.str="Down", .flags=ENUM_VALUE_WRITE|ENUM_VALUE_STR_CONST},
};

static PTPPropValueEnum sControl_SelectMediaFormat[] = {
    {.propValue.u16=0x0001, .str.str="Full Format - Slot 1", .flags=ENUM_VALUE_WRITE|ENUM_VALUE_STR_CONST},
    {.propValue.u16=0x0002, .str.str="Full Format - Slot 2", .flags=ENUM_VALUE_WRITE|ENUM_VALUE_STR_CONST},
    {.propValue.u16=0x0011, .str.str="Quick Format - Slot 1", .flags=ENUM_VALUE_WRITE|ENUM_VALUE_STR_CONST},
    {.propValue.u16=0x0012, .str.str="Quick Format - Slot 2", .flags=ENUM_VALUE_WRITE|ENUM_VALUE_STR_CONST},
};

#define PROP_ENUM_SET(a) .form.enums = {.values=(a), .size=MStaticArraySize(a)}

static PtpControl sPtpControlsMetadata[] = {
    {DPC_SHUTTER_HALF_PRESS,           PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Shutter Half-Press Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SHUTTER,                      PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Shutter Release Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_AE_LOCK,                      PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "AEL Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_AFL_BUTTON,                   PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "AFL Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SHUTTER_ONE_RESET,            PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Shutter One Reset", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SHUTTER_ONE,                  PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Shutter One", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_MOVIE_RECORD,                 PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Movie Record Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FEL_BUTTON,                   PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "FEL Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_MEDIA_FORMAT,                 PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Format Media", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FOCUS_MAGNIFIER,              PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Magnifier", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FOCUS_MAGNIFIER_CANCEL,       PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Magnifier Cancel", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_REMOTE_KEY_UP,                PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Magnifier Up", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_REMOTE_KEY_DOWN,              PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Magnifier Down", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_REMOTE_KEY_LEFT,              PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Magnifier Left", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_REMOTE_KEY_RIGHT,             PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Magnifier Right", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_MANUAL_FOCUS_ADJUST,          PTP_DT_INT16,  SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_RANGE, "Manual Focus Adjust", .form.range={.min.i16=-7,.max.i16=7,.step.i16=1}},
    {DPC_AUTO_FOCUS_HOLD,              PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Autofocus Hold", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_PIXEL_SHIFT_SHOOT_CANCEL,     PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Pixel Shift Shooting Cancel", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_PIXEL_SHIFT_SHOOT,            PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Pixel Shift Shooting Mode", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_HFR_STANDBY,                  PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "HFR Standby", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_HFR_RECORD_CANCEL,            PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "HFR Record Cancel", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FOCUS_STEP_NEAR,              PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Step Near", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FOCUS_STEP_FAR,               PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Step Far", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_AWB_LOCK,                     PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "AWBL Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FOCUS_AREA_X_Y,               PTP_DT_UINT32, SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_RANGE, "AF Area Position (x, y)", .form.range={.min.u32=0,.max.u32=0xffffffff,.step.u32=1}},
    {0xD2DB,                           PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Unknown Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_ZOOM,                         PTP_DT_INT8,   SDI_CONTROL_VARIABLE, PTP_FORM_FLAG_RANGE, "Zoom Operation", .form.range={.min.i8=-1,.max.i8=1,.step.i8=1}},
    {DPC_CUSTOM_WB_CAPTURE_STANDBY,    PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Custom WB Capture Standby", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_CUSTOM_WB_CAPTURE_STANDBY_CANCEL, PTP_DT_UINT16, SDI_CONTROL_BUTTON, PTP_FORM_FLAG_ENUM, "Custom WB Capture Standby Cancel", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_CUSTOM_WB_CAPTURE,            PTP_DT_UINT32, SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_RANGE, "Custom WB Capture", .form.range={.min.u32=0,.max.u32=0xffffffff,.step.u32=1}},
    {DPC_FORMAT_MEDIA,                 PTP_DT_UINT16, SDI_CONTROL_VARIABLE, PTP_FORM_FLAG_ENUM,  "Format Media", PROP_ENUM_SET(sControl_SelectMediaFormat)},
    {DPC_REMOTE_TOUCH_XY,              PTP_DT_UINT32, SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_RANGE, "Remote Touch (x, y)", .form.range={.min.u32=0,.max.u32=0xffffffff,.step.u32=1}},
    {DPC_REMOTE_TOUCH_CANCEL,          PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Remote Touch Cancel", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SHUTTER_BOTH,                 PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "S1 & S2 Button", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FORMAT_MEDIA_CANCEL,          PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Format Media Cancel", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SAVE_ZOOM_AND_FOCUS_POSITION, PTP_DT_UINT8,  SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_ENUM,  "Save Zoom and Focus Position"},
    {DPC_LOAD_ZOOM_AND_FOCUS_POSITION, PTP_DT_UINT8,  SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_ENUM,  "Load Zoom and Focus Position"},
    {DPC_APS_C_FULL_TOGGLE,            PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "APS-C / Full Toggle", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_COLOR_TEMPERATURE_STEP,       PTP_DT_INT16,  SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_RANGE, "Color Temperature Step", .form.range={.min.i16=-30,.max.i16=30,.step.i16=1}},
    {DPC_WHITE_BALANCE_TINT_STEP,      PTP_DT_INT16,  SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_RANGE, "White Balance Tint Step", .form.range={.min.i16=-198,.max.i16=198,.step.i16=1}},
    {DPC_FOCUS_OPERATION,              PTP_DT_INT8,   SDI_CONTROL_VARIABLE, PTP_FORM_FLAG_RANGE, "Focus Operation", .form.range={.min.i8=-1,.max.i8=1,.step.i8=1}},
    {DPC_FLICKER_SCAN,                 PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Flicker Scan", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SETTINGS_RESET,               PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Settings Reset", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_PIXEL_MAPPING,                PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Pixel Mapping", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_POWER_OFF,                    PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Power Off", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_TIME_CODE_PRESET_RESET,       PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Time Code Preset Reset", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_USER_BIT_PRESET_RESET,        PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "User Bit Preset Reset", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SENSOR_CLEANING,              PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Sensor Cleaning", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_RESET_PICTURE_PROFILE,        PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Reset Picture Profile", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_RESET_CREATIVE_LOOK,          PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Reset Creative Look", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_SHUTTER_ECS_NUMBER_STEP,      PTP_DT_INT16,  SDI_CONTROL_NOTCH,    PTP_FORM_FLAG_ENUM,  "Shutter ECS Number Step", .form.range={.min.i16=-32768,.max.i16=32767,.step.i16=1}},
    {DPC_MOVIE_RECORD_TOGGLE,          PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Movie Record Toggle", PROP_ENUM_SET(sControl_UpDown)},
    {DPC_FOCUS_POSITION_CANCEL,        PTP_DT_UINT16, SDI_CONTROL_BUTTON,   PTP_FORM_FLAG_ENUM,  "Focus Position Cancel", PROP_ENUM_SET(sControl_UpDown)},
};

char* PTP_GetPropertyStr(u16 propCode) {
    for (int i = 0; i < MStaticArraySize(sPtpPropertiesMetadata); i++) {
        if (sPtpPropertiesMetadata[i].code == propCode) {
            return sPtpPropertiesMetadata[i].name;
        }
    }

    return NULL;
}

char* PTP_GetControlStr(u16 controlCode) {
    for (int i = 0; i < MStaticArraySize(sPtpControlsMetadata); i++) {
        if (sPtpControlsMetadata[i].controlCode == controlCode) {
            return sPtpControlsMetadata[i].name;
        }
    }

    return NULL;
}

char* PTP_GetEventStr(u16 eventCode) {
    for (int i = 0; i < MStaticArraySize(sPtpEventsMetadata); i++) {
        if (sPtpEventsMetadata[i].code == eventCode) {
            return sPtpEventsMetadata[i].name;
        }
    }

    return NULL;
}


typedef struct {
    u16 code;
    char* name;
} ObjectFormatMetadata;

static ObjectFormatMetadata sPtpObjectFormatMetadata[] = {
    {PTP_OFC_FOLDER, "Folder"},
    {PTP_OFC_TEXT, "TEXT"},
    {PTP_OFC_MPEG, "MPEG"},
    {PTP_OFC_JPEG, "JPEG"},
    {PTP_OFC_JFIF, "JFIF"},
    {PTP_OFC_RAW, "RAW"},
    {PTP_OFC_HEIF, "HEIF"},
    {PTP_OFC_MPO, "MPO"},
    {PTP_OFC_WMA, "WMA"},
    {PTP_OFC_MP4, "MP4"},
};

char* PTP_GetObjectFormatStr(u16 objectFormatCode) {
    for (int i = 0; i < MStaticArraySize(sPtpObjectFormatMetadata); i++) {
        if (sPtpObjectFormatMetadata[i].code == objectFormatCode) {
            return sPtpObjectFormatMetadata[i].name;
        }
    }

    return NULL;
}

typedef struct {
    u16 code;
    char* name;
    char* description;
} OperationMetadata;

static OperationMetadata sPtpOperationMetadata[] = {
    {PTP_OC_GetDeviceInfo, "GetDeviceInfo", NULL},
    {PTP_OC_OpenSession, "OpenSession", NULL},
    {PTP_OC_CloseSession, "CloseSession", NULL},
    {PTP_OC_GetStorageID, "GetStorageIDs", NULL},
    {PTP_OC_GetStorageInfo, "GetStorageInfo", NULL},
    {PTP_OC_GetNumObjects, "GetNumObjects", NULL},
    {PTP_OC_GetObjectHandles, "GetObjectHandles", NULL},
    {PTP_OC_GetObjectInfo, "GetObjectInfo", NULL},
    {PTP_OC_GetObject, "GetObject", NULL},
    {PTP_OC_GetThumb, "GetThumb", NULL},
    {PTP_OC_DeleteObject, "DeleteObject", NULL},
    {0x100C, "SendObjectInfo", NULL},
    {PTP_OC_SendObject, "SendObject", NULL},
    {0x100E, "InitiateCapture", NULL},
    {0x100F, "FormatStore", NULL},
    {0x1010, "ResetDevice", NULL},
    {0x1011, "SelfTest", NULL},
    {0x1012, "SetObjectProtection", NULL},
    {0x1013, "PowerDown", NULL},
    {0x1014, "GetDevicePropDesc", NULL},
    {0x1015, "GetDevicePropValue", NULL},
    {0x1016, "SetDevicePropValue", NULL},
    {0x1017, "ResetDevicePropValue", NULL},
    {0x1018, "TerminateOpenCapture", NULL},
    {0x1019, "MoveObject", NULL},
    {0x101A, "CopyObject", NULL},
    {0x101B, "GetPartialObject", NULL},
    {0x101C, "InitiateOpenCapture", NULL},
    {0x9801, "GetObjectPropsSupported", "same as Media Transfer Protocol v.1.1 Spec"},
    {0x9802, "GetObjectPropDesc", "same as Media Transfer Protocol v.1.1 Spec"},
    {0x9803, "GetObjectPropValue", "same as Media Transfer Protocol v.1.1 Spec"},
    {0x9804, "SetObjectPropValue", "same as Media Transfer Protocol v.1.1 Spec"},
    {0x9805, "GetObjectPropList", "same as Media Transfer Protocol v.1.1 Spec"},
    {PTP_OC_SDIO_Connect, "SDIO_Connect", "This is for the authentication handshake."},
    {PTP_OC_SDIO_GetExtDeviceInfo, "SDIO_GetExtDeviceInfo", "Get the protocol version and the supported properties "
        "of the connected device."},
    {PTP_OC_SDIO_SetExtDevicePropValue, "SDIO_SetExtDevicePropValue", "Set a DevicePropValue for a device property."},
    {PTP_OC_SDIO_ControlDevice, "SDIO_ControlDevice", "Set the SDIControl value for the SDIControlCode."},
    {PTP_OC_SDIO_GetAllExtDevicePropInfo, "SDIO_GetAllExtDevicePropInfo", "Obtain all support DevicePropDescs at one time. "
        "The host will send this operation at regular intervals to obtain the latest (current) camera settings."},
    {PTP_OC_SDIO_SetFTPSettingFilePassword, "SDIO_SetFTPSettingFilePassword", "Set the password for getting/setting a "
        "FTP-Setting File."},
    {PTP_OC_SDIO_OpenSession, "SDIO_OpenSession", "Open Session with Function Mode."},
    {PTP_OC_SDIO_GetPartialLargeObject, "SDIO_GetPartialLargeObject", "Get partial object from the device."},
    {PTP_OC_SDIO_SetContentsTransferMode, "SDIO_SetContentsTransferMode", "Turn on/off content transfer mode."},
    {PTP_OC_SDIO_GetDisplayStringList, "SDIO_GetDisplayStringList", "Get Display String List."},
    {PTP_OC_SDIO_GetVendorCodeVersion, "SDIO_GetVendorCodeVersion", "Get vendor code version."},
    {PTP_OC_SDIO_GetFTPJobList, "SDIO_GetFTPJobList", "Get the FTP Job List."},
    {PTP_OC_SDIO_ControlFTPJobList, "SDIO_ControlFTPJobList", "Control the FTP Job List."},
    {PTP_OC_SDIO_UploadData, "SDIO_UploadData", "Upload data to Camera temporary storage."},
    {PTP_OC_SDIO_ControlUploadData, "SDIO_ControlUploadData", "Control the Upload Data."},
    {PTP_OC_SDIO_GetFTPSettingList, "SDIO_GetFTPSettingList", "Get FTP Setting List."},
    {PTP_OC_SDIO_SetFTPSettingList, "SDIO_SetFTPSettingList", "Set FTP Setting List."},
    {PTP_OC_SDIO_GetLensInformation, "SDIO_GetLensInformation", "Get Lens Information."},
    {PTP_OC_SDIO_OperationResultsSupported, "SDIO_OperationResultsSupported", "Get the Operation Results Supported."},
};

char* PTP_GetOperationStr(u16 operationCode) {
    for (int i = 0; i < MStaticArraySize(sPtpOperationMetadata); i++) {
        if (sPtpOperationMetadata[i].code == operationCode) {
            return sPtpOperationMetadata[i].name;
        }
    }

    return NULL;
}

char* PTP_GetDataTypeStr(PTPDataType dataType) {
    switch (dataType) {
        case PTP_DT_UNDEF:
            return "undef";
        case PTP_DT_INT8:
            return "i8";
        case PTP_DT_UINT8:
            return "u8";
        case PTP_DT_INT16:
            return "i16";
        case PTP_DT_UINT16:
            return "u16";
        case PTP_DT_INT32:
            return "i32";
        case PTP_DT_UINT32:
            return "u32";
        case PTP_DT_INT64:
            return "i64";
        case PTP_DT_UINT64:
            return "u64";
        case PTP_DT_INT128:
            return "i128";
        case PTP_DT_UINT128:
            return "u128";
        case PTP_DT_AINT8:
            return "[i8]";
        case PTP_DT_AUINT8:
            return "[u8]";
        case PTP_DT_AINT16:
            return "[i16]";
        case PTP_DT_AUINT16:
            return "[u16]";
        case PTP_DT_AINT32:
            return "[i32]";
        case PTP_DT_AUINT32:
            return "[u32]";
        case PTP_DT_AINT64:
            return "[i64]";
        case PTP_DT_AUINT64:
            return "[u64]";
        case PTP_DT_AINT128:
            return "[i128]";
        case PTP_DT_AUINT128:
            return "[u128]";
        case PTP_DT_STR:
            return "string";
    }
    return NULL;
}

char* PTP_GetFormFlagStr(PTPFormFlag formFlag) {
    switch (formFlag) {
        case PTP_FORM_FLAG_NONE:
            return "";
        case PTP_FORM_FLAG_RANGE:
            return "Range";
        case PTP_FORM_FLAG_ENUM:
            return "Enum";
    }
    return NULL;
}

char* PTP_GetPropIsEnabledStr(u8 propIsEnabled) {
    switch (propIsEnabled) {
        case 0x0:
            return "N/A";
        case 0x1:
            return "RW";
        case 0x2:
            return "RO";
    }
    return NULL;
}

void PTP_GetPropValueStr(PTPDataType dataType, PTPPropValue value, char* buffer, size_t bufferLen) {
    switch (dataType) {
        case PTP_DT_INT8:
            snprintf(buffer, bufferLen, " %hhd (%02hhx)",  value.i8, value.i8);
            break;
        case PTP_DT_UINT8:
            snprintf(buffer, bufferLen, " %hhu (%02hhx)",  value.u8, value.u8);
            break;
        case PTP_DT_INT16:
            snprintf(buffer, bufferLen, " %hd (%04hx)", value.i16, value.i16);
            break;
        case PTP_DT_UINT16:
            snprintf(buffer, bufferLen, " %hu (%04hx)", value.u16, value.u16);
            break;
        case PTP_DT_INT32:
            snprintf(buffer, bufferLen, " %d (%08x)", value.i32, value.i32);
            break;
        case PTP_DT_UINT32:
            snprintf(buffer, bufferLen, " %u (%08x)", value.u32, value.u32);
            break;
        case PTP_DT_INT64:
            snprintf(buffer, bufferLen, " %lld (%016llx)", value.i64, value.i64);
           break;
        case PTP_DT_UINT64:
            snprintf(buffer, bufferLen, " %llu (%016llx)", value.u64, value.u64);
            break;
        case PTP_DT_INT128:
            break;
        case PTP_DT_UINT128:
            break;
        case PTP_DT_AINT8:
            break;
        case PTP_DT_AUINT8:
            break;
        case PTP_DT_AINT16:
            break;
        case PTP_DT_AUINT16:
            break;
        case PTP_DT_AINT32:
            break;
        case PTP_DT_AUINT32:
            break;
        case PTP_DT_AINT64:
            break;
        case PTP_DT_AUINT64:
            break;
        case PTP_DT_AINT128:
            break;
        case PTP_DT_AUINT128:
            break;
        case PTP_DT_STR:
            snprintf(buffer, bufferLen, " %s", value.str.str);
            break;
        default:
            break;
    }
}

b32 PTP_PropValueEq(PTPDataType dataType, PTPPropValue value1, PTPPropValue value2) {
    switch (dataType) {
        case PTP_DT_INT8:
            return value1.i8 == value2.i8;
        case PTP_DT_UINT8:
            return value1.u8 == value2.u8;
        case PTP_DT_INT16:
            return value1.i16 == value2.i16;
        case PTP_DT_UINT16:
            return value1.u16 == value2.u16;
        case PTP_DT_INT32:
            return value1.i32 == value2.i32;
        case PTP_DT_UINT32:
            return value1.u32 == value2.u32;
        case PTP_DT_INT64:
            return value1.i64 == value2.i64;
        case PTP_DT_UINT64:
            return value1.u64 == value2.u64;
        case PTP_DT_INT128:
            return MStrCmp(value1.i128, value2.i128) == 0;
        case PTP_DT_UINT128:
            return MStrCmp(value1.i128, value2.i128) == 0;
        case PTP_DT_AINT8:
            break;
        case PTP_DT_AUINT8:
            break;
        case PTP_DT_AINT16:
            break;
        case PTP_DT_AUINT16:
            break;
        case PTP_DT_AINT32:
            break;
        case PTP_DT_AUINT32:
            break;
        case PTP_DT_AINT64:
            break;
        case PTP_DT_AUINT64:
            break;
        case PTP_DT_AINT128:
            break;
        case PTP_DT_AUINT128:
            break;
        case PTP_DT_STR:
            return MStrCmp(value1.str.str, value2.str.str) == 0;
        default:
            break;
    }
    return FALSE;
}

size_t Ptp_PropValueSize(PTPDataType dataType, PTPPropValue value) {
    switch (dataType) {
        case PTP_DT_INT8:
        case PTP_DT_UINT8:
            return 1;
        case PTP_DT_INT16:
        case PTP_DT_UINT16:
            return 2;
        case PTP_DT_INT32:
        case PTP_DT_UINT32:
            return 4;
        case PTP_DT_INT64:
        case PTP_DT_UINT64:
            return 8;
        case PTP_DT_INT128:
        case PTP_DT_UINT128:
            return 16;
        case PTP_DT_AINT8:
            break;
        case PTP_DT_AUINT8:
            break;
        case PTP_DT_AINT16:
            break;
        case PTP_DT_AUINT16:
            break;
        case PTP_DT_AINT32:
            break;
        case PTP_DT_AUINT32:
            break;
        case PTP_DT_AINT64:
            break;
        case PTP_DT_AUINT64:
            break;
        case PTP_DT_AINT128:
            break;
        case PTP_DT_AUINT128:
            break;
        case PTP_DT_STR: {
            size_t len = MStrLen(value.str.str);
            if (len == 0) {
                return 0;
            } else {
                return len + 2;
            }
            break;
        }
        default:
            return 0;
    }
    return 0;
}

void PropValueFree(PTPDataType dataType, PTPPropValue* value) {
    if (value == NULL) {
        return;
    }
    if (dataType == PTP_DT_STR) {
        MStrFree(value->str);
    }
}

typedef struct {
    u32 storageID;
    u16 objectFormat; // ObjectFormatMetadata
    u16 protectionStatus;
    u32 objectCompressedSize;
    u16 thumbFormat;
    u32 thumbCompressedSize;
    u32 thumbPixWidth;
    u32 thumbPixHeight;
    u32 imagePixWidth;
    u32 imagePixHeight;
    u32 imagePixDepth;
    u32 parentObject;
    u16 associationType;
    u32 associationDesc;
    u32 sequenceNumber;

    MStr filename;
    MStr captureDateTime;
    MStr modDateTime;
    MStr keywords;
} PTPObjectInfo;

void Ptp_FreeObjectInfo(PTPObjectInfo *objectInfo) {
    MStrFree(objectInfo->filename);
    MStrFree(objectInfo->captureDateTime);
    MStrFree(objectInfo->modDateTime);
    MStrFree(objectInfo->keywords);
}

void PTP_FreeLiveViewFrames(LiveViewFrames* liveViewFrames) {
    MArrayFree(liveViewFrames->focus.frames);
    MArrayFree(liveViewFrames->face.frames);
    MArrayFree(liveViewFrames->tracking.frames);
}

void PTP_FreePropValueEnums(PTPPropValueEnums* outEnums) {
    for (int i = 0; i < MArraySize(outEnums->values); ++i) {
        if (outEnums->values[i].str.size) {
            MStrFree(outEnums->values[i].str);
        }
    }
    MArrayFree(outEnums->values);
}

static MStr ReadPtpString8(MMemIO* memIo) {
    MStr r = {};
    u8 len = 0;
    u16 buffer[256];

    MMemReadU8(memIo, &len);

    if (len) {
        for (size_t i = 0; i < len; i++) {
            MMemReadU16LE(memIo, buffer + i);
        }

        size_t utf8Len = UTF8_GetConvertUTF16Len(buffer, len);
        if (utf8Len) {
            MStrInit(r, utf8Len);
            if (UTF8_ConvertFromUTF16(buffer, len, r.str, r.size) == 0) {
                MStrFree(r);
                return r;
            }
        }
    }
    return r;
}

static char* ReadPtpString16(MMemIO* memIo) {
    u16 utf8Len = 0;
    MMemReadU16(memIo, &utf8Len);
    if (utf8Len) {
        char* utf8 = MMalloc(utf8Len);
        MMemReadCharCopyN(memIo, utf8, utf8Len);
        return utf8;
    }
    return NULL;
}

static i32 ReadPropertyValue(MMemIO* memIo, u16 dataType, PTPPropValue* value) {
    i32 r = 0;
    switch (dataType) {
        case PTP_DT_INT8:
            r = MMemReadI8(memIo, &value->i8);
            break;
        case PTP_DT_UINT8:
            r = MMemReadU8(memIo, &value->u8);
            break;
        case PTP_DT_INT16:
            r = MMemReadI16LE(memIo, &value->i16);
            break;
        case PTP_DT_UINT16:
            r = MMemReadU16LE(memIo, &value->u16);
            break;
        case PTP_DT_INT32:
            r = MMemReadI32LE(memIo, &value->i32);
            break;
        case PTP_DT_UINT32:
            r = MMemReadU32LE(memIo, &value->u32);
            break;
        case PTP_DT_INT64:
            r = MMemReadI64LE(memIo, &value->i64);
            break;
        case PTP_DT_UINT64:
            r = MMemReadU64LE(memIo, &value->u64);
            break;
        case PTP_DT_INT128:
            break;
        case PTP_DT_UINT128:
            break;
        case PTP_DT_AINT8:
            break;
        case PTP_DT_AUINT8:
            break;
        case PTP_DT_AINT16:
            break;
        case PTP_DT_AUINT16:
            break;
        case PTP_DT_AINT32:
            break;
        case PTP_DT_AUINT32:
            break;
        case PTP_DT_AINT64:
            break;
        case PTP_DT_AUINT64:
            break;
        case PTP_DT_AINT128:
            break;
        case PTP_DT_AUINT128:
            break;
        case PTP_DT_STR:
            value->str = ReadPtpString8(memIo);
            break;
        default:
            break;
    }
    return r;
}

static void PrintPropertyValue(u16 dataType, PTPPropValue* value) {
    switch (dataType) {
        case PTP_DT_INT8:
            MLogf(" %d (%02x)", value->i8, value->i8);
            break;
        case PTP_DT_UINT8:
            MLogf(" %d (%02x)", value->u8, value->u8);
            break;
        case PTP_DT_INT16:
            MLogf(" %d (%04x)", value->i16, value->i16);
            break;
        case PTP_DT_UINT16:
            MLogf(" %d (%04x)", value->u16, value->u16);
            break;
        case PTP_DT_INT32:
            MLogf(" %d (%08x)",  value->i32, value->i32);
            break;
        case PTP_DT_UINT32:
            MLogf(" %d (%08x)", value->u32, value->u32);
            break;
        case PTP_DT_INT64:
            MLogf(" %d (%016llx)",  value->i64, value->i64);
            break;
        case PTP_DT_UINT64:
            MLogf(" %d (%016llx)", value->u64, value->i64);
            break;
        case PTP_DT_INT128:
            MLogf(" %d (%03ll2x)", value->i128, value->i128);
            break;
        case PTP_DT_UINT128:
            MLogf(" %d (%032llx)", value->u128, value->u128);
            break;
        case PTP_DT_AINT8:
            break;
        case PTP_DT_AUINT8:
            break;
        case PTP_DT_AINT16:
            break;
        case PTP_DT_AUINT16:
            break;
        case PTP_DT_AINT32:
            break;
        case PTP_DT_AUINT32:
            break;
        case PTP_DT_AINT64:
            break;
        case PTP_DT_AUINT64:
            break;
        case PTP_DT_AINT128:
            break;
        case PTP_DT_AUINT128:
            break;
        case PTP_DT_STR:
            MLogf(" %s", value->str);
           break;
        default:
            break;
    }
}

static void PrintProperties(PTPControl* self) {
    size_t numProperties = MArraySize(self->properties);
    for (int i = 0; i < numProperties; i++) {
        PTPProperty* p = self->properties + i;
        MLogf("Property Code: %04x GetSet: %02x IsEnabled: %02x Type: %04x Form: %02x",
            p->propCode, p->getSet, p->isEnabled, p->dataType, p->formFlag);

        MLog("Default Value:");
        PrintPropertyValue(p->dataType, &p->defaultValue);

        MLog("Value:");
        PrintPropertyValue(p->dataType, &p->value);

        switch (p->formFlag) {
            case PTP_FORM_FLAG_ENUM: {
                    size_t numSetValues = MArraySize(p->form.enums.set);
                    if (numSetValues > 0) {
                        MLog("Set Values:");
                        for (int j = 0; j < numSetValues; j++) {
                            PrintPropertyValue(p->dataType, p->form.enums.set + j);
                        }
                    }
                    size_t numGetSetValues = MArraySize(p->form.enums.getSet);
                    if (numGetSetValues > 0) {
                        MLog("Get/Set Values:");
                        for (int j = 0; j < numGetSetValues; j++) {
                            PrintPropertyValue(p->dataType, p->form.enums.getSet + j);
                        }
                    }
                }
                break;
            case PTP_FORM_FLAG_RANGE:
                MLog("Range Min:");
                PrintPropertyValue(p->dataType, &p->form.range.min);
                MLog("Range Max:");
                PrintPropertyValue(p->dataType, &p->form.range.max);
                MLog("Range Step Size:");
                PrintPropertyValue(p->dataType, &p->form.range.step);
                break;
        }
    }
}

void PTPControl_InitDataBuffers(PTPControl* self, size_t dataInSize, size_t dataOutSize) {
    if (dataInSize > self->dataInCapacity || self->dataInMem == NULL) {
        void* mem = self->deviceTransport->reallocBuffer(self->deviceTransport, PTP_BUFFER_IN,
                                                         self->dataInMem, self->dataInCapacity,
                                                         dataInSize);
        self->dataInCapacity = dataInSize;
        self->dataInMem = mem;
    }

    if (dataOutSize > self->dataOutCapacity || self->dataOutMem == NULL) {
        void* mem = self->deviceTransport->reallocBuffer(self->deviceTransport, PTP_BUFFER_OUT,
                                                         self->dataOutMem, self->dataOutCapacity,
                                                         dataOutSize);
        self->dataOutCapacity = dataOutSize;
        self->dataOutMem = mem;
    }

    self->dataInSize = dataInSize;
    self->dataOutSize = dataOutSize;
    memset(self->dataInMem, 0, dataInSize);
    if (dataOutSize) {
        memset(self->dataOutMem, 0, dataOutSize);
    }
}

void PTPControl_FreeDataBuffers(PTPControl* self) {
    self->deviceTransport->freeBuffer(self, PTP_BUFFER_IN, self->dataInMem, self->dataInCapacity);
    self->dataInMem = NULL;
    self->dataInCapacity = 0;
    self->deviceTransport->freeBuffer(self, PTP_BUFFER_OUT, self->dataOutMem, self->dataOutCapacity);
    self->dataOutMem = NULL;
    self->dataOutCapacity = 0;
}

static PTPRequestHeader BuildReq(PTPControl* self, size_t dataInExtra, size_t dataOutExtra, u16 opCode) {
    u32 dataInSize = dataInExtra;
    u32 dataOutSize = dataOutExtra;
    PTPControl_InitDataBuffers(self, dataInSize, dataOutSize);
    PTPRequestHeader r = {
        .OpCode = opCode,
        .NextPhase = PTP_NEXT_PHASE_READ_DATA,
        .SessionId = self->sessionId,
        .TransactionId = self->transactionId++
    };
    return r;
}

typedef struct {
    PTPResult result;
    PTPResponseHeader* dataOut;
    MMemIO memIo;
} PTPResponse;

#define OK(a) ((a) == PTP_OK)
#define RETURN_IF_FAIL(r) if ((r).result != PTP_OK || (r).memIo.mem == NULL || (r).memIo.size == 0) { return (r).result; }

static PTPResponse SendReq(PTPControl* self, PTPRequestHeader* request) {
    size_t actualDataOutSize = 0;
    PTPResult r = self->deviceTransport->sendAndRecvEx(self->deviceTransport,
        request, self->dataInMem, self->dataInSize,
        &self->ptpResponse, self->dataOutMem, self->dataOutSize,
        &actualDataOutSize);

    PTPResponse response = {.result=r};
    if (r != PTP_OK) {
        return response;
    }

    response.dataOut = &self->ptpResponse;
    response.result = response.dataOut->ResponseCode;
    if (actualDataOutSize > sizeof(PTPResponseHeader)) {
        MMemReadInit(&response.memIo, self->dataOutMem, actualDataOutSize);
    } else {
        response.memIo.mem = NULL;
        response.memIo.pos = NULL;
        response.memIo.size = 0;
        response.memIo.capacity = 0;
    }

    return response;
}

static PTPResponse DoRequest(PTPControl* self, u16 opCode, size_t dataInExtra, size_t dataOutExtra, int numParams, ...) {
    PTPRequestHeader req = BuildReq(self, dataInExtra, dataOutExtra, opCode);

    va_list vargs;
    va_start(vargs, numParams);
    for (int i = 0; i < numParams; ++i) {
        req.Params[i] = va_arg(vargs, int);
    }
    va_end(vargs);

    req.NumParams = numParams;
    return SendReq(self, &req);
}

static PTPResult OpenSession(PTPControl* self, u32 sessionId) {
    PTPResponse r = DoRequest(self, PTP_OC_OpenSession, 0, 8, 1, sessionId);
    RETURN_IF_FAIL(r);
    return r.result;
}

static PTPResult CloseSession(PTPControl* self) {
    PTPResponse r = DoRequest(self, PTP_OC_CloseSession, 0, 8, 0);
    RETURN_IF_FAIL(r);
    return r.result;
}

static PTPResult SDIO_Connect(PTPControl* self, u32 phase, u32 connectionId) {
    PTPResponse r = DoRequest(self, PTP_OC_SDIO_Connect, 0, 8,
                              3, phase, connectionId, connectionId);
    RETURN_IF_FAIL(r);
    return r.result;
}

static PTPResult PTP_GetDeviceInfo(PTPControl* self) {
    PTPResponse r = DoRequest(self, PTP_OC_GetDeviceInfo, 0, 0x1000, 0);
    RETURN_IF_FAIL(r);

    u16 standardVersion = 0;
    MMemReadU16LE(&r.memIo, &standardVersion);
    self->standardVersion = standardVersion;

    u32 vendorExtensionId = 0;
    MMemReadU32LE(&r.memIo, &vendorExtensionId);
    self->vendorExtensionId = vendorExtensionId;

    u16 vendorExtensionVersion = 0;
    MMemReadU16LE(&r.memIo, &vendorExtensionVersion);
    self->vendorExtensionVersion = vendorExtensionVersion;

    self->vendorExtension = ReadPtpString8(&r.memIo);

    u16 functionalMode = 0;
    MMemReadU16LE(&r.memIo, &functionalMode);

    u32 operationsLen = 0;
    MMemReadU32LE(&r.memIo, &operationsLen);
    for (int i = 0; i < operationsLen; i++) {
        u16 operation = 0;
        MMemReadU16LE(&r.memIo, &operation);
        MArrayAdd(self->supportedOperations, operation);
    }

    u32 eventsLen = 0;
    MMemReadU32LE(&r.memIo, &eventsLen);
    for (int i = 0; i < eventsLen; i++) {
        u16 event = 0;
        MMemReadU16LE(&r.memIo, &event);
        MArrayAdd(self->supportedEvents, event);
    }

    u32 propertiesSupportedLen = 0;
    MMemReadU32LE(&r.memIo, &propertiesSupportedLen);
    for (int i = 0; i < propertiesSupportedLen; i++) {
        u16 devicePropertyCode = 0;
        MMemReadU16LE(&r.memIo, &devicePropertyCode);
        MArrayAdd(self->supportedProperties, devicePropertyCode);
    }

    u32 captureFormatsLen = 0;
    MMemReadU32LE(&r.memIo, &captureFormatsLen);
    for (int i = 0; i < captureFormatsLen; i++) {
        u16 captureFormat = 0;
        MMemReadU16LE(&r.memIo, &captureFormat);
        MArrayAdd(self->captureFormats, captureFormat);
    }

    u32 imageFormatsLen = 0;
    MMemReadU32LE(&r.memIo, &imageFormatsLen);
    for (int i = 0; i < imageFormatsLen; i++) {
        u16 imageFormat = 0;
        MMemReadU16LE(&r.memIo, &imageFormat);
        MArrayAdd(self->imageFormats, imageFormat);
    }

    self->manufacturer = ReadPtpString8(&r.memIo);
    self->model = ReadPtpString8(&r.memIo);
    self->deviceVersion = ReadPtpString8(&r.memIo);
    self->serialNumber = ReadPtpString8(&r.memIo);

    return r.result;
}

static void SDIO_ProcessDeviceProperties200(PTPControl *self, b32 incremental, PTPResponse r, u64 numProperties) {
    if (!incremental) {
        MArrayInit(self->properties, 0);
        MArrayInit(self->controls, 0);
    }

    for (int i = 0; i < numProperties; i++) {
        u16 propCode = 0;
        MMemReadU16LE(&r.memIo, &propCode);

        if (PTPControl_SupportsControl(self, propCode)) {
            // Handle control
            PtpControl *control = NULL;
            if (incremental) {
                control = PTPControl_GetControl(self, propCode);
            }
            if (!control) {
                control = MArrayAddPtr(self->controls);
                memset(control, 0, sizeof(PtpControl));
                control->controlCode = propCode;

                size_t numControlsMeta = MStaticArraySize(sPtpControlsMetadata);
                for (int j = 0; j < numControlsMeta; j++) {
                    PtpControl* controlDesc = sPtpControlsMetadata + j;
                    if (controlDesc->controlCode == propCode) {
                        control->name = controlDesc->name;
                        break;
                    }
                }
            }

            MMemReadU16LE(&r.memIo, &control->dataType);
            u8 getSet = 0;
            MMemReadU8(&r.memIo, &getSet);
            u8 isEnabled = 0;
            MMemReadU8(&r.memIo, &isEnabled);

            PTPPropValue dummy;
            ReadPropertyValue(&r.memIo, control->dataType, &dummy);
            ReadPropertyValue(&r.memIo, control->dataType, &dummy);

            MMemReadU8(&r.memIo, &control->formFlag);

            if (control->formFlag == PTP_FORM_FLAG_ENUM) {
                u16 numEnumSet = 0;
                MMemReadU16LE(&r.memIo, &numEnumSet);
                MArrayInit(control->form.enums.values, numEnumSet);
                memset(control->form.enums.values, 0, numEnumSet * sizeof(PtpControl));
                for (int j = 0; j < numEnumSet; j++) {
                    PTPPropValueEnum *value = MArrayAddPtr(control->form.enums.values);
                    ReadPropertyValue(&r.memIo, control->dataType, &value->propValue);
                }
                control->form.enums.size = numEnumSet;
                control->form.enums.owned = TRUE;
            } else if (control->formFlag == PTP_FORM_FLAG_RANGE) {
                ReadPropertyValue(&r.memIo, control->dataType, &control->form.range.min);
                ReadPropertyValue(&r.memIo, control->dataType, &control->form.range.max);
                ReadPropertyValue(&r.memIo, control->dataType, &control->form.range.step);
            }
        } else {
            // Handle property
            PTPProperty *property = NULL;
            if (incremental) {
                property = PTPControl_GetProperty(self, propCode);
            }
            if (!property) {
                property = MArrayAddPtr(self->properties);
                memset(property, 0, sizeof(PTPProperty));
                property->propCode = propCode;
            }

            MMemReadU16LE(&r.memIo, &property->dataType);
            MMemReadU8(&r.memIo, &property->getSet);
            MMemReadU8(&r.memIo, &property->isEnabled);

            ReadPropertyValue(&r.memIo, property->dataType, &property->defaultValue);
            ReadPropertyValue(&r.memIo, property->dataType, &property->value);

            MMemReadU8(&r.memIo, &property->formFlag);

            if (property->formFlag == PTP_FORM_FLAG_ENUM) {
                u16 numEnumSet = 0;
                MMemReadU16LE(&r.memIo, &numEnumSet);
                MArrayInit(property->form.enums.set, numEnumSet);
                for (int j = 0; j < numEnumSet; j++) {
                    PTPPropValue* value = MArrayAddPtr(property->form.enums.set);
                    ReadPropertyValue(&r.memIo, property->dataType, value);
                }
            } else if (property->formFlag == PTP_FORM_FLAG_RANGE) {
                ReadPropertyValue(&r.memIo, property->dataType, &property->form.range.min);
                ReadPropertyValue(&r.memIo, property->dataType, &property->form.range.max);
                ReadPropertyValue(&r.memIo, property->dataType, &property->form.range.step);
            }

            // On older pre-2020 cameras, some properties can only be adjusted up or down, mark them as such with
            // 'isNotch', client code can change these properties with PTPControl_SetPropertyNotch()
            switch (propCode) {
                case DPC_F_NUMBER:
                case DPC_EXPOSURE_COMPENSATION:
                case DPC_FLASH_COMPENSATION:
                case DPC_SHUTTER_SPEED:
                case DPC_ISO:
                    property->isNotch = TRUE;
                    break;
                default:
                    property->isNotch = FALSE;
                    break;
            }
        }
    }
}

static void SDIO_ProcessDeviceProperties300(PTPControl *self, b32 incremental, PTPResponse r, u64 numProperties) {
    if (!incremental) {
        MArrayInit(self->properties, numProperties);
        memset(self->properties, 0, numProperties * sizeof(PTPProperty));
    }

    for (int i = 0; i < numProperties; i++) {
        u16 propCode = 0;
        MMemReadU16LE(&r.memIo, &propCode);

        PTPProperty *property = NULL;
        if (incremental) {
            property = PTPControl_GetProperty(self, propCode);
        }
        if (!property) {
            property = MArrayAddPtr(self->properties);
            property->propCode = propCode;
        }

        MMemReadU16LE(&r.memIo, &property->dataType);
        MMemReadU8(&r.memIo, &property->getSet);
        MMemReadU8(&r.memIo, &property->isEnabled);

        ReadPropertyValue(&r.memIo, property->dataType, &property->defaultValue);
        ReadPropertyValue(&r.memIo, property->dataType, &property->value);

        MMemReadU8(&r.memIo, &property->formFlag);

        if (property->formFlag == PTP_FORM_FLAG_ENUM) {
            u16 numEnumSet = 0;
            MMemReadU16LE(&r.memIo, &numEnumSet);
            MArrayInit(property->form.enums.set, numEnumSet);
            for (int j = 0; j < numEnumSet; j++) {
                PTPPropValue* value = MArrayAddPtr(property->form.enums.set);
                ReadPropertyValue(&r.memIo, property->dataType, value);
            }
            u16 numEnumGetSet = 0;
            MMemReadU16LE(&r.memIo, &numEnumGetSet);
            MArrayInit(property->form.enums.getSet, numEnumGetSet);
            for (int j = 0; j < numEnumGetSet; j++) {
                PTPPropValue* value = MArrayAddPtr(property->form.enums.getSet);
                ReadPropertyValue(&r.memIo, property->dataType, value);
            }
        } else if (property->formFlag == PTP_FORM_FLAG_RANGE) {
            ReadPropertyValue(&r.memIo, property->dataType, &property->form.range.min);
            ReadPropertyValue(&r.memIo, property->dataType, &property->form.range.max);
            ReadPropertyValue(&r.memIo, property->dataType, &property->form.range.step);
        }
    }
}

static PTPResult SDIO_GetAllExtDevicePropInfo(PTPControl* self, b32 incremental, b32 addExtended) {
    PTPRequestHeader req = BuildReq(self, 0, 64 * 1024, PTP_OC_SDIO_GetAllExtDevicePropInfo);
    if (self->protocolVersion >= SDI_EXTENSION_VERSION_300) {
        req.Params[0] = incremental ? 0x1 : 0x0;
        req.Params[1] = addExtended ? 0x1 : 0x0;
        req.NumParams = 2;
    } else {
        req.NumParams = 0;
    }

    PTPResponse r = SendReq(self, &req);
    RETURN_IF_FAIL(r);

    u64 numProperties = 0;
    MMemReadU64LE(&r.memIo, &numProperties);

    if (self->protocolVersion == SDI_EXTENSION_VERSION_200) {
        SDIO_ProcessDeviceProperties200(self, incremental, r, numProperties);
    } else {
        SDIO_ProcessDeviceProperties300(self, incremental, r, numProperties);
    }

    return r.result;
}

static PTPResult SDIO_SetExtDevicePropValue(PTPControl* self, u16 propCode, u16 dataType, PTPPropValue value) {
    size_t size = Ptp_PropValueSize(dataType, value);

    PTPRequestHeader req = BuildReq(self, size, 0x1000, PTP_OC_SDIO_SetExtDevicePropValue);
    req.NextPhase = PTP_NEXT_PHASE_WRITE_DATA;
    req.Params[0] = propCode;
    req.NumParams = 1;

    MMemIO memIo;
    MMemInit(&memIo, self->dataInMem, size);
    switch (dataType) {
        case PTP_DT_UINT8:
            MMemWriteU8(&memIo, value.u8);
            break;
        case PTP_DT_INT8:
            MMemWriteI8(&memIo, value.i8);
            break;
        case PTP_DT_UINT16:
            MMemWriteU16LE(&memIo, value.u16);
            break;
        case PTP_DT_INT16:
            MMemWriteI16LE(&memIo, value.i16);
            break;
        case PTP_DT_UINT32:
            MMemWriteU32LE(&memIo, value.u32);
            break;
        case PTP_DT_INT32:
            MMemWriteI32LE(&memIo, value.i32);
            break;
        case PTP_DT_STR: {
            u8 strSize = value.str.size;
            MMemWriteU8(&memIo, strSize);
            MMemWriteI8CopyN(&memIo, value.str.str, strSize);
            break;
        }
    }

    PTPResponse r = SendReq(self, &req);
    return r.result;
}

static PTPResult SDIO_ControlDevice(PTPControl* self, u16 propCode, u16 dataType, PTPPropValue value) {
    size_t size = Ptp_PropValueSize(dataType, value);

    PTPRequestHeader req = BuildReq(self, size, 0x1000, PTP_OC_SDIO_ControlDevice);
    req.NextPhase = PTP_NEXT_PHASE_WRITE_DATA;
    req.Params[0] = propCode;
    req.NumParams = 1;

    MMemIO memIo;
    MMemInit(&memIo, self->dataInMem, size);
    switch (dataType) {
        case PTP_DT_UINT8:
            MMemWriteU8(&memIo, value.u8);
            break;
        case PTP_DT_INT8:
            MMemWriteI8(&memIo, value.i8);
            break;
        case PTP_DT_UINT16:
            MMemWriteU16LE(&memIo, value.u16);
            break;
        case PTP_DT_INT16:
            MMemWriteI16LE(&memIo, value.i16);
            break;
        case PTP_DT_UINT32:
            MMemWriteU32LE(&memIo, value.u32);
            break;
        case PTP_DT_INT32:
            MMemWriteI32LE(&memIo, value.i32);
            break;
        case PTP_DT_STR: {
            u8 strSize = value.str.size;
            MMemWriteU8(&memIo, strSize);
            MMemWriteI8CopyN(&memIo, value.str.str, strSize);
            break;
        }
    }

    PTPResponse r = SendReq(self, &req);
    return r.result;
}

static PTPResult SDIO_GetDisplayStringList(PTPControl* self, PtpStringDisplayList displayList) {
    PTPResponse r = DoRequest(self,
                              PTP_OC_SDIO_GetDisplayStringList,
                              0,
                              0x1000,
                              1,
                              displayList);

    RETURN_IF_FAIL(r);

    u32 offset = 0;
    MMemReadU32(&r.memIo, &offset);

    u32 size = 0;
    MMemReadU32(&r.memIo, &size);

    // Skip to display lists
    r.memIo.pos = r.memIo.mem + offset;

    for (int i = 0; i < size; i++) {
        u16 listTypeNum = 0;
        MMemReadU16(&r.memIo, &listTypeNum);
        r.memIo.pos += 2;
        for (int j = 0; j < listTypeNum; j++) {
            u32 listType = 0;
            MMemReadU32(&r.memIo, &listType);
            u16 dataType = 0;
            MMemReadU16(&r.memIo, &dataType);
            u16 displayStringNum = 0;
            MMemReadU16(&r.memIo, &displayStringNum);
            MLogf("Display String List: %04x Data: %04x Num: %d", listType, dataType, displayStringNum);
            for (int k = 0; k < displayStringNum; k++) {
                PTPPropValue value;
                ReadPropertyValue(&r.memIo, dataType, &value);
                char* str = ReadPtpString16(&r.memIo);
                MLogf(" -- %s", str);
            }
        }
    }

    return r.result;
}

static PTPResult SDIO_GetLensInformation(PTPControl* self, PtpFocusUnits focusUnits) {
    PTPResponse r = DoRequest(self,
                              PTP_OC_SDIO_GetLensInformation,
                              0,
                              0x1000,
                              1,
                              focusUnits);

    RETURN_IF_FAIL(r);

    u32 offset = 0;
    MMemReadU32(&r.memIo, &offset);

    u32 size = 0;
    MMemReadU32(&r.memIo, &size);

    // Skip to display lists
    r.memIo.pos = r.memIo.mem + offset;

    u16 dataVersion = 0;
    MMemReadU16(&r.memIo, &dataVersion);

    u16 numTables = 0;
    MMemReadU16(&r.memIo, &numTables);

    for (int i = 0; i < numTables; i++) {
        u16 listNums = 0;
        MMemReadU16(&r.memIo, &listNums);
        r.memIo.pos += 2;
        for (int j = 0; j < listNums; j++) {
            u32 normalizedValue = 0;
            MMemReadU32(&r.memIo, &normalizedValue);

            u32 focusPosition = 0;
            MMemReadU32(&r.memIo, &focusPosition);
        }
    }

    return r.result;
}

static PTPResult SDIO_GetExtDeviceInfo(PTPControl* self, u32 protocolVersion, b32 extended) {
    PTPResponse r = DoRequest(self,
                              PTP_OC_SDIO_GetExtDeviceInfo,
                              0,
                              0x1000,
                              2,
                              protocolVersion,
                              extended ? 1: 0);

    RETURN_IF_FAIL(r);

    u16 version = 0;
    MMemReadU16LE(&r.memIo, &version);
    self->protocolVersion = version;

    u32 numProperties = 0;
    MMemReadU32LE(&r.memIo, &numProperties);

    for (int i = 0; i < numProperties; i++) {
        u16 propCode = 0;
        MMemReadU16LE(&r.memIo, &propCode);
        MArrayAdd(self->supportedProperties, propCode);
    }

    u32 numControls = 0;
    MMemReadU32LE(&r.memIo, &numControls);
    for (int i = 0; i < numControls; i++) {
        u16 controlCode = 0;
        MMemReadU16LE(&r.memIo, &controlCode);
        MArrayAdd(self->supportedControls, controlCode);
    }

    return r.result;
}

static PTPResult PTP_GetObjectInfo(PTPControl* self, u32 objectHandle, PTPObjectInfo* objectInfo) {
    PTPResponse r = DoRequest(self,
                              PTP_OC_GetObjectInfo,
                              0,
                              0x1000,
                              1,
                              objectHandle);

    RETURN_IF_FAIL(r);

    MMemReadU32LE(&r.memIo, &objectInfo->storageID);
    MMemReadU16LE(&r.memIo, &objectInfo->objectFormat);
    MMemReadU16LE(&r.memIo, &objectInfo->protectionStatus);
    MMemReadU32LE(&r.memIo, &objectInfo->objectCompressedSize);
    MMemReadU16LE(&r.memIo, &objectInfo->thumbFormat);
    MMemReadU32LE(&r.memIo, &objectInfo->thumbCompressedSize);
    MMemReadU32LE(&r.memIo, &objectInfo->thumbPixWidth);
    MMemReadU32LE(&r.memIo, &objectInfo->thumbPixHeight);
    MMemReadU32LE(&r.memIo, &objectInfo->imagePixWidth);
    MMemReadU32LE(&r.memIo, &objectInfo->imagePixHeight);
    MMemReadU32LE(&r.memIo, &objectInfo->imagePixDepth);
    MMemReadU32LE(&r.memIo, &objectInfo->parentObject);
    MMemReadU16LE(&r.memIo, &objectInfo->associationType);
    MMemReadU32LE(&r.memIo, &objectInfo->associationDesc);
    MMemReadU32LE(&r.memIo, &objectInfo->sequenceNumber);

    objectInfo->filename = ReadPtpString8(&r.memIo);
    objectInfo->captureDateTime = ReadPtpString8(&r.memIo);
    objectInfo->modDateTime = ReadPtpString8(&r.memIo);
    objectInfo->keywords = ReadPtpString8(&r.memIo);

    return r.result;
}

static PTPResult PTP_GetLiveViewImage(PTPControl* self, size_t objectSize, MMemIO* fileOut, LiveViewFrames* liveViewFrames) {
    fileOut->pos = fileOut->mem;
    fileOut->size = 0;

    PTPResponse r = DoRequest(self,
                              PTP_OC_GetObject,
                              0,
                              objectSize + 0x100,
                              1,
                              SD_OH_LIVE_VIEW_IMAGE);

    RETURN_IF_FAIL(r);

    b32 readFocalFrame = FALSE;
    if (self->protocolVersion >= SDI_EXTENSION_VERSION_300) {
        readFocalFrame = TRUE;
    }

    u32 offsetImage = 0;
    MMemReadU32LE(&r.memIo, &offsetImage);

    u32 imageSize = 0;
    MMemReadU32LE(&r.memIo, &imageSize);

    if (readFocalFrame) {
        u32 focalFrameOffset = 0;
        MMemReadU32LE(&r.memIo, &focalFrameOffset);

        u32 focalFrameSize = 0;
        MMemReadU32LE(&r.memIo, &focalFrameSize);

        if (focalFrameSize) {
            r.memIo.pos = r.memIo.mem + focalFrameOffset;

            MMemReadU16LE(&r.memIo, &liveViewFrames->version);
            MMemReadSkipBytes(&r.memIo, 6 + 40);

            u16 reservedArrayNum = 0;
            MMemReadU16LE(&r.memIo, &reservedArrayNum);
            MMemReadSkipBytes(&r.memIo, 6);
            if (reservedArrayNum) {
                MMemReadSkipBytes(&r.memIo, reservedArrayNum * 24);
            }

            FocusFrames* focusFrames = &liveViewFrames->focus;
            MMemReadU32LE(&r.memIo, &focusFrames->xDenominator);
            MMemReadU32LE(&r.memIo, &focusFrames->yDenominator);

            u16 frameNum = 0;
            MMemReadU16LE(&r.memIo, &frameNum);

            MMemReadSkipBytes(&r.memIo, 6);

            if (frameNum) {
                MArrayInit(focusFrames->frames, frameNum);

                for (int i = 0; i < frameNum; ++i) {
                    FocusFrame* focusFrame = focusFrames->frames + i;
                    MMemReadU16LE(&r.memIo, &focusFrame->frameType);
                    MMemReadU16LE(&r.memIo, &focusFrame->focusFrameState);
                    MMemReadU8(&r.memIo, &focusFrame->priority);
                    MMemReadSkipBytes(&r.memIo, 3);
                    MMemReadU32LE(&r.memIo, &focusFrame->width);
                    MMemReadU32LE(&r.memIo, &focusFrame->height);
                }
            }

            if (liveViewFrames->version > 101) {
            }
        }
    }

    fileOut->pos = fileOut->mem;
    fileOut->size = 0;
    MMemWriteU8CopyN(fileOut, r.memIo.mem + offsetImage, imageSize);

    return r.result;
}

PTPResult PTP_GetObject(PTPControl* self, u32 objectHandle, size_t objectSize, MMemIO* fileOut) {
    PTP_TRACE("PTP_GetObject");
    fileOut->pos = fileOut->mem;
    fileOut->size = 0;

    PTPResponse r = DoRequest(self,
                              PTP_OC_GetObject,
                              0,
                              objectSize,
                              1,
                              objectHandle);

    RETURN_IF_FAIL(r);

    MMemReadCopy(&r.memIo, fileOut, r.memIo.size);

    return r.result;
}

int PTPControl_GetPendingFiles(PTPControl* self) {
    PTPProperty* property = PTPControl_GetProperty(self, DPC_PENDING_FILES);
    if (property != NULL && property->dataType == PTP_DT_UINT16) {
        u16 value = property->value.u16;
        if (value & 0x8000) {
            value = value & 0x7fff;
        }
        PTP_TRACE_F("PTPControl_GetPendingFiles -> %d", value);
        return value;
    }
    return 0;
}

PTPResult PTPControl_GetLiveViewImage(PTPControl* self, MMemIO* fileOut, LiveViewFrames* liveViewFramesOut) {
    PTP_TRACE("PTPControl_GetLiveViewImage");
    PTPObjectInfo objectInfo = {};
    PTPResult r = PTP_GetObjectInfo(self, SD_OH_LIVE_VIEW_IMAGE, &objectInfo);
    if (r != PTP_OK) {
        return r;
    }
    Ptp_FreeObjectInfo(&objectInfo);
    return PTP_GetLiveViewImage(self, objectInfo.objectCompressedSize, fileOut, liveViewFramesOut);
}

PTPResult PTPControl_GetCapturedImage(PTPControl* self, MMemIO* fileOut, PTPCapturedImageInfo* ciiOut) {
    PTP_TRACE("PTPControl_GetCapturedImage");
    PTPObjectInfo objectInfo = {};
    PTPResult r = PTP_GetObjectInfo(self, SD_OH_CAPTURED_IMAGE, &objectInfo);
    if (r != PTP_OK) {
        return r;
    }
    PTP_DEBUG_F("Downloading image... (%s format: %s size: %d)", objectInfo.filename.str,
        PTP_GetObjectFormatStr(objectInfo.objectFormat), objectInfo.objectCompressedSize);
    ciiOut->filename = objectInfo.filename;
    ciiOut->objectFormat = objectInfo.objectFormat;
    ciiOut->size = objectInfo.objectCompressedSize;
    objectInfo.filename.str = NULL; // Pass filename ownership to ciiOut
    objectInfo.filename.size = 0;
    Ptp_FreeObjectInfo(&objectInfo); // Free any other strings
    r = PTP_GetObject(self, SD_OH_CAPTURED_IMAGE, objectInfo.objectCompressedSize, fileOut);
    PTP_DEBUG_F("Downloaded image size: %d", fileOut->size);
    return r;
}

PTPResult PTPControl_GetCameraSettingsFile(PTPControl* self, MMemIO* fileOut) {
    PTP_TRACE("PTPControl_GetCameraSettingsFile");
    PTPObjectInfo objectInfo = {};
    PTPResult r = PTP_GetObjectInfo(self, SD_OH_CAMERA_SETTINGS, &objectInfo);
    if (r != PTP_OK) {
        return r;
    }
    Ptp_FreeObjectInfo(&objectInfo);
    return PTP_GetObject(self, SD_OH_CAMERA_SETTINGS, objectInfo.objectCompressedSize, fileOut);
}

PTPResult PTP_SendObject(PTPControl* self, u32 objectHandle, MMemIO* fileIn) {
    PTPRequestHeader req = BuildReq(self, fileIn->size, 0x1000, PTP_OC_SendObject);
    req.Params[0] = objectHandle;
    req.NumParams = 1;
    req.NextPhase = PTP_NEXT_PHASE_WRITE_DATA;

    MMemIO memIo;
    MMemInit(&memIo, self->dataInMem, fileIn->size);
    MMemReadCopy(fileIn, &memIo, fileIn->size);

    PTPResponse r = SendReq(self, &req);
    return r.result;
}

PTPResult PTPControl_PutCameraSettingsFile(PTPControl* self, MMemIO* fileIn) {
    PTP_TRACE("PTPControl_PutCameraSettingsFile");
    return PTP_SendObject(self, SD_OH_CAMERA_SETTINGS, fileIn);
}

PTPResult PTPControl_Init(PTPControl* self, PTPDevice* device) {
    if (!self || !device) {
        return PTP_GENERAL_ERROR;
    }

    self->deviceTransport = &device->transport;
    self->logger = device->logger;
    return PTP_OK;
}

static void SDIO_InitControlsMetadata200(PTPControl *self, size_t numControls) {
    for (int i = 0; i < numControls; i++) {
        u16 controlCode = self->supportedControls[i];
        PtpControl* control = PTPControl_GetControl(self, controlCode);
        if (!control) {
            control = MArrayAddPtr(self->controls);

            b32 found = FALSE;
            size_t numControlsMeta = MStaticArraySize(sPtpControlsMetadata);
            for (int j = 0; j < numControlsMeta; j++) {
                PtpControl *meta = sPtpControlsMetadata + j;
                if (meta->controlCode == controlCode) {
                    memcpy(control, meta, sizeof(PtpControl));
                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                memset(control, 0, sizeof(PtpControl));
                control->controlCode = controlCode;
            }
        }
    }
}

static void SDIO_InitControlsMetadata300(PTPControl *self, size_t numControls) {
    MArrayInit(self->controls, numControls);
    for (int i = 0; i < numControls; i++) {
        u16 controlCode = self->supportedControls[i];

        PtpControl* control = MArrayAddPtr(self->controls);
        b32 found = FALSE;
        size_t numControlsMeta = MStaticArraySize(sPtpControlsMetadata);
        for (int j = 0; j < numControlsMeta; j++) {
            PtpControl* meta = sPtpControlsMetadata + j;
            if (meta->controlCode == controlCode) {
                memcpy(control, meta, sizeof(PtpControl));
                found = TRUE;
                break;
            }
        }
        if (!found) {
            memset(control, 0, sizeof(PtpControl));
            control->controlCode = controlCode;
        }
    }
}

PTPResult PTPControl_Connect(PTPControl* self, SonyProtocolVersion version) {
    PTP_TRACE_F("PTPControl_Connect 0x04%x", version);
    ////////////////////////////////////////////
    // Open Session (if not done by transport layer implicitly)
    ////////////////////////////////////////////
    if (self->deviceTransport->requiresSessionOpenClose) {
        self->sessionId = 0;
        self->transactionId = 0;
        u32 sessionId = 0x1;
        OpenSession(self, sessionId);
        self->sessionId = sessionId;
    }

    ////////////////////////////////////////////
    // Authentication begin
    ////////////////////////////////////////////
    u32 connectionId = 0;

    // 1. Authentication Packet 1
    SDIO_Connect(self, 1, connectionId);

    // 2. Authentication Packet 2
    SDIO_Connect(self, 2, connectionId);

    // 3. Authentication - Request available properties and controls
    int retries = 10;
    b32 gotExtDeviceInfo = FALSE;
    for (int i = 0; i < retries; ++i) {
        if (OK(SDIO_GetExtDeviceInfo(self, version, TRUE))) {
            gotExtDeviceInfo = TRUE;
            break;
        }
    }
    if (!gotExtDeviceInfo) {
        return PTP_GENERAL_ERROR;
    }

    // 4. Authentication Phase 3
    SDIO_Connect(self, 3, connectionId);

    ////////////////////////////////////////////
    // Authentication done
    ////////////////////////////////////////////

    // Get general device info
    PTP_GetDeviceInfo(self);

    // Get property metadata & values
    SDIO_GetAllExtDevicePropInfo(self, FALSE, TRUE);

    // SDIO_GetDisplayStringList(self, PTP_DL_ALL);

    // SDIO_GetLensInformation(self, PTP_FOCUS_UNIT_FEET);

    // Build controls list
    size_t numControls = MArraySize(self->supportedControls);
    PTP_INFO_F("Connected to device (protocol: %d).", self->protocolVersion);
    if (self->protocolVersion == SDI_EXTENSION_VERSION_200) {
        SDIO_InitControlsMetadata200(self, numControls);
    } else {
        SDIO_InitControlsMetadata300(self, numControls);
    }

    return PTP_OK;
}

PTPResult PTPControl_Cleanup(PTPControl* self) {
    PTP_TRACE("PTPControl_Cleanup");

    ////////////////////////////////////////////
    // Close session (if not done by transport layer implicitly)
    ////////////////////////////////////////////
    if (self->deviceTransport->requiresSessionOpenClose) {
        CloseSession(self);
        self->sessionId = 0;
        self->transactionId = 0;
    }

    PTPControl_FreeDataBuffers(self);

    self->protocolVersion = 0;
    self->standardVersion = 0;

    MArrayFree(self->supportedProperties);
    MArrayFree(self->supportedControls);
    MArrayFree(self->supportedEvents);
    MArrayFree(self->supportedOperations);
    MArrayFree(self->captureFormats);
    MArrayFree(self->imageFormats);
    for (int i = 0; i < MArraySize(self->properties); ++i) {
        PTPProperty* property = self->properties + i;
        PropValueFree(property->dataType, &property->value);
        PropValueFree(property->dataType, &property->defaultValue);
        if (property->formFlag == PTP_FORM_FLAG_ENUM) {
            MArrayFree(property->form.enums.set);
            MArrayFree(property->form.enums.getSet);
        } else if (property->formFlag == PTP_FORM_FLAG_RANGE) {
            PropValueFree(property->dataType, &property->form.range.min);
            PropValueFree(property->dataType, &property->form.range.max);
            PropValueFree(property->dataType, &property->form.range.step);
        }
    }
    MArrayFree(self->properties);

    for (int i = 0; i < MArraySize(self->controls); ++i) {
        PtpControl* control = self->controls + i;
        if (control->formFlag == PTP_FORM_FLAG_ENUM) {
            if (control->form.enums.owned) {
                MArrayFree(control->form.enums.values);
            }
        }
    }
    MArrayFree(self->controls);

    MStrFree(self->manufacturer);
    MStrFree(self->model);
    MStrFree(self->deviceVersion);
    MStrFree(self->serialNumber);
    MStrFree(self->vendorExtension);

    return PTP_OK;
}

b32 PTPControl_SupportsEvent(PTPControl* self, u16 eventCode) {
    for (int i = 0; i < MArraySize(self->supportedEvents); ++i) {
        if (self->supportedEvents[i] == eventCode) {
            return TRUE;
        }
    }
    return FALSE;
}

b32 PTPControl_SupportsControl(PTPControl* self, u16 controlCode) {
    for (int i = 0; i < MArraySize(self->supportedControls); ++i) {
        if (self->supportedControls[i] == controlCode) {
            return TRUE;
        }
    }
    return FALSE;
}

b32 PTPControl_SupportsProperty(PTPControl* self, u16 propertyCode) {
    for (int i = 0; i < MArraySize(self->supportedProperties); ++i) {
        if (self->supportedProperties[i] == propertyCode) {
            return TRUE;
        }
    }
    return FALSE;
}

b32 PTPControl_PropertyEnabled(PTPControl* self, u16 propertyCode) {
    PTPProperty* prop = PTPControl_GetProperty(self, propertyCode);
    if (!prop) {
        return FALSE;
    }
    if (prop->dataType == PTP_DT_UINT8) {
        return prop->value.u8 == SD_Enabled;
    }
    return FALSE;
}

PTPProperty* PTPControl_GetProperty(PTPControl* self, u16 propertyCode) {
    for (int i = 0; i < MArraySize(self->properties); ++i) {
        if (self->properties[i].propCode == propertyCode) {
            return self->properties + i;
        }
    }
    return NULL;
}

PTPResult PTPControl_UpdateProperties(PTPControl* self) {
    PTP_TRACE("PTPControl_UpdateProperties");
    return SDIO_GetAllExtDevicePropInfo(self, TRUE, TRUE);
}

typedef struct {
    u8 value;
    char* str;
} EnumValueU8;

typedef struct {
    u16 value;
    char* str;
} EnumValueU16;

typedef struct {
    u32 value;
    char* str;
} EnumValueU32;

static char* EnumValue8_Lookup(EnumValueU8* enumValues, size_t numEnumValues, u8 lookupValue) {
    for (int j = 0; j < numEnumValues; j++) {
        u8 enumValue = enumValues[j].value;
        if (lookupValue == enumValue) {
            return enumValues[j].str;
        }
    }
    return NULL;
}

static b32 BuildEnumsFromListU8(PTPControl* self, PTPProperty* property, EnumValueU8* enumValues, size_t numEnumValues, PTPPropValueEnums* outEnums) {
    for (int i = 0; i < MArraySize(property->form.enums.getSet); i++) {
        u8 lookupValue = property->form.enums.getSet[i].u8;
        char *str = EnumValue8_Lookup(enumValues, numEnumValues, lookupValue);
        PTPPropValueEnum *propEnum = MArrayAddPtr(outEnums->values);
        propEnum->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
        propEnum->propValue.u8 = lookupValue;
        propEnum->str.str = str;
        propEnum->str.size = 0;
    }

    if (MArraySize(property->form.enums.set)) {
        size_t getSetItems = MArraySize(outEnums->values);
        for (int j = 0; j < getSetItems; j++) {
            PTPPropValueEnum* prop = outEnums->values + j;
            prop->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ;
        }
        for (int i = 0; i < MArraySize(property->form.enums.set); i++) {
            u8 lookupValue = property->form.enums.set[i].u8;
            PTPPropValueEnum* prop = NULL;
            for (int j = 0; j < getSetItems; j++) {
                u8 enumValue = outEnums->values[j].propValue.u8;
                if (lookupValue == enumValue) {
                    prop = outEnums->values + j;
                    break;
                }
            }
            if (prop) {
                prop->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
            } else {
                char *str = EnumValue8_Lookup(enumValues, numEnumValues, lookupValue);
                PTPPropValueEnum *propEnum = MArrayAddPtr(outEnums->values);
                propEnum->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
                propEnum->propValue.u8 = lookupValue;
                propEnum->str.str = str;
                propEnum->str.size = 0;
            }
        }
    }

    return TRUE;
}

static char* EnumValue16_Lookup(EnumValueU16* enumValues, size_t numEnumValues, u16 lookupValue) {
    for (int j = 0; j < numEnumValues; j++) {
        u16 enumValue = enumValues[j].value;
        if (lookupValue == enumValue) {
            return enumValues[j].str;
        }
    }
    return NULL;
}

static b32 BuildEnumsFromListU16(PTPControl* self, PTPProperty* property, EnumValueU16* enumValues, size_t numEnumValues, PTPPropValueEnums* outEnums) {
    for (int i = 0; i < MArraySize(property->form.enums.getSet); i++) {
        u16 lookupValue = property->form.enums.getSet[i].u16;
        char *str = EnumValue16_Lookup(enumValues, numEnumValues, lookupValue);
        PTPPropValueEnum *propEnum = MArrayAddPtr(outEnums->values);
        propEnum->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
        propEnum->propValue.u16 = lookupValue;
        propEnum->str.str = str;
        propEnum->str.size = 0;
    }

    if (MArraySize(property->form.enums.set)) {
        size_t getSetItems = MArraySize(outEnums->values);
        for (int j = 0; j < getSetItems; j++) {
            PTPPropValueEnum* prop = outEnums->values + j;
            prop->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ;
        }
        for (int i = 0; i < MArraySize(property->form.enums.set); i++) {
            u16 lookupValue = property->form.enums.set[i].u16;
            PTPPropValueEnum* prop = NULL;
            for (int j = 0; j < getSetItems; j++) {
                u16 enumValue = outEnums->values[j].propValue.u16;
                if (lookupValue == enumValue) {
                    prop = outEnums->values + j;
                    break;
                }
            }
            if (prop) {
                prop->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
            } else {
                char *str = EnumValue16_Lookup(enumValues, numEnumValues, lookupValue);
                PTPPropValueEnum *propEnum = MArrayAddPtr(outEnums->values);
                propEnum->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
                propEnum->propValue.u16 = lookupValue;
                propEnum->str.str = str;
                propEnum->str.size = 0;
            }
        }
    }

    return TRUE;
}

static char* EnumValue32_Lookup(EnumValueU32* enumValues, size_t numEnumValues, u32 lookupValue) {
    for (int j = 0; j < numEnumValues; j++) {
        u32 enumValue = enumValues[j].value;
        if (lookupValue == enumValue) {
            return enumValues[j].str;
        }
    }
    return NULL;
}

static b32 BuildEnumsFromListU32(PTPControl* self, PTPProperty* property, EnumValueU32* enumValues, size_t numEnumValues, PTPPropValueEnums* outEnums) {
    for (int i = 0; i < MArraySize(property->form.enums.getSet); i++) {
        u32 lookupValue = property->form.enums.getSet[i].u32;
        char *str = EnumValue32_Lookup(enumValues, numEnumValues, lookupValue);
        PTPPropValueEnum *propEnum = MArrayAddPtr(outEnums->values);
        propEnum->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
        propEnum->propValue.u32 = lookupValue;
        propEnum->str.str = str;
        propEnum->str.size = 0;
    }

    if (MArraySize(property->form.enums.set)) {
        size_t getSetItems = MArraySize(outEnums->values);
        for (int j = 0; j < getSetItems; j++) {
            PTPPropValueEnum* prop = outEnums->values + j;
            prop->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ;
        }
        for (int i = 0; i < MArraySize(property->form.enums.set); i++) {
            u32 lookupValue = property->form.enums.set[i].u32;
            PTPPropValueEnum* prop = NULL;
            for (int j = 0; j < getSetItems; j++) {
                u32 enumValue = outEnums->values[j].propValue.u32;
                if (lookupValue == enumValue) {
                    prop = outEnums->values + j;
                    break;
                }
            }
            if (prop) {
                prop->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
            } else {
                char *str = EnumValue32_Lookup(enumValues, numEnumValues, lookupValue);
                PTPPropValueEnum *propEnum = MArrayAddPtr(outEnums->values);
                propEnum->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
                propEnum->propValue.u32 = lookupValue;
                propEnum->str.str = str;
                propEnum->str.size = 0;
            }
        }
    }

    return TRUE;
}

static MStr GetFNumberAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u16 value = propValue.u16;
    int whole = value / 100;
    int decimals = (value % 100) / 10;
    char text[32];
    int len = 0;
    if (value == 0xfffd) {
        r.str = "Iris Close";
        return r;
    } else if (value == 0xfffe) {
        r.str = "--"; // No lens or no lens info
        return r;
    } else if (value == 0xffff) {
        r.str = "";
        return r;
    }
    if (whole >= 10 && decimals == 0) {
        len = snprintf(text, sizeof(text), "%d", whole);
    } else {
        len = snprintf(text, sizeof(text), "%d.%d", whole, decimals);
    }
    if (len > 0) {
        len++;
        r.str = MMalloc(len);
        memcpy(r.str, text, len);
        r.size = len;
    }
    return r;
}

static MStr GetShutterSpeedAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    u32 value = propValue.u32;
    MStr r = {};
    if (value == 0xffffffff) {
        r.str = "n/a";
    } else if (value == 0x0) {
        r.str = "Bulb";
    } else {
        char text[32];
        u16 top = (value >> 16);
        u16 bottom = (value & 0xffff);
        int len;
        if (bottom == 10 && top != 1) {
            int w = top / 10;
            int d = top % 10;
            if (d) {
                len = snprintf(text, sizeof(text), "%d.%d", w, d);
            } else {
                len = snprintf(text, sizeof(text), "%d", w);
            }
        } else {
            len = snprintf(text, sizeof(text), "%d/%d", top, bottom);
        }
        if (len > 0) {
            r.str = MMalloc(len+1);
            memcpy(r.str, text, len+1);
            r.size = len + 1;
        }
    }
    return r;
}

static MStr GetWhiteBalanceGMAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    int value = propValue.u8;
    if (value > 0xE4 || value < 0x9C) {
        return r;
    }
    value = value - (0x9C + 0x24);
    if (value == 0) {
        r.str = "0.0";
        return r;
    }

    int len = 6;
    r.str = MMalloc(len);
    r.size = len;

    if (value > 0) {
        r.str[0] = 'G';
    } else {
        value = -value;
        r.str[0] = 'M';
    }

    int v = (value & 0xfc) >> 2;
    int f = value & 0x03;
    int intSize = snprintf(r.str + 1, len, "%d", v);
    if (intSize) {
        int pos = intSize + 1;
        r.str[pos++] = '.';
        if (f == 0) {
            r.str[pos++] = '0';
        } else if (f == 1) {
            r.str[pos++] = '2'; r.str[pos++] = '5';
        } else if (f == 2) {
            r.str[pos++] = '5';
        } else if (f == 3) {
            r.str[pos++] = '7'; r.str[pos++] = '5';
        }
        r.str[pos] = 0;
    } else {
        r.str[1] = 0;
    }

    return r;
}

static MStr GetWhiteBalanceABAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    int value = propValue.u8;
    if (value > 0xE4 || value < 0x9C) {
        return r;
    }
    value = value - (0x9C + 0x24);
    if (value == 0) {
        r.str = "0.0";
        return r;
    }

    int len = 6;
    r.str = MMalloc(len);
    r.size = len;

    if (value > 0) {
        r.str[0] = 'A';
    } else {
        value = -value;
        r.str[0] = 'B';
    }

    int v = (value & 0xfc) >> 2;
    int f = value & 0x03;
    int intSize = snprintf(r.str + 1, len, "%d", v);
    if (intSize) {
        int pos = intSize + 1;
        r.str[pos++] = '.';
        if (f == 0) {
            r.str[pos++] = '0';
        } else if (f == 1) {
            r.str[pos++] = '2'; r.str[pos++] = '5';
        } else if (f == 2) {
            r.str[pos++] = '5';
        } else if (f == 3) {
            r.str[pos++] = '7'; r.str[pos++] = '5';
        }
        r.str[pos] = 0;
    } else {
        r.str[1] = 0;
    }

    return r;
}

static MStr GetPendingFileInfoAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u16 value = propValue.u16;
    if (value == 0x0000) {
        r.str = "None";
    } else if (value < 0x8000) {
        char text[32];
        int len = snprintf(text, sizeof(text), "Files (%d)", value);
        if (len > 0) {
            r.str = MMalloc(len+1);
            memcpy(r.str, text, len+1);
            r.size = len + 1;
        }
    } else {
        char text[32];
        value &= 0x7fff;
        int len = snprintf(text, sizeof(text), "Files (%d)", value);
        if (len > 0) {
            r.str = MMalloc(len+1);
            memcpy(r.str, text, len+1);
            r.size = len + 1;
        }
    }
    return r;
}

static MStr GetPixelShootingNumberAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u16 value = propValue.u16;
    if (value == 0) {
        r.str = "None";
    } else if (value == 1) {
        r.str = "1 Sheet";
    } else {
        char text[32];
        int len = snprintf(text, sizeof(text), "%d Sheets", value);
        if (len > 0) {
            r.str = MMalloc(len+1);
            memcpy(r.str, text, len+1);
            r.size = len + 1;
        }
    }
    return r;
}

static MStr GetPixelShootingIntervalAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u16 value = propValue.u16;
    if (value == 0xFFFF) {
        r.str = "Shortest Interval";
    } else {
        char text[32];
        value &= 0x7fff;
        int len = snprintf(text, sizeof(text), "%d sec", value);
        if (len > 0) {
            r.str = MMalloc(len+1);
            memcpy(r.str, text, len+1);
            r.size = len + 1;
        }
    }
    return r;
}

static MStr GetPixelShootingProgressAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u16 value = propValue.u16;
    char text[32];
    value &= 0x7fff;
    int len = snprintf(text, sizeof(text), "Shot %d", value);
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

static MStr GetBatteryRemainingAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    i8 value = propValue.i8;
    if (value == -1) {
        r.str = "n/a";
    } else {
        char text[32];
        int len = snprintf(text, sizeof(text), "%d%%", value);
        if (len > 0) {
            r.str = MMalloc(len+1);
            memcpy(r.str, text, len+1);
            r.size = len + 1;
        }
    }
    return r;
}

static MStr GetPredictedMaxFileSizeAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u32 value = propValue.u32;
    char text[32];
    int len = snprintf(text, sizeof(text), "%d bytes", value);
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

static MStr GetTemperatureAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u16 value = propValue.u16;
    if (value == 0x0000) {
        r.str = "N/A";
    } else if (value == 0xFFFF) {
        r.str = ">9900K";
    } else {
        char text[32];
        int len = snprintf(text, sizeof(text), "%dK", value);
        if (len > 0) {
            r.str = MMalloc(len+1);
            memcpy(r.str, text, len+1);
            r.size = len + 1;
        }
    }
    return r;
}

static MStr GetIsoAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u32 value = propValue.u32;
    u32 mode = value >> 24;
    u32 iso = value & 0xffffff;
    if (mode == 0 || mode == 0x10) {
        if (iso == 0x00ffffff) {
            r.str = "Auto";
        } else {
            char text[32];
            int len = snprintf(text, sizeof(text), "%d", iso);
            if (len > 0) {
                r.str = MMalloc(len+1);
                memcpy(r.str, text, len+1);
                r.size = len + 1;
            }
        }
    } else if (mode == 0x1) {
        if (iso == 0x00ffffff) {
            r.str = "Multi-Frame NR Auto";
        } else {
            char text[32];
            int len = snprintf(text, sizeof(text), "Multi-Frame NR %d", iso);
            if (len > 0) {
                r.str = MMalloc(len+1);
                memcpy(r.str, text, len+1);
                r.size = len + 1;
            }
        }
    } else if (mode == 0x2) {
        if (iso == 0x00ffffff) {
            r.str = "Multi-Frame NR High Auto";
        } else {
            char text[32];
            int len = snprintf(text, sizeof(text), "Multi-Frame NR High %d", iso);
            if (len > 0) {
                r.str = MMalloc(len+1);
                memcpy(r.str, text, len+1);
                r.size = len + 1;
            }
        }
    }
    return r;
}

static MStr GetExposureBiasAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    i16 value = propValue.i16;
    MStr r = {};
    int whole = value / 1000;
    int decimals = (value % 1000) / 100;
    if (decimals < 0) {
        decimals = -decimals;
    }
    char text[32];
    int len = snprintf(text, sizeof(text), "%d.%dEV", whole, decimals);
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

static MStr GetFlashCompAsString(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    i16 value = propValue.i16;
    MStr r = {};
    int whole = value / 1000;
    int decimal = (value % 1000) / 100;
    if (decimal < 0) {
        decimal = -decimal;
    }
    char text[32];
    int len = snprintf(text, sizeof(text), "%d.%dEV", whole, decimal);
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

static MStr GetZoomScale(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    int whole = propValue.u32 / 1000;
    int decimals = (propValue.u32 % 1000) / 100;
    char text[32];
    int len = 0;
    if (decimals) {
        len = snprintf(text, sizeof(text), "%d.%d", whole, decimals);
    } else {
        len = snprintf(text, sizeof(text), "%d", whole);
    }
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

static MStr GetZoomBarInfo(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u32 value = propValue.u32;
    u32 zoomPosition = value & 0xffff;
    u32 currentBox = (value >> 16) & 0xff;
    u32 totalBox = (value >> 24) & 0xff;
    char text[32];
    int len = snprintf(text, sizeof(text), "%d, %d, %d", totalBox, currentBox, zoomPosition);
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

static EnumValueU8 sProp_OnOff0[] = {
    {0x00, "Off"},
    {0x01, "On"}
};

static EnumValueU8 sProp_OnOff1[] = {
    {0x01, "Off"},
    {0x02, "On"}
};

static EnumValueU8 sProp_EnabledDisabled[] = {
    {0x00, "Disabled"},
    {0x01, "Enabled"}
};

static EnumValueU8 sProp_PixelShiftShootingMode[] = {
    {0x00, "Off"},
    {0x01, "Burst Shooting"}
};

static EnumValueU8 sProp_PixelShiftShootingStatus[] = {
    {0x00, "Not Shooting"},
    {0x01, "Shooting"}
};

static EnumValueU8 sProp_CompressionSetting[] = {
    {0x01, "ECO/LIGHT"},
    {0x02, "STD"},
    {0x03, "FINE"},
    {0x04, "XFINE"},
    {0x10, "RAW"},
    {0x11, "RAW+JPG(LIGHT)"},
    {0x12, "RAW+JPG(STD)"},
    {0x13, "RAW+JPG(FINE)"},
    {0x14, "RAW+JPG(XFINE)"},
    {0x20, "RAW(compression)"},
    {0x23, "RAW(compression)+JPG(FINE)"},
    {0x31, "HEIF(ECO/LIGHT)"},
    {0x32, "HEIF(STD)"},
    {0x33, "HEIF(FINE)"},
    {0x34, "HEIF(XFINE)"},
    {0x41, "RAW+HEIF(LIGHT)"},
    {0x42, "RAW+HEIF(STD)"},
    {0x43, "RAW+HEIF(FINE)"},
    {0x44, "RAW+HEIF(XFINE)"},
};

static EnumValueU16 sProp_WhiteBalance[] = {
    {0x0001, "Manual"},
    {0x0002, "AWB"},
    {0x0003, "One-push Automatic"},
    {0x0004, "Daylight"},
    {0x0005, "Fluorescent"},
    {0x0006, "Tungsten (Incandescent)"},
    {0x0007, "Flash"},
    {0x8001, "Fluorescence - Warm White (-1)"},
    {0x8002, "Fluorescence - Cool White (0)"},
    {0x8003, "Fluorescence - Day White (+1)"},
    {0x8004, "Fluorescence - Daylight White (+2)"},
    {0x8010, "Cloudy"},
    {0x8011, "Shade"},
    {0x8012, "Custom Temperature"},
    {0x8020, "Custom 1"},
    {0x8021, "Custom 2"},
    {0x8022, "Custom 3"},
    {0x8023, "Custom"},
    {0x8030, "Underwater Auto"},
};

static EnumValueU16 sProp_FocusMode[] = {
    {0x0001, "Manual"},
    {0x0002, "AF-S"},
    {0x0003, "Auto-Macro"},
    {0x8004, "AF-C"},
    {0x8005, "AF-Auto"},
    {0x8006, "DMF"},
    {0x8007, "MF-R"},
    {0x8008, "AF-D"},
    {0x8009, "Preset Focus"},
};

static EnumValueU8 sProp_ImageSize[] = {
    {0x01, "L"},
    {0x02, "M"},
    {0x03, "S"},
    {0x04, "24M"},
    {0x05, "16M"},
    {0x06, "13M"},
    {0x07, "11M"},
    {0x08, "8.4M"},
    {0x09, "6.1M"},
    {0x0A, "6M"},
    {0x0B, "5.6M"},
    {0x0C, "4M"},
    {0x0D, "2.6M"},
    {0x0E, "21M"},
    {0x0F, "20M"},
    {0x10, "10M"},
    {0x11, "9.2M"},
    {0x12, "8.7M"},
    {0x13, "6.9M"},
    {0x14, "4.6M"},
    {0x15, "4.1M"},
    {0x16, "4M"},
    {0x17, "3.9M"},
    {0x18, "3.1M"},
    {0x19, "VGA"},
};

static EnumValueU16 sProp_ExposureMeteringMode[] = {
    {0x0001, "Average"},
    {0x0002, "Center Weighted Average"},
    {0x0003, "Multi Spot"},
    {0x0004, "Center Spot"},
    {0x8001, "Multi"},
    {0x8002, "Center Weighted"},
    {0x8003, "Entire Screen Avg"},
    {0x8004, "Spot: Standard"},
    {0x8005, "Spot: Large"},
    {0x8006, "Highlight"},
    {0x8011, "Standard"},
    {0x8012, "Backlight"},
    {0x8013, "Spotlight"},
};

static EnumValueU16 sProp_FlashMode[] = {
    {0x0001, "Auto flash"},
    {0x0002, "Flash off"},
    {0x0003, "Fill flash"},
    {0x0004, "Red eye auto"},
    {0x0005, "Red eye fill"},
    {0x0006, "External Sync"},
    {0x8001, "Slow Sync"},
    {0x8003, "Rear Sync"},
    {0x8004, "Wireless"},
    {0x8021, "HSS auto"},
    {0x8022, "HSS fill"},
    {0x8024, "HSS WL"},
    {0x8031, "Slow Sync Red Eye On"},
    {0x8032, "Slow Sync Red Eye Off"},
    {0x8041, "Slow Sync Wireless"},
    {0x8042, "Rear Sync Wireless"},
};

static EnumValueU16 sProp_ExposureProgramMode16[] = {
    {0x0001, "Manual (M)"},
    {0x0002, "Automatic (P)"},
    {0x0003, "Aperture Priority (A)"},
    {0x0004, "Shutter Priority (S)"},
    {0x0005, "Program Creative (Greater Depth of Field)"},
    {0x0006, "Program Action (Faster Shutter Speed)"},
    {0x0007, "Portrait"},
    {0x8000, "Auto"},
    {0x8001, "Auto+"},
    {0x8008, "P_A"},
    {0x8009, "P_S"},
    {0x8011, "Sports Action"},
    {0x8012, "Sunset"},
    {0x8013, "Night Scene"},
    {0x8014, "Landscape"},
    {0x8015, "Macro"},
    {0x8016, "Hand-held Twilight"},
    {0x8017, "Night Portrait"},
    {0x8018, "Anti Motion Blur"},
    {0x8019, "Pet"},
    {0x801A, "Gourmet"},
    {0x801B, "Fireworks"},
    {0x801C, "High Sensitivity"},
    {0x8020, "Memory Recall (MR)"},
    {0x8030, "Continuous Priority AE"},
    {0x8031, "Tele-Zoom Continuous Priority AE 8pics"},
    {0x8032, "Tele-Zoom Continuous Priority AE 10pics"},
    {0x8033, "Continuous Priority AE12pics"},
    {0x8040, "3D Sweep Panorama Shooting"},
    {0x8041, "Sweep Panorama Shooting"},
    {0x8050, "Movie Recording (P)"},
    {0x8051, "Movie Recording (A)"},
    {0x8052, "Movie Recording (S)"},
    {0x8053, "Movie Recording (M)"},
    {0x8054, "Movie Recording (Auto)"},
    {0x8059, "Movie Recording (S&Q Motion (P))"},
    {0x805A, "Movie Recording (S&Q Motion (A))"},
    {0x805B, "Movie Recording (S&Q Motion (S))"},
    {0x805C, "Movie Recording (S&Q Motion (M))"},
    {0x805D, "Movie Recording (S&Q Motion (Auto))"},
    {0x8060, "Flash Off"},
    {0x8070, "Picture Effect"},
    {0x8080, "High Frame Rate (P)"},
    {0x8081, "High Frame Rate (A)"},
    {0x8082, "High Frame Rate (S)"},
    {0x8083, "High Frame Rate (M)"},
    {0x8084, "S&Q Motion (P)"},
    {0x8085, "S&Q Motion (A)"},
    {0x8086, "S&Q Motion (S)"},
    {0x8087, "S&Q Motion (M)"},
    {0x8088, "Movie"},
    {0x8089, "Still"},
    {0x808A, "F (Movie or S&Q)"},
    {0x8090, "Movie F Mode"},
    {0x8091, "S&Q F Mode"},
    {0x8092, "Interval REC (Movie) F Mode"},
    {0x8093, "Interval REC (Movie) (P)"},
    {0x8094, "Interval REC (Movie) (A)"},
    {0x8095, "Interval REC (Movie) (S)"},
    {0x8096, "Interval REC (Movie) (M)"},
    {0x8097, "Interval REC (Movie) (Auto)"},
};

static EnumValueU32 sProp_ExposureProgramMode32[] = {
    {0x00000001, "Manual (M)"},
    {0x00010002, "Automatic (P)"},
    {0x00020003, "Aperture Priority (A)"},
    {0x00030004, "Shutter Priority (S)"},
    {0x00000005, "Program Creative (Greater Depth of Field)"},
    {0x00000006, "Program Action (Faster Shutter Speed)"},
    {0x00000007, "Portrait"},
    {0x00048000, "Auto"},
    {0x00048001, "Auto+"},
    {0x00008008, "P_A"},
    {0x00008009, "P_S"},
    {0x00058011, "Sports Action"},
    {0x00058012, "Sunset"},
    {0x00058013, "Night Scene"},
    {0x00058014, "Landscape"},
    {0x00058015, "Macro"},
    {0x00058016, "Hand-held Twilight"},
    {0x00058017, "Night Portrait"},
    {0x00058018, "Anti Motion Blur"},
    {0x00058019, "Pet"},
    {0x0005801A, "Gourmet"},
    {0x0005801B, "Fireworks"},
    {0x0005801C, "High Sensitivity"},
    {0x00008020, "Memory Recall (MR)"},
    {0x00008030, "Continuous Priority AE"},
    {0x00008031, "Tele-Zoom Continuous Priority AE 8pics"},
    {0x00008032, "Tele-Zoom Continuous Priority AE 10pics"},
    {0x00008033, "Continuous Priority AE12pics"},
    {0x00068040, "3D Sweep Panorama Shooting"},
    {0x00068041, "Sweep Panorama Shooting"},
    {0x00078050, "Movie Recording (P)"},
    {0x00078051, "Movie Recording (A)"},
    {0x00078052, "Movie Recording (S)"},
    {0x00078053, "Movie Recording (M)"},
    {0x00078054, "Movie Recording (Auto)"},
    {0x00098059, "Movie Recording (S&Q Motion (P))"},
    {0x0009805A, "Movie Recording (S&Q Motion (A))"},
    {0x0009805B, "Movie Recording (S&Q Motion (S))"},
    {0x0009805C, "Movie Recording (S&Q Motion (M))"},
    {0x0009805D, "Movie Recording (S&Q Motion (Auto))"},
    {0x00008060, "Flash Off"},
    {0x00008070, "Picture Effect"},
    {0x00088080, "High Frame Rate (P)"},
    {0x00088081, "High Frame Rate (A)"},
    {0x00088082, "High Frame Rate (S)"},
    {0x00088083, "High Frame Rate (M)"},
    {0x00008084, "S&Q Motion (P)"},
    {0x00008085, "S&Q Motion (A)"},
    {0x00008086, "S&Q Motion (S)"},
    {0x00008087, "S&Q Motion (M)"},
    {0x000A8088, "Movie"},
    {0x000A8089, "Still"},
    {0x000B808A, "F (Movie or S&Q)"},
    {0x00078090, "Movie F Mode"},
    {0x00098091, "S&Q F Mode"},
    {0x000C8092, "Interval REC (Movie) F Mode"},
    {0x000C8093, "Interval REC (Movie) (P)"},
    {0x000C8094, "Interval REC (Movie) (A)"},
    {0x000C8095, "Interval REC (Movie) (S)"},
    {0x000C8096, "Interval REC (Movie) (M)"},
    {0x000C8097, "Interval REC (Movie) (Auto)"},
};

static EnumValueU32 sProp_CaptureMode32[] = {
    {0x00000001, "Normal"},
    {0x00010002, "Continuous Shooting Hi"},
    {0x00018010, "Continuous Shooting Hi+"},
    {0x00018011, "Continuous Shooting Hi-Live"},
    {0x00018012, "Continuous Shooting Lo"},
    {0x00018013, "Continuous Shooting"},
    {0x00018014, "Continuous Shooting Speed Priority"},
    {0x00018015, "Continuous Shooting Mid"},
    {0x00018016, "Continuous Shooting Mid-Live"},
    {0x00018017, "Continuous Shooting Lo-Live"},
    {0x00020003, "Timelapse"},
    {0x00038003, "Self Timer 5 Sec."},
    {0x00038004, "Self Timer 10 Sec."},
    {0x00038005, "Self Timer 2 Sec."},
    {0x00048337, "Continuous Bracket 0.3 EV 3 Img."},
    {0x00048537, "Continuous Bracket 0.3 EV 5 Img."},
    {0x00048737, "Continuous Bracket 0.3 EV 7 Img."},
    {0x00048937, "Continuous Bracket 0.3 EV 9 Img."},
    {0x00048357, "Continuous Bracket 0.5 EV 3 Img."},
    {0x00048557, "Continuous Bracket 0.5 EV 5 Img."},
    {0x00048757, "Continuous Bracket 0.5 EV 7 Img."},
    {0x00048957, "Continuous Bracket 0.5 EV 9 Img."},
    {0x0004C237, "Continuous Bracket 0.3 EV 2 Img. +"},
    {0x0004C257, "Continuous Bracket 0.5 EV 2 Img. +"},
    {0x0004C23F, "Continuous Bracket 0.3 EV 2 Img. -"},
    {0x0004C25F, "Continuous Bracket 0.5 EV 2 Img. -"},
    {0x0004C277, "Continuous Bracket 0.7 EV 2 Img. +"},
    {0x0004C27F, "Continuous Bracket 0.7 EV 2 Img. -"},
    {0x00048377, "Continuous Bracket 0.7 EV 3 Img."},
    {0x00048577, "Continuous Bracket 0.7 EV 5 Img."},
    {0x00048777, "Continuous Bracket 0.7 EV 7 Img."},
    {0x00048977, "Continuous Bracket 0.7 EV 9 Img."},
    {0x0004C211, "Continuous Bracket 1.0 EV 2 Img. +"},
    {0x0004C219, "Continuous Bracket 1.0 EV 2 Img. -"},
    {0x00048311, "Continuous Bracket 1.0 EV 3 Img."},
    {0x00048511, "Continuous Bracket 1.0 EV 5 Img."},
    {0x00048711, "Continuous Bracket 1.0 EV 7 Img."},
    {0x00048911, "Continuous Bracket 1.0 EV 9 Img."},
    {0x0004C241, "Continuous Bracket 1.3 EV 2 Img. +"},
    {0x0004C249, "Continuous Bracket 1.3 EV 2 Img. -"},
    {0x00048341, "Continuous Bracket 1.3 EV 3 Img."},
    {0x00048541, "Continuous Bracket 1.3 EV 5 Img."},
    {0x00048741, "Continuous Bracket 1.3 EV 7 Img."},
    {0x0004C261, "Continuous Bracket 1.5 EV 2 Img. +"},
    {0x0004C269, "Continuous Bracket 1.5 EV 2 Img. -"},
    {0x00048361, "Continuous Bracket 1.5 EV 3 Img."},
    {0x00048561, "Continuous Bracket 1.5 EV 5 Img."},
    {0x00048761, "Continuous Bracket 1.5 EV 7 Img."},
    {0x0004C281, "Continuous Bracket 1.7 EV 2 Img. +"},
    {0x0004C289, "Continuous Bracket 1.7 EV 2 Img. -"},
    {0x00048381, "Continuous Bracket 1.7 EV 3 Img."},
    {0x00048581, "Continuous Bracket 1.7 EV 5 Img."},
    {0x00048781, "Continuous Bracket 1.7 EV 7 Img."},
    {0x0004C221, "Continuous Bracket 2.0 EV 2 Img. +"},
    {0x0004C229, "Continuous Bracket 2.0 EV 2 Img. -"},
    {0x00048321, "Continuous Bracket 2.0 EV 3 Img."},
    {0x00048521, "Continuous Bracket 2.0 EV 5 Img."},
    {0x00048721, "Continuous Bracket 2.0 EV 7 Img."},
    {0x0004C251, "Continuous Bracket 2.3 EV 2 Img. +"},
    {0x0004C259, "Continuous Bracket 2.3 EV 2 Img. -"},
    {0x00048351, "Continuous Bracket 2.3 EV 3 Img."},
    {0x00048551, "Continuous Bracket 2.3 EV 5 Img."},
    {0x0004C271, "Continuous Bracket 2.5 EV 2 Img. +"},
    {0x0004C279, "Continuous Bracket 2.5 EV 2 Img. -"},
    {0x00048371, "Continuous Bracket 2.5 EV 3 Img."},
    {0x00048571, "Continuous Bracket 2.5 EV 5 Img."},
    {0x0004C291, "Continuous Bracket 2.7 EV 2 Img. +"},
    {0x0004C299, "Continuous Bracket 2.7 EV 2 Img. -"},
    {0x00048391, "Continuous Bracket 2.7 EV 3 Img."},
    {0x00048591, "Continuous Bracket 2.7 EV 5 Img."},
    {0x0004C231, "Continuous Bracket 3.0 EV 2 Img. +"},
    {0x0004C239, "Continuous Bracket 3.0 EV 2 Img. -"},
    {0x00048331, "Continuous Bracket 3.0 EV 3 Img."},
    {0x00048531, "Continuous Bracket 3.0 EV 5 Img."},
    {0x0005C236, "Single Bracket 0.3 EV 2 Img. +"},
    {0x0005C23E, "Single Bracket 0.3 EV 2 Img. -"},
    {0x00058336, "Single Bracket 0.3 EV 3 Img."},
    {0x00058536, "Single Bracket 0.3 EV 5 Img."},
    {0x00058736, "Single Bracket 0.3 EV 7 Img."},
    {0x00058936, "Single Bracket 0.3 EV 9 Img."},
    {0x0005C256, "Single Bracket 0.5 EV 2 Img. +"},
    {0x0005C25E, "Single Bracket 0.5 EV 2 Img. -"},
    {0x00058356, "Single Bracket 0.5 EV 3 Img."},
    {0x00058556, "Single Bracket 0.5 EV 5 Img."},
    {0x00058756, "Single Bracket 0.5 EV 7 Img."},
    {0x00058956, "Single Bracket 0.5 EV 9 Img."},
    {0x0005C276, "Single Bracket 0.7 EV 2 Img. +"},
    {0x0005C27E, "Single Bracket 0.7 EV 2 Img. -"},
    {0x00058376, "Single Bracket 0.7 EV 3 Img."},
    {0x00058576, "Single Bracket 0.7 EV 5 Img."},
    {0x00058776, "Single Bracket 0.7 EV 7 Img."},
    {0x00058976, "Single Bracket 0.7 EV 9 Img."},
    {0x0005C210, "Single Bracket 1.0 EV 2 Img. +"},
    {0x0005C218, "Single Bracket 1.0 EV 2 Img. -"},
    {0x00058310, "Single Bracket 1.0 EV 3 Img."},
    {0x00058510, "Single Bracket 1.0 EV 5 Img."},
    {0x00058710, "Single Bracket 1.0 EV 7 Img."},
    {0x00058910, "Single Bracket 1.0 EV 9 Img."},
    {0x0005C240, "Single Bracket 1.3 EV 2 Img. +"},
    {0x0005C248, "Single Bracket 1.3 EV 2 Img. -"},
    {0x00058340, "Single Bracket 1.3 EV 3 Img."},
    {0x00058540, "Single Bracket 1.3 EV 5 Img."},
    {0x00058740, "Single Bracket 1.3 EV 7 Img."},
    {0x0005C260, "Single Bracket 1.5 EV 2 Img. +"},
    {0x0005C268, "Single Bracket 1.5 EV 2 Img. -"},
    {0x00058360, "Single Bracket 1.5 EV 3 Img."},
    {0x00058560, "Single Bracket 1.5 EV 5 Img."},
    {0x00058760, "Single Bracket 1.5 EV 7 Img."},
    {0x0005C280, "Single Bracket 1.7 EV 2 Img. +"},
    {0x0005C288, "Single Bracket 1.7 EV 2 Img. -"},
    {0x00058380, "Single Bracket 1.7 EV 3 Img."},
    {0x00058580, "Single Bracket 1.7 EV 5 Img."},
    {0x00058780, "Single Bracket 1.7 EV 7 Img."},
    {0x0005C220, "Single Bracket 2.0 EV 2 Img. +"},
    {0x0005C228, "Single Bracket 2.0 EV 2 Img. -"},
    {0x00058320, "Single Bracket 2.0 EV 3 Img."},
    {0x00058520, "Single Bracket 2.0 EV 5 Img."},
    {0x00058720, "Single Bracket 2.0 EV 7 Img."},
    {0x0005C250, "Single Bracket 2.3 EV 2 Img. +"},
    {0x0005C258, "Single Bracket 2.3 EV 2 Img. -"},
    {0x00058350, "Single Bracket 2.3 EV 3 Img."},
    {0x00058550, "Single Bracket 2.3 EV 5 Img."},
    {0x0005C270, "Single Bracket 2.5 EV 2 Img. +"},
    {0x0005C278, "Single Bracket 2.5 EV 2 Img. -"},
    {0x00058370, "Single Bracket 2.5 EV 3 Img."},
    {0x00058570, "Single Bracket 2.5 EV 5 Img."},
    {0x0005C290, "Single Bracket 2.7 EV 2 Img. +"},
    {0x0005C298, "Single Bracket 2.7 EV 2 Img. -"},
    {0x00058390, "Single Bracket 2.7 EV 3 Img."},
    {0x00058590, "Single Bracket 2.7 EV 5 Img."},
    {0x0005C230, "Single Bracket 3.0 EV 2 Img. +"},
    {0x0005C238, "Single Bracket 3.0 EV 2 Img. -"},
    {0x00058330, "Single Bracket 3.0 EV 3 Img."},
    {0x00058530, "Single Bracket 3.0 EV 5 Img."},
    {0x00068018, "White Balance Bracket Lo"},
    {0x00068028, "White Balance Bracket Hi"},
    {0x00078019, "DRO Bracket Lo"},
    {0x00078029, "DRO Bracket Hi"},
    {0x0007801A, "LPF Bracket"},
    {0x0007800A, "Remote Commander"},
    {0x0007800B, "Mirror Up"},
    {0x00078006, "Self Portrait 1 Person"},
    {0x00078007, "Self Portrait 2 People"},
    {0x00088008, "Continuous Self Timer 3 Img."},
    {0x00088009, "Continuous Self Timer 5 Img."},
    {0x0008800C, "Continuous Self Timer 3 Img. 5 Sec."},
    {0x0008800D, "Continuous Self Timer 5 Img. 5 Sec."},
    {0x0008800E, "Continuous Self Timer 3 Img. 2 Sec."},
    {0x0008800F, "Continuous Self Timer 5 Img. 2 Sec."},
    {0x00098030, "Spot Burst Shooting Lo"},
    {0x00098031, "Spot Burst Shooting Mid"},
    {0x00098032, "Spot Burst Shooting Hi"},
    {0x000A8040, "Focus Bracket"},
};

static EnumValueU16 sProp_CaptureMode16[] = {
    {0x0001, "Normal"},
    {0x0002, "Continuous Shooting Hi"},
    {0x0003, "Timelapse"},
    {0x8003, "Self Timer 5 Sec."},
    {0x8004, "Self Timer 10 Sec."},
    {0x8005, "Self Timer 2 Sec."},
    {0x8006, "Self Portrait 1 Person"},
    {0x8007, "Self Portrait 2 People"},
    {0x8008, "Continuous Self Timer 3 Img."},
    {0x8009, "Continuous Self Timer 5 Img."},
    {0x800A, "Remote Commander"},
    {0x800B, "Mirror Up"},
    {0x800C, "Continuous Self Timer 3 Img. 5 Sec."},
    {0x800D, "Continuous Self Timer 5 Img. 5 Sec."},
    {0x800E, "Continuous Self Timer 3 Img. 2 Sec."},
    {0x800F, "Continuous Self Timer 5 Img. 2 Sec."},
    {0x8010, "Continuous Shooting Hi+"},
    {0x8011, "Continuous Shooting Hi-Live"},
    {0x8012, "Continuous Shooting Lo"},
    {0x8013, "Continuous Shooting"},
    {0x8014, "Continuous Shooting Speed Priority"},
    {0x8015, "Continuous Shooting Mid"},
    {0x8016, "Continuous Shooting Mid-Live"},
    {0x8017, "Continuous Shooting Lo-Live"},
    {0x8018, "White Balance Bracket Lo"},
    {0x8019, "DRO Bracket Lo"},
    {0x801A, "LPF Bracket"},
    {0x8028, "White Balance Bracket Hi"},
    {0x8029, "DRO Bracket Hi"},
    {0x8030, "Spot Burst Shooting Lo"},
    {0x8031, "Spot Burst Shooting Mid"},
    {0x8032, "Spot Burst Shooting Hi"},
    {0x8040, "Focus Bracket"},
    {0x8337, "Continuous Bracket 0.3 EV 3 Img."},
    {0x8537, "Continuous Bracket 0.3 EV 5 Img."},
    {0x8737, "Continuous Bracket 0.3 EV 7 Img."},
    {0x8937, "Continuous Bracket 0.3 EV 9 Img."},
    {0x8357, "Continuous Bracket 0.5 EV 3 Img."},
    {0x8557, "Continuous Bracket 0.5 EV 5 Img."},
    {0x8757, "Continuous Bracket 0.5 EV 7 Img."},
    {0x8957, "Continuous Bracket 0.5 EV 9 Img."},
    {0xC237, "Continuous Bracket 0.3 EV 2 Img. +"},
    {0xC257, "Continuous Bracket 0.5 EV 2 Img. +"},
    {0xC23F, "Continuous Bracket 0.3 EV 2 Img. -"},
    {0xC25F, "Continuous Bracket 0.5 EV 2 Img. -"},
    {0xC277, "Continuous Bracket 0.7 EV 2 Img. +"},
    {0xC27F, "Continuous Bracket 0.7 EV 2 Img. -"},
    {0x8377, "Continuous Bracket 0.7 EV 3 Img."},
    {0x8577, "Continuous Bracket 0.7 EV 5 Img."},
    {0x8777, "Continuous Bracket 0.7 EV 7 Img."},
    {0x8977, "Continuous Bracket 0.7 EV 9 Img."},
    {0xC211, "Continuous Bracket 1.0 EV 2 Img. +"},
    {0xC219, "Continuous Bracket 1.0 EV 2 Img. -"},
    {0x8311, "Continuous Bracket 1.0 EV 3 Img."},
    {0x8511, "Continuous Bracket 1.0 EV 5 Img."},
    {0x8711, "Continuous Bracket 1.0 EV 7 Img."},
    {0x8911, "Continuous Bracket 1.0 EV 9 Img."},
    {0xC241, "Continuous Bracket 1.3 EV 2 Img. +"},
    {0xC249, "Continuous Bracket 1.3 EV 2 Img. -"},
    {0x8341, "Continuous Bracket 1.3 EV 3 Img."},
    {0x8541, "Continuous Bracket 1.3 EV 5 Img."},
    {0x8741, "Continuous Bracket 1.3 EV 7 Img."},
    {0xC261, "Continuous Bracket 1.5 EV 2 Img. +"},
    {0xC269, "Continuous Bracket 1.5 EV 2 Img. -"},
    {0x8361, "Continuous Bracket 1.5 EV 3 Img."},
    {0x8561, "Continuous Bracket 1.5 EV 5 Img."},
    {0x8761, "Continuous Bracket 1.5 EV 7 Img."},
    {0xC281, "Continuous Bracket 1.7 EV 2 Img. +"},
    {0xC289, "Continuous Bracket 1.7 EV 2 Img. -"},
    {0x8381, "Continuous Bracket 1.7 EV 3 Img."},
    {0x8581, "Continuous Bracket 1.7 EV 5 Img."},
    {0x8781, "Continuous Bracket 1.7 EV 7 Img."},
    {0xC221, "Continuous Bracket 2.0 EV 2 Img. +"},
    {0xC229, "Continuous Bracket 2.0 EV 2 Img. -"},
    {0x8321, "Continuous Bracket 2.0 EV 3 Img."},
    {0x8521, "Continuous Bracket 2.0 EV 5 Img."},
    {0x8721, "Continuous Bracket 2.0 EV 7 Img."},
    {0xC251, "Continuous Bracket 2.3 EV 2 Img. +"},
    {0xC259, "Continuous Bracket 2.3 EV 2 Img. -"},
    {0x8351, "Continuous Bracket 2.3 EV 3 Img."},
    {0x8551, "Continuous Bracket 2.3 EV 5 Img."},
    {0xC271, "Continuous Bracket 2.5 EV 2 Img. +"},
    {0xC279, "Continuous Bracket 2.5 EV 2 Img. -"},
    {0x8371, "Continuous Bracket 2.5 EV 3 Img."},
    {0x8571, "Continuous Bracket 2.5 EV 5 Img."},
    {0xC291, "Continuous Bracket 2.7 EV 2 Img. +"},
    {0xC299, "Continuous Bracket 2.7 EV 2 Img. -"},
    {0x8391, "Continuous Bracket 2.7 EV 3 Img."},
    {0x8591, "Continuous Bracket 2.7 EV 5 Img."},
    {0xC231, "Continuous Bracket 3.0 EV 2 Img. +"},
    {0xC239, "Continuous Bracket 3.0 EV 2 Img. -"},
    {0x8331, "Continuous Bracket 3.0 EV 3 Img."},
    {0x8531, "Continuous Bracket 3.0 EV 5 Img."},
    {0xC236, "Single Bracket 0.3 EV 2 Img. +"},
    {0xC23E, "Single Bracket 0.3 EV 2 Img. -"},
    {0x8336, "Single Bracket 0.3 EV 3 Img."},
    {0x8536, "Single Bracket 0.3 EV 5 Img."},
    {0x8736, "Single Bracket 0.3 EV 7 Img."},
    {0x8936, "Single Bracket 0.3 EV 9 Img."},
    {0xC256, "Single Bracket 0.5 EV 2 Img. +"},
    {0xC25E, "Single Bracket 0.5 EV 2 Img. -"},
    {0x8356, "Single Bracket 0.5 EV 3 Img."},
    {0x8556, "Single Bracket 0.5 EV 5 Img."},
    {0x8756, "Single Bracket 0.5 EV 7 Img."},
    {0x8956, "Single Bracket 0.5 EV 9 Img."},
    {0xC276, "Single Bracket 0.7 EV 2 Img. +"},
    {0xC27E, "Single Bracket 0.7 EV 2 Img. -"},
    {0x8376, "Single Bracket 0.7 EV 3 Img."},
    {0x8576, "Single Bracket 0.7 EV 5 Img."},
    {0x8776, "Single Bracket 0.7 EV 7 Img."},
    {0x8976, "Single Bracket 0.7 EV 9 Img."},
    {0xC210, "Single Bracket 1.0 EV 2 Img. +"},
    {0xC218, "Single Bracket 1.0 EV 2 Img. -"},
    {0x8310, "Single Bracket 1.0 EV 3 Img."},
    {0x8510, "Single Bracket 1.0 EV 5 Img."},
    {0x8710, "Single Bracket 1.0 EV 7 Img."},
    {0x8910, "Single Bracket 1.0 EV 9 Img."},
    {0xC240, "Single Bracket 1.3 EV 2 Img. +"},
    {0xC248, "Single Bracket 1.3 EV 2 Img. -"},
    {0x8340, "Single Bracket 1.3 EV 3 Img."},
    {0x8540, "Single Bracket 1.3 EV 5 Img."},
    {0x8740, "Single Bracket 1.3 EV 7 Img."},
    {0xC260, "Single Bracket 1.5 EV 2 Img. +"},
    {0xC268, "Single Bracket 1.5 EV 2 Img. -"},
    {0x8360, "Single Bracket 1.5 EV 3 Img."},
    {0x8560, "Single Bracket 1.5 EV 5 Img."},
    {0x8760, "Single Bracket 1.5 EV 7 Img."},
    {0xC280, "Single Bracket 1.7 EV 2 Img. +"},
    {0xC288, "Single Bracket 1.7 EV 2 Img. -"},
    {0x8380, "Single Bracket 1.7 EV 3 Img."},
    {0x8580, "Single Bracket 1.7 EV 5 Img."},
    {0x8780, "Single Bracket 1.7 EV 7 Img."},
    {0xC220, "Single Bracket 2.0 EV 2 Img. +"},
    {0xC228, "Single Bracket 2.0 EV 2 Img. -"},
    {0x8320, "Single Bracket 2.0 EV 3 Img."},
    {0x8520, "Single Bracket 2.0 EV 5 Img."},
    {0x8720, "Single Bracket 2.0 EV 7 Img."},
    {0xC250, "Single Bracket 2.3 EV 2 Img. +"},
    {0xC258, "Single Bracket 2.3 EV 2 Img. -"},
    {0x8350, "Single Bracket 2.3 EV 3 Img."},
    {0x8550, "Single Bracket 2.3 EV 5 Img."},
    {0xC270, "Single Bracket 2.5 EV 2 Img. +"},
    {0xC278, "Single Bracket 2.5 EV 2 Img. -"},
    {0x8370, "Single Bracket 2.5 EV 3 Img."},
    {0x8570, "Single Bracket 2.5 EV 5 Img."},
    {0xC290, "Single Bracket 2.7 EV 2 Img. +"},
    {0xC298, "Single Bracket 2.7 EV 2 Img. -"},
    {0x8390, "Single Bracket 2.7 EV 3 Img."},
    {0x8590, "Single Bracket 2.7 EV 5 Img."},
    {0xC230, "Single Bracket 3.0 EV 2 Img. +"},
    {0xC238, "Single Bracket 3.0 EV 2 Img. -"},
    {0x8330, "Single Bracket 3.0 EV 3 Img."},
    {0x8530, "Single Bracket 3.0 EV 5 Img."},
};

static EnumValueU8 sProp_IrisMode[] = {
    {0x01, "Automatic"},
    {0x02, "Manual"},
};

static EnumValueU8 sProp_ApertureDriveAF[] = {
    {0x01, "Not Target"},
    {0x02, "Standard"},
    {0x03, "Silent Priority"},
};

static EnumValueU8 sProp_SilentModePowerOff[] = {
    {0x01, "Not Target"},
    {0x02, "Off"},
};

static EnumValueU8 sProp_SilentModeAutoPixelMapping[] = {
    {0x01, "Not Target"},
    {0x02, "Off"},
};

static EnumValueU8 sProp_ShutterType[] = {
    {0x01, "Auto"},
    {0x02, "Mechanical Shutter"},
    {0x03, "Electronic Shutter"},
};

static EnumValueU8 sProp_ShutterMode[] = {
    {0x01, "Speed"},
    {0x02, "Angle"},
};

static EnumValueU8 sProp_ShutterModeSetting[] = {
    {0x01, "Automatic"},
    {0x02, "Manual"},
};

static EnumValueU8 sProp_GainControl[] = {
    {0x01, "Automatic"},
    {0x02, "Manual"},
};

static EnumValueU8 sProp_DRO[] = {
    {0x01, "DRO Off"},
    {0x02, "DRO"},
    {0x10, "DRO+"},
    {0x11, "DRO + Manual1"},
    {0x12, "DRO + Manual2"},
    {0x13, "DRO + Manual3"},
    {0x14, "DRO + Manual4"},
    {0x15, "DRO + Manual5"},
    {0x1F, "DRO Auto"},
    {0x20, "HDR Auto"},
    {0x21, "HDR 1.0EV"},
    {0x22, "HDR 2.0EV"},
    {0x23, "HDR 3.0EV"},
    {0x24, "HDR 4.0EV"},
    {0x25, "HDR 5.0EV"},
    {0x26, "HDR 6.0EV"},
};

static EnumValueU8 sProp_LiveViewStatus[] = {
    {0x00, "Disabled"}, // Supported, don't get Live View yet... wait for 'Enabled' status
    {0x01, "Enabled"},
    {0x02, "Not Supported"},
};

static EnumValueU8 sProp_TimeCodeFormat[] = {
    {0x01, "NF"},
    {0x02, "NDF"},
};

static EnumValueU8 sProp_AspectRatio[] = {
    {0x01, "3:2"},
    {0x02, "16:9"},
    {0x03, "4:3"},
    {0x04, "1:1"},
};

static EnumValueU16 sProp_PictureEffect[] = {
    {0x8000, "Off"},
    {0x8001, "Toy Camera Normal"},
    {0x8002, "Toy Camera Cool"},
    {0x8003, "Toy Camera Warm"},
    {0x8004, "Toy Camera Green"},
    {0x8005, "Toy Camera Magenta"},
    {0x8010, "Pop Color"},
    {0x8020, "Posterization B/W"},
    {0x8021, "Posterization Color"},
    {0x8030, "Retro Photo"},
    {0x8040, "Soft High-key"},
    {0x8050, "Partial Color Red"},
    {0x8051, "Partial Color Green"},
    {0x8052, "Partial Color Blue"},
    {0x8053, "Partial Color Yellow"},
    {0x8060, "High Contrast Mono"},
    {0x8070, "Soft Focus Low"},
    {0x8071, "Soft Focus Mid"},
    {0x8072, "Soft Focus High"},
    {0x8080, "HDR Painting Low"},
    {0x8081, "HDR Painting Mid"},
    {0x8082, "HDR Painting High"},
    {0x8090, "Rich-tone Mono"},
    {0x80A0, "Miniature Auto"},
    {0x80A1, "Miniature Top"},
    {0x80A2, "Miniature Middle(Horizontal"},
    {0x80A3, "Miniature Bottom"},
    {0x80A4, "Miniature Right"},
    {0x80A5, "Miniature Middle(Vertical)"},
    {0x80A6, "Miniature Left"},
    {0x80B0, "Miniature Wator Color"},
    {0x80C0, "Miniature Illustration Low"},
    {0x80C1, "Miniature Illustration Mid"},
    {0x80C2, "Miniature Illustration High"},
};

static EnumValueU16 sProp_PcRemoteSaveDest[] = {
    {0x0001, "PC"},
    {0x0010, "Camera Media Card"},
    {0x0011, "PC & Camera Media Card"},
};

static EnumValueU16 sProp_FocusArea[] = {
    {0x0000, "Unknown"},
    {0x0001, "Wide"},
    {0x0002, "Zone"},
    {0x0003, "Center"},
    {0x0101, "Flexible Spot S"},
    {0x0102, "Flexible Spot M"},
    {0x0103, "Flexible Spot L"},
    {0x0104, "Expand Flexible Spot"},
    {0x0105, "Flexible Spot"},
    {0x0106, "Flexible Spot XS"},
    {0x0107, "Flexible Spot XL"},
    {0x1101, "Flexible Spot Free Size 1"},
    {0x1102, "Flexible Spot Free Size 2"},
    {0x1103, "Flexible Spot Free Size 3"},
    {0x0201, "Lock on AF Wide"},
    {0x0202, "Lock on AF Zone"},
    {0x0203, "Lock on AF Center"},
    {0x0204, "Lock on AF Flexible Spot S"},
    {0x0205, "Lock on AF Flexible Spot M"},
    {0x0206, "Lock on AF Flexible Spot L"},
    {0x0207, "Lock on Expand Flexible Spot"},
    {0x0208, "Lock on AF Flexible Spot"},
    {0x0209, "Lock on AF Flexible Spot XS"},
    {0x020A, "Lock on AF Flexible Spot XL"},
    {0x1201, "Lock on AF Flexible Spot Free Size 1"},
    {0x1202, "Lock on AF Flexible Spot Free Size 2"},
    {0x1203, "Lock on AF Flexible Spot Free Size 3"},
};

static EnumValueU8 sProp_LiveViewSettingEffect[] = {
    {0x00, "N/A"},
    {0x01, "On"},
    {0x02, "Off"}
};

static EnumValueU8 sProp_PictureProfile[] = {
    {0x00, "Off"},
    {0x01, "Profile 1"},
    {0x02, "Profile 2"},
    {0x03, "Profile 3"},
    {0x04, "Profile 4"},
    {0x05, "Profile 5"},
    {0x06, "Profile 6"},
    {0x07, "Profile 7"},
    {0x08, "Profile 8"},
    {0x09, "Profile 9"},
    {0x0A, "Profile 10"},
    {0x0B, "Profile 11"},
    {0x41, "Profile LUT 1"},
    {0x42, "Profile LUT 2"},
    {0x43, "Profile LUT 3"},
    {0x44, "Profile LUT 4"},
};

static EnumValueU8 sProp_CreativeStyle[] = {
    {0x01, "Standard"},
    {0x02, "Vivid"},
    {0x03, "Portrait"},
    {0x04, "Landscape"},
    {0x05, "Sunset"},
    {0x06, "B/W (Black & White)"},
    {0x07, "Light"},
    {0x08, "Neutral"},
    {0x09, "Clear"},
    {0x0A, "Deep"},
    {0x0B, "Night View"},
    {0x0C, "Autumn Leaves"},
    {0x0D, "Sepia"},
    {0x0E, "Creative BOX1"},
    {0x0F, "Creative BOX2"},
    {0x10, "Creative BOX3"},
    {0x11, "Creative BOX4"},
    {0x12, "Creative BOX5"},
    {0x13, "Creative BOX6"},
};

static EnumValueU16 sProp_CreativeLook[] = {
    {0x01, "ST (Standard)"},
    {0x0002, "PT (Portrait)"},
    {0x0003, "NT (Neutral)"},
    {0x0004, "VV (Vivid)"},
    {0x0005, "VV2 (Vivid2)"},
    {0x0006, "FL (Film)"},
    {0x0007, "IN (Instant)"},
    {0x0008, "SH (Soft Highkey)"},
    {0x0009, "BW (Black & White)"},
    {0x000A, "SE (Sepia)"},
    {0x000B, "FL2 (Film2)"},
    {0x000C, "FL3 (Film3)"},
    {0x0101, "Custom Look 1"},
    {0x0102, "Custom Look 2"},
    {0x0103, "Custom Look 3"},
    {0x0104, "Custom Look 4"},
    {0x0105, "Custom Look 5"},
    {0x0106, "Custom Look 6"},
};

static EnumValueU8 sProp_MovieFormat[] = {
    {0x01, "DVD"},
    {0x02, "M2PS"},
    {0x03, "AVCHD"},
    {0x04, "MP4"},
    {0x05, "DV"},
    {0x06, "XAVC"},
    {0x07, "MXF"},
    {0x08, "XAVC S 4K"},
    {0x09, "XAVC S HD"},
    {0x0A, "XAVC HS 8K"},
    {0x0B, "XAVC HS 4K"},
    {0x0C, "XAVC S-L 4K"},
    {0x0D, "XAVC S-L HD"},
    {0x0E, "XAVC S-I 4K"},
    {0x0F, "XAVC S-I HD"},
    {0x10, "XAVC I"},
    {0x11, "XAVC L"},
    {0x12, "XAVC Proxy"},
    {0x13, "XAVC HS HD"},
    {0x14, "XAVC S-I DCI 4K"},
    {0x15, "XAVC H-I HQ"},
    {0x16, "XAVC H-I SQ"},
    {0x17, "XAVC H-L"},
    {0x18, "X-OCN XT"},
    {0x19, "X-OCN ST"},
    {0x1A, "X-OCN LT"},
    {0x1B, "XAVC HS-L 422"},
    {0x1C, "XAVC HS-L 420"},
    {0x1D, "XAVC S-L 422"},
    {0x1E, "XAVC S-L 420"},
    {0x1F, "XAVC S-I 422"},
};

static EnumValueU16 sProp_MovieQuality[] = {
    {0x0001, "60p 50M / XAVC S"},
    {0x0002, "30p 50M / XAVC S"},
    {0x0003, "24p 50M / XAVC S"},
    {0x0004, "50p 50M / XAVC S"},
    {0x0005, "25p 50M / XAVC S"},
    {0x0006, "60i 24M(FX) / AVCHD"},
    {0x0007, "50i 24M(FX) / AVCHD"},
    {0x0008, "60i 17M(FH) / AVCHD"},
    {0x0009, "50i 17M(FH) / AVCHD"},
    {0x000A, "60p 28M(PS) / AVCHD"},
    {0x000B, "50p 28M(PS) / AVCHD"},
    {0x000C, "24p 24M(FX) / AVCHD"},
    {0x000D, "25p 24M(FX) / AVCHD"},
    {0x000E, "24p 17M(FH) / AVCHD"},
    {0x000F, "25p 17M(FH) / AVCHD"},
    {0x0010, "120p 50M (1280x720) / XAVC S"},
    {0x0011, "100p 50M (1280x720) / XAVC S"},
    {0x0012, "1920x1080 30p 16M / MP4"},
    {0x0013, "1920x1080 25p 16M / MP4"},
    {0x0014, "1280x720 30p 6M / MP4"},
    {0x0015, "1280x720 25p 6M / MP4"},
    {0x0016, "1920x1080 60p 28M / MP4"},
    {0x0017, "1920x1080 50p 28M / MP4"},
    {0x0018, "60p 25M / XAVC S HD"},
    {0x0019, "50p 25M / XAVC S HD"},
    {0x001A, "30p 16M / XAVC S HD"},
    {0x001B, "25p 16M / XAVC S HD"},
    {0x001C, "120p 100M (1920x1080) / XAVC S HD"},
    {0x001D, "100p 100M (1920x1080) / XAVC S HD"},
    {0x001E, "120p 60M (1920x1080) / XAVC S HD"},
    {0x001F, "100p 60M (1920x1080) / XAVC S HD"},
    {0x0020, "30p 100M / XAVC S 4K"},
    {0x0021, "25p 100M / XAVC S 4K"},
    {0x0022, "24p 100M / XAVC S 4K"},
    {0x0023, "30p 60M / XAVC S 4K"},
    {0x0024, "25p 60M / XAVC S 4K"},
    {0x0025, "24p 60M / XAVC S 4K"},
    {0x0026, "600M 422 10bit"},
    {0x0027, "500M 422 10bit"},
    {0x0028, "400M 420 10bit"},
    {0x0029, "300M 422 10bit"},
    {0x002A, "280M 422 10bit"},
    {0x002B, "250M 422 10bit"},
    {0x002C, "240M 422 10bit"},
    {0x002D, "222M 422 10bit"},
    {0x002E, "200M 422 10bit"},
    {0x002F, "200M 420 10bit"},
    {0x0030, "200M 420 8bit"},
    {0x0031, "185M 422 10bit"},
    {0x0032, "150M 420 10bit"},
    {0x0033, "150M 420 8bit"},
    {0x0034, "140M 422 10bit"},
    {0x0035, "111M 422 10bit"},
    {0x0036, "100M 422 10bit"},
    {0x0037, "100M 420 10bit"},
    {0x0038, "100M 420 8bit"},
    {0x0039, "93M 422 10bit"},
    {0x003A, "89M 422 10bit"},
    {0x003B, "75M 420 10bit"},
    {0x003C, "60M 420 8bit"},
    {0x003D, "50M 422 10bit"},
    {0x003E, "50M 420 10bit"},
    {0x003F, "50M 420 8bit"},
    {0x0040, "45M 420 10bit"},
    {0x0041, "30M 420 10bit"},
    {0x0042, "25M 420 8bit"},
    {0x0043, "16M 420 8bit"},
    {0x0044, "520M 422 10bit"},
    {0x0045, "260M 422 10bit"},
};

static EnumValueU8 sProp_ImageQuality[] = {
    {0x01, "Extra Fine"},
    {0x02, "Fine"},
    {0x03, "Standard"},
    {0x04, "Light"},
};

static EnumValueU8 sProp_ImageFileFormat[] = {
    {0x01, "RAW"},
    {0x02, "RAW & JPEG"},
    {0x03, "JPEG"},
    {0x04, "RAW & HEIF"},
    {0x05, "HEIF"},
};

static EnumValueU8 sProp_AFTrackingSensitivity[] = {
    {0x01, "1 (Locked on)"},
    {0x02, "2"},
    {0x03, "3 (Standard)"},
    {0x04, "4"},
    {0x05, "5 (Responsive)"},
};

static EnumValueU8 sProp_ZoomSetting[] = {
    {0x01, "Optical Zoom"},
    {0x02, "Smart Zoom"},
    {0x03, "Clear Image Zoom"},
    {0x04, "Digital Zoom"},
};

static EnumValueU8 sProp_PcSaveImage[] = {
    {0x01, "RAW & JPEG"},
    {0x02, "JPEG Only"},
    {0x03, "RAW Only"},
    {0x04, "RAW & HEIF"},
    {0x05, "HEIF Only"},
};

static EnumValueU8 sProp_ImageTransferSize[] = {
    {0x01, "Original"},
    {0x02, "Small Size JPEG"},
};

static EnumValueU8 sProp_LiveViewImageQuality[] = {
    {0x01, "Low"},
    {0x02, "High"},
};

static EnumValueU8 sProp_RawFileType[] = {
    {0x01, "Compressed"},
    {0x02, "Lossless L"},
    {0x03, "Lossless M"},
    {0x04, "Lossless S"},
    {0x05, "No Compression"},
    {0x06, "Lossless"},
};

static EnumValueU8 sProp_CompressedImageFileFormat[] = {
    {0x01, "JPEG"},
    {0x02, "HEIF (4:2:2)"},
    {0x03, "HEIF (4:2:0)"},
};

static EnumValueU8 sProp_TouchOperation[] = {
    {0x01, "Off"},
    {0x02, "On"},
    {0x03, "Playback Only"},
};

static EnumValueU8 sProp_TouchOperationFunction[] = {
    {0x01, "OFF"},
    {0x02, "Touch Shutter"},
    {0x03, "Touch Focus"},
    {0x04, "Touch Tracking"},
    {0x05, "Touch AE"},
    {0x06, "Touch Shutter + Touch AE ON"},
    {0x07, "Touch Shutter + Touch AE OFF"},
    {0x08, "Touch Focus + Touch AE ON"},
    {0x09, "Touch Focus + Touch AE OFF"},
    {0x0A, "Touch Tracking + Touch AE ON"},
    {0x0B, "Touch Tracking + Touch AE OFF"},
};

static EnumValueU8 sProp_BatteryLevel[] = {
    {0x01, "Fake Battery"},
    {0x02, "N/A"},
    {0x03, "Empty"},
    {0x04, "1/4"},
    {0x05, "2/4"},
    {0x06, "3/4"},
    {0x07, "4/4"},
    {0x08, "1/3"},
    {0x09, "2/3"},
    {0x0A, "3/3"},
    {0x0B, "Empty (USB Power)"},
    {0x0C, "1/4 (USB Power)"},
    {0x0D, "2/4 (USB Power)"},
    {0x0E, "3/4 (USB Power)"},
    {0x0F, "4/4 (USB Power)"},
    {0x10, "USB Power"},
    {0xFF, "No Battery"},
};

static EnumValueU8 sProp_MovieRecState[] = {
   {0x00, "Not Recording"},
   {0x01, "Recording"},
   {0x02, "Recording Failed"},
   {0x03, "Waiting Record"},
};

static EnumValueU8 sProp_MovieFrameRate[] = {
    {0x01, "120p"},
    {0x02, "100p"},
    {0x03, "60p"},
    {0x04, "50p"},
    {0x05, "30p"},
    {0x06, "25p"},
    {0x07, "24p"},
    {0x08, "23.98p"},
    {0x09, "29.97p"},
    {0x0A, "59.94p"},
    {0x0B, "19.98p"},
    {0x0C, "14.99p"},
    {0x0D, "12.50p"},
    {0x0E, "12.00p"},
    {0x0F, "11.99p"},
    {0x10, "10.00p"},
    {0x11, "9.99p"},
    {0x12, "6.00p"},
    {0x13, "5.99p"},
    {0x14, "5.00p"},
    {0x15, "4.995p"},
    {0x16, "24.00p"},
    {0x17, "119.88p"},
    {0x41, "120i"},
    {0x42, "100i"},
    {0x43, "60i"},
    {0x44, "50i"},
    {0x45, "30i"},
    {0x46, "25i"},
    {0x47, "24i"},
    {0x48, "23.98i"},
    {0x49, "29.97i"},
    {0x4A, "59.94i"},
    {0x4B, "19.98i"},
    {0x4C, "14.99i"},
    {0x4D, "12.50i"},
    {0x4E, "12.00i"},
    {0x4F, "11.99i"},
    {0x50, "10.00i"},
    {0x51, "9.99i"},
    {0x52, "6.00i"},
    {0x53, "5.99i"},
    {0x54, "5.00i"},
    {0x55, "4.995i"},
    {0x56, "24.00i"},
    {0x57, "119.88i"},
};

static EnumValueU8 sProp_AutoFocusStatus[] = {
    {0x01, "Unlock"},
    {0x02, "[AF-S] Focused, AF Locked"},
    {0x03, "[AF-S] No focus / Low Contrast"},
    {0x04, "Not Used"},
    {0x05, "[AF-C] Tracking"},
    {0x06, "[AF-C] Focused"},
    {0x07, "[AF-C] No focus / Low Contrast"},
    {0x08, "Unpause"},
    {0x09, "Pause"},
};

static EnumValueU8 sProp_MediaPlayback[] = {
    {0x01, "Slot 1"},
    {0x02, "Slot 2"},
};

static EnumValueU8 sProp_MediaSlotStatus[] = {
    {0x01, "OK"},
    {0x02, "No Card"},
    {0x03, "Card Error"},
    {0x04, "Card Recognizing / Card Locked / DB Error"},
    {0x05, "DB Error"},
    {0x06, "Card Recognizing"},
    {0x07, "Card Locked and DB Error"},
    {0x08, "DB Error (Needs Reformat)"},
    {0x09, "Card Error (Read-Only)"},
};

static EnumValueU8 sProp_DeviceOverheatingState[] = {
    {0x00, "Not Overheating"},
    {0x01, "Pre Overheating"},
    {0x02, "Overheating"},
};

static EnumValueU8 sProp_LockedUnlocked[] = {
    {0x01, "Unlocked"},
    {0x02, "Locked"},
};

static EnumValueU8 sProp_IntervalRecStatus[] = {
    {0x00, "Waiting"},
    {0x01, "Shooting"},
};

static EnumValueU8 sProp_ExposureModeKey[] = {
    {0x00, "Camera"},
    {0x01, "PC Remote"},
};

static MStr GetFocusMagnifyScale(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u16 value = propValue.u16;
    int whole = value / 10;
    int decimals = (value % 10);
    char text[32];
    int len = 0;
    if (whole >= 10 && decimals == 0) {
        len = snprintf(text, sizeof(text), "%d", whole);
    } else {
        len = snprintf(text, sizeof(text), "%d.%d", whole, decimals);
    }
    if (len > 0) {
        len++;
        r.str = MMalloc(len);
        memcpy(r.str, text, len);
        r.size = len;
    }
    return r;
}

static MStr GetFocusMagnifyPos(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u32 value = propValue.u32;
    u32 x = (value >> 16) & 0xffff;
    u32 y = value & 0xffff;
    char text[32];
    int len = snprintf(text, sizeof(text), "%d, %d", x, y);
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

static MStr GetFocusSpotPos(PTPControl* self, PTPProperty* property, PTPPropValue propValue) {
    MStr r = {};
    u32 value = propValue.u32;
    u32 x = (value >> 16) & 0xffff;
    u32 y = value & 0xffff;
    char text[32];
    int len = snprintf(text, sizeof(text), "%d, %d", x, y);
    if (len > 0) {
        r.str = MMalloc(len+1);
        memcpy(r.str, text, len+1);
        r.size = len + 1;
    }
    return r;
}

typedef union uFixedEnums {
    EnumValueU8* u8;
    EnumValueU16* u16;
    EnumValueU32* u32;
} FixedEnums;

typedef b32 (*PTP_PropBuildEnumsFunc_t)(PTPControl* self, PTPProperty* property, PTPPropValueEnums* outEnums);
typedef MStr (*PTP_PropValueAsStringFunc_t)(PTPControl* self, PTPProperty* property, PTPPropValue value);

typedef struct {
    char* name;
    u16 propCode;
    u16 type;

    FixedEnums fixedEnums;
    u32 fixedEnumsSize;

    PTP_PropBuildEnumsFunc_t buildEnumsFunc;
    PTP_PropValueAsStringFunc_t valueAsStringFunc;
} PropertyMetadata;

#define META_ENUM_U8(n, c, e) {n, c, PTP_DT_UINT8, .fixedEnums.u8=(e), .fixedEnumsSize=MStaticArraySize(e)}
#define META_ENUM_I16(n, c, e) {n, c, PTP_DT_IINT16, .fixedEnums.i16=(e), .fixedEnumsSize=MStaticArraySize(e)}
#define META_ENUM_U16(n, c, e) {n, c, PTP_DT_UINT16, .fixedEnums.u16=(e), .fixedEnumsSize=MStaticArraySize(e)}
#define META_ENUM_U32(n, c, e) {n, c, PTP_DT_UINT32, .fixedEnums.u32=(e), .fixedEnumsSize=MStaticArraySize(e)}
#define META_FUNC_I8(n, c, f, g) {n, c, PTP_DT_INT8, .buildEnumsFunc=(f), .valueAsStringFunc=(g)}
#define META_FUNC_U8(n, c, f, g) {n, c, PTP_DT_UINT8, .buildEnumsFunc=(f), .valueAsStringFunc=(g)}
#define META_FUNC_I16(n, c, f, g) {n, c, PTP_DT_INT16, .buildEnumsFunc=(f), .valueAsStringFunc=(g)}
#define META_FUNC_U16(n, c, f, g) {n, c, PTP_DT_UINT16, .buildEnumsFunc=(f), .valueAsStringFunc=(g)}
#define META_FUNC_I32(n, c, f, g) {n, c, PTP_DT_INT32, .buildEnumsFunc=(f), .valueAsStringFunc=(g)}
#define META_FUNC_U32(n, c, f, g) {n, c, PTP_DT_UINT32, .buildEnumsFunc=(f), .valueAsStringFunc=(g)}

static PropertyMetadata sPropertyMetadata[] = {
    META_ENUM_U8 ("image-file-format", DPC_COMPRESSION_SETTING, sProp_CompressionSetting),
    META_ENUM_U8 ("image-file-format", DPC_IMAGE_FILE_FORMAT, sProp_ImageFileFormat),
    META_ENUM_U8 ("raw-file-type", DPC_RAW_FILE_TYPE, sProp_RawFileType),
    META_ENUM_U8 ("compressed-file-type", DPC_COMPRESSED_IMAGE_FILE_FORMAT, sProp_CompressedImageFileFormat),
    META_ENUM_U8 ("image-quality", DPC_IMAGE_QUALITY, sProp_ImageQuality),
    META_ENUM_U8 ("image-size", DPC_IMAGE_SIZE, sProp_ImageSize),
    META_ENUM_U16("image-save-destination", DPC_IMAGE_SAVE_DESTINATION, sProp_PcRemoteSaveDest),
    META_ENUM_U8 ("pc-save-image", DPC_PC_SAVE_IMAGE, sProp_PcSaveImage),
    META_ENUM_U8 ("image-transfer-size", DPC_IMAGE_TRANSFER_SIZE, sProp_ImageTransferSize),

    META_ENUM_U16("program-mode", DPC_EXPOSURE_PROGRAM_MODE, sProp_ExposureProgramMode16),
    META_ENUM_U32("program-mode", DPC_EXPOSURE_PROGRAM_MODE, sProp_ExposureProgramMode32),
    META_ENUM_U8 ("program-mode-key", DPC_EXPOSURE_MODE_KEY, sProp_ExposureModeKey),
    META_ENUM_U16("capture-mode", DPC_CAPTURE_MODE, sProp_CaptureMode16),
    META_ENUM_U32("capture-mode", DPC_CAPTURE_MODE, sProp_CaptureMode32),
    META_FUNC_U16("f-number", DPC_F_NUMBER, NULL, GetFNumberAsString),
    META_FUNC_U32("shutter-speed", DPC_SHUTTER_SPEED, NULL, GetShutterSpeedAsString),
    META_FUNC_U32("iso", DPC_ISO, NULL, GetIsoAsString),
    META_FUNC_U32("iso-current", DPC_ISO_CURRENT, NULL, GetIsoAsString),
    META_ENUM_U8 ("shutter-mode", DPC_SHUTTER_MODE, sProp_ShutterMode),
    META_ENUM_U8 ("shutter-type", DPC_SHUTTER_TYPE, sProp_ShutterType),
    META_ENUM_U8 ("shutter-mode-setting", DPC_SHUTTER_MODE_SETTING, sProp_ShutterModeSetting),
    META_ENUM_U8 ("silent-mode", DPC_SILENT_MODE, sProp_OnOff1),
    META_ENUM_U8 ("silent-mode-aperture-drive-af", DPC_SILENT_MODE_APERTURE_DRIVE_AF, sProp_ApertureDriveAF),
    META_ENUM_U8 ("silent-mode-power-off", DPC_SILENT_MODE_POWER_OFF, sProp_SilentModePowerOff),
    META_ENUM_U8 ("silent-mode-auto-pixel-mapping", DPC_SILENT_MODE_AUTO_PIXEL_MAPPING, sProp_SilentModeAutoPixelMapping),

    META_ENUM_U8 ("aspect-ratio", DPC_ASPECT_RATIO, sProp_AspectRatio),
    META_ENUM_U8 ("iris-mode", DPC_IRIS_MODE, sProp_IrisMode),

    META_ENUM_U16("white-balance", DPC_WHITE_BALANCE, sProp_WhiteBalance),
    META_FUNC_U16("white-balance-custom-temp", DPC_COLOR_TEMPERATURE, NULL, GetTemperatureAsString),
    META_FUNC_U8 ("white-balance-gm", DPC_WHITE_BALANCE_GM, NULL, GetWhiteBalanceGMAsString),
    META_FUNC_U8 ("white-balance-ab", DPC_WHITE_BALANCE_AB, NULL, GetWhiteBalanceABAsString),

    META_ENUM_U8 ("gain-control", DPC_GAIN_CONTROL, sProp_GainControl),
    META_ENUM_U16("exposure-metering-mode", DPC_EXPOSURE_METERING_MODE, sProp_ExposureMeteringMode),
    META_FUNC_I16("exposure-bias-compensation", DPC_EXPOSURE_COMPENSATION, NULL, GetExposureBiasAsString),
    META_ENUM_U8 ("dro-hdr-mode", DPC_DRO_HDR_MODE, sProp_DRO),

    META_ENUM_U8 ("awb-lock-satus", DPC_AWB_LOCK_STATUS, sProp_LockedUnlocked),
    META_ENUM_U8 ("fel-lock-satus", DPC_FEL_LOCK_STATUS, sProp_LockedUnlocked),
    META_ENUM_U8 ("ae-lock-satus", DPC_AE_LOCK_STATUS, sProp_LockedUnlocked),

    META_ENUM_U16("focus-mode", DPC_FOCUS_MODE, sProp_FocusMode),
    META_ENUM_U16("focus-area", DPC_FOCUS_AREA, sProp_FocusArea),
    META_ENUM_U8 ("manual-focus-adjust-enabled", DPC_MANUAL_FOCUS_ADJUST_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("af-tracking-sens", DPC_AF_TRACKING_SENS, sProp_AFTrackingSensitivity),
    META_ENUM_U8 ("auto-focus-status", DPC_AUTO_FOCUS_STATUS, sProp_AutoFocusStatus),
    META_FUNC_U16("focus-magnify-scale", DPC_FOCUS_MAGNIFY_SCALE, NULL, GetFocusMagnifyScale),
    META_FUNC_U32("focus-magnify-pos", DPC_FOCUS_MAGNIFY_POS, NULL, GetFocusMagnifyPos),
    META_FUNC_U32("focus-spot-pos", DPC_FOCUS_AREA_POS_OLD, NULL, GetFocusSpotPos),

    META_ENUM_U16("flash-mode", DPC_FLASH_MODE, sProp_FlashMode),
    META_ENUM_U8 ("wireless-flash", DPC_WIRELESS_FLASH, sProp_OnOff0),
    META_ENUM_U8 ("red-eye-reduction", DPC_RED_EYE_REDUCTION, sProp_OnOff0),
    META_FUNC_I16("flash-compensation", DPC_FLASH_COMPENSATION, NULL, GetFlashCompAsString),

    META_ENUM_U16("picture-effect", DPC_PICTURE_EFFECT, sProp_PictureEffect),
    META_ENUM_U8 ("picture-profile", DPC_PICTURE_PROFILE, sProp_PictureProfile),
    META_ENUM_U8 ("creative-style", DPC_CREATIVE_STYLE, sProp_CreativeStyle),
    META_ENUM_U16("creative-look", DPC_CREATIVE_LOOK, sProp_CreativeLook),

    META_ENUM_U8 ("movie-format", DPC_MOVIE_FILE_FORMAT, sProp_MovieFormat),
    META_ENUM_U8 ("movie-format-proxy", DPC_MOVIE_FILE_FORMAT_PROXY, sProp_MovieFormat),
    META_ENUM_U16("movie-quality", DPC_MOVIE_QUALITY, sProp_MovieQuality),
    META_ENUM_U8 ("movie-recording-state", DPC_MOVIE_REC_STATE, sProp_MovieRecState),
    META_ENUM_U8 ("movie-frame-rate", DPC_MOVIE_FRAME_RATE, sProp_MovieFrameRate),

    META_ENUM_U8 ("interval-record-mode", DPC_INTERVAL_RECORD_MODE, sProp_OnOff1),
    META_ENUM_U8 ("interval-record-status", DPC_INTERVAL_RECORD_STATUS, sProp_IntervalRecStatus),

    META_ENUM_U8 ("media-playback", DPC_PLAYBACK_MEDIA, sProp_MediaPlayback),
    META_ENUM_U8 ("media-slot1-status", DPC_MEDIA_SLOT1_STATUS, sProp_MediaSlotStatus),
    META_ENUM_U8 ("media-slot2-status", DPC_MEDIA_SLOT2_STATUS, sProp_MediaSlotStatus),
    META_ENUM_U8 ("format-media-slot1-enabled", DPC_FORMAT_MEDIA_SLOT1_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("format-media-slot2-enabled", DPC_FORMAT_MEDIA_SLOT2_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("format-media-quick-slot1-enabled", DPC_FORMAT_MEDIA_QUICK_SLOT1_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("format-media-quick-slot1-enabled", DPC_FORMAT_MEDIA_QUICK_SLOT2_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("format-media-cancel-enabled", DPC_FORMAT_MEDIA_CANCEL_ENABLED, sProp_EnabledDisabled),

    META_ENUM_U8 ("contents-transfer-enabled", DPC_CONTENTS_TRANSFER_ENABLED, sProp_EnabledDisabled),

    META_ENUM_U8 ("live-view-quality", DPC_LIVE_VIEW_QUALITY, sProp_LiveViewImageQuality),
    META_ENUM_U8 ("live-view-status", DPC_LIVE_VIEW_STATUS, sProp_LiveViewStatus),
    META_ENUM_U8 ("live-view-setting-effect", DPC_LIVE_VIEW_SETTING_EFFECT, sProp_LiveViewSettingEffect),

    META_FUNC_I8 ("battery-remaining", DPC_BATTERY_REMAINING, NULL, GetBatteryRemainingAsString),
    META_ENUM_U8 ("battery-level", DPC_BATTERY_LEVEL, sProp_BatteryLevel),
    META_ENUM_U8 ("device-overheating-state", DPC_DEVICE_OVERHEATING_STATE, sProp_DeviceOverheatingState),
    META_ENUM_U8 ("touch-operation", DPC_TOUCH_OPERATION, sProp_TouchOperation),
    META_ENUM_U8 ("touch-operation-function", DPC_TOUCH_OPERATION_FUNCTION, sProp_TouchOperationFunction),
    META_ENUM_U8 ("remote-touch-enabled", DPC_REMOTE_TOUCH_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("remote-touch-cancel-enabled", DPC_REMOTE_TOUCH_CANCEL_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("time-code-format", DPC_TIME_CODE_FORMAT, sProp_TimeCodeFormat),
    META_FUNC_U32("predicted-max-file-size", DPC_PREDICTED_MAX_FILE_SIZE, NULL, GetPredictedMaxFileSizeAsString),
    META_FUNC_U16("pending-files", DPC_PENDING_FILES, NULL, GetPendingFileInfoAsString),

    META_ENUM_U8 ("pixel-shift-shooting-mode", DPC_PIXEL_SHIFT_SHOOTING_MODE, sProp_PixelShiftShootingMode),
    META_FUNC_U16("pixel-shift-shooting-number", DPC_PIXEL_SHIFT_SHOOTING_NUMBER, NULL, GetPixelShootingNumberAsString),
    META_FUNC_U16("pixel-shift-shooting-interval", DPC_PIXEL_SHIFT_SHOOTING_INTERVAL, NULL, GetPixelShootingIntervalAsString),
    META_ENUM_U8 ("pixel-shift-shooting-status", DPC_PIXEL_SHIFT_SHOOTING_STATUS, sProp_PixelShiftShootingStatus),
    META_FUNC_U16("pixel-shift-shooting-status", DPC_PIXEL_SHIFT_SHOOTING_PROGRESS, NULL, GetPixelShootingProgressAsString),

    META_ENUM_U8 ("zoom-operation-enabled", DPC_ZOOM_OPERATION_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("zoom-setting", DPC_ZOOM_SETTING, sProp_ZoomSetting),
    META_ENUM_U8 ("zoom-type-status", DPC_ZOOM_TYPE_STATUS, sProp_ZoomSetting),
    META_FUNC_U32("zoom-scale", DPC_ZOOM_SCALE, NULL, GetZoomScale),
    META_FUNC_U32("zoom-bar-info", DPC_ZOOM_BAR_INFO, NULL, GetZoomBarInfo),

    META_ENUM_U8 ("remote-restrict-status", DPC_REMOTE_RESTRICT_STATUS, sProp_EnabledDisabled),
    META_ENUM_U8 ("lens-info-enabled", DPC_LENS_INFORMATION_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("camera-settings-save-enabled", DPC_CAMERA_SETTING_SAVE_ENABLED, sProp_EnabledDisabled),
    META_ENUM_U8 ("camera-settings-read-enabled", DPC_CAMERA_SETTING_READ_ENABLED, sProp_EnabledDisabled),
};

b32 BuildEnumsFromGetFunc(PTPControl* self, PTPProperty* property, PropertyMetadata* meta, PTPPropValueEnums* outEnums) {
    for (int i = 0; i < MArraySize(property->form.enums.getSet); i++) {
        PTPPropValueEnum *propEnum = MArrayAddPtr(outEnums->values);
        PTPPropValue propValue = property->form.enums.getSet[i];
        propEnum->propValue = propValue;
        propEnum->str = meta->valueAsStringFunc(self, property, propValue);
        if (propEnum->str.size) {
            propEnum->flags = ENUM_VALUE_READ | ENUM_VALUE_WRITE;
        } else {
            propEnum->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
        }
    }

    if (MArraySize(property->form.enums.set)) {
        size_t getSetItems = MArraySize(outEnums->values);
        for (int j = 0; j < getSetItems; j++) {
            PTPPropValueEnum* prop = outEnums->values + j;
            prop->flags &= ~ENUM_VALUE_WRITE;
        }

        for (int i = 0; i < MArraySize(property->form.enums.set); i++) {
            PTPPropValue lookupValue = property->form.enums.set[i];
            PTPPropValueEnum* prop = NULL;
            for (int j = 0; j < getSetItems; j++) {
                PTPPropValue enumValue = outEnums->values[j].propValue;
                if (PTP_PropValueEq(property->dataType, lookupValue, enumValue)) {
                    prop = outEnums->values + j;
                    break;
                }
            }

            if (!prop) {
                prop = MArrayAddPtr(outEnums->values);
                prop->propValue = lookupValue;
                prop->str = meta->valueAsStringFunc(self, property, lookupValue);
            }

            if (prop->str.size) {
                prop->flags = ENUM_VALUE_READ | ENUM_VALUE_WRITE;
            } else {
                prop->flags = ENUM_VALUE_STR_CONST | ENUM_VALUE_READ | ENUM_VALUE_WRITE;
            }
        }
    }

    return TRUE;
}

b32 PTPControl_GetEnumsForProperty(PTPControl* self, u16 propertyCode, PTPPropValueEnums* outEnums) {
    PTPProperty* property = PTPControl_GetProperty(self, propertyCode);
    if (property->formFlag != PTP_FORM_FLAG_ENUM) {
        return FALSE;
    }
    for (int i = 0; i < MStaticArraySize(sPropertyMetadata); i++) {
        PropertyMetadata* meta = sPropertyMetadata + i;
        if (propertyCode == meta->propCode && meta->type == property->dataType) {
            if (meta->fixedEnumsSize) {
                switch (meta->type) {
                    case PTP_DT_UINT8:
                        return BuildEnumsFromListU8(self, property, meta->fixedEnums.u8, meta->fixedEnumsSize, outEnums);
                    case PTP_DT_UINT16:
                        return BuildEnumsFromListU16(self, property, meta->fixedEnums.u16, meta->fixedEnumsSize, outEnums);
                    case PTP_DT_UINT32:
                        return BuildEnumsFromListU32(self, property, meta->fixedEnums.u32, meta->fixedEnumsSize, outEnums);
                }
            } else if (meta->buildEnumsFunc) {
                return meta->buildEnumsFunc(self, property, outEnums);
            } else if (meta->valueAsStringFunc) {
                return BuildEnumsFromGetFunc(self, property, meta, outEnums);
            }
        }
    }
    return FALSE;
}

b32 PTPControl_GetPropertyAsStr(PTPControl* self, u16 propertyCode, MStr* strOut) {
    PTPProperty* property = PTPControl_GetProperty(self, propertyCode);
    if (property == NULL) {
        return FALSE;
    }
    char* str = NULL;
    for (int i = 0; i < MStaticArraySize(sPropertyMetadata); i++) {
        PropertyMetadata* meta = sPropertyMetadata + i;
        if (propertyCode == meta->propCode && meta->type == property->dataType) {
            if (meta->fixedEnumsSize) {
                switch (meta->type) {
                    case PTP_DT_UINT8:
                        str = EnumValue8_Lookup(meta->fixedEnums.u8, meta->fixedEnumsSize, property->value.u8);
                        break;
                    case PTP_DT_UINT16:
                        str = EnumValue16_Lookup(meta->fixedEnums.u16, meta->fixedEnumsSize, property->value.u16);
                        break;
                    case PTP_DT_UINT32:
                        str = EnumValue32_Lookup(meta->fixedEnums.u32, meta->fixedEnumsSize, property->value.u32);
                        break;
                }
            } else if (meta->valueAsStringFunc) {
                *strOut = meta->valueAsStringFunc(self, property, property->value);
            }

            if (!strOut->str) {
                strOut->str = str;
                strOut->size = 0;
            }

            return strOut->str != NULL;
        }
    }

    return FALSE;
}

PTPResult PTPControl_SetProperty(PTPControl* self, u16 propertyCode, PTPPropValue value) {
    PTPProperty* property = PTPControl_GetProperty(self, propertyCode);
    if (!property) {
        return PTP_GENERAL_ERROR;
    }
    PTPResult r = SDIO_SetExtDevicePropValue(self, propertyCode, property->dataType, value);
    if (r == PTP_OK) {
        property->value = value; // TODO : Copy func so we dont steal string ptr
    }
    return r;
}

b32 PTPControl_SetPropertyU16(PTPControl* self, u16 propertyCode, u16 value) {
    return FALSE;
}

b32 PTPControl_SetPropertyU32(PTPControl* self, u16 propertyCode, u32 value) {
    return FALSE;
}

b32 PTPControl_SetPropertyU64(PTPControl* self, u16 propertyCode, u64 value) {
    return FALSE;
}

b32 PTPControl_SetPropertyStr(PTPControl* self, u16 propertyCode, MStr value) {
    // Set from string directly
    return FALSE;
}

b32 PTPControl_SetPropertyFancy(PTPControl* self, u16 propertyCode, MStr value) {
    // Parse string
    return FALSE;
}

b32 PTPControl_SetPropertyNotch(PTPControl* self, u16 propertyCode, i8 notch) {
    PTP_TRACE_F("PTPControl_SetPropertyNotch(%x, %d)", propertyCode, notch);
    PTPProperty* property = PTPControl_GetProperty(self, propertyCode);
    if (!property) {
        return PTP_GENERAL_ERROR;
    }
    if (!property->isNotch) {
        PTP_ERROR_F("Property %d is not a notch property", propertyCode);
        return PTP_GENERAL_ERROR;
    }
    return SDIO_ControlDevice(self, propertyCode, PTP_DT_INT8, (PTPPropValue){.i8=notch});
}

PtpControl* PTPControl_GetControl(PTPControl* self, u16 controlCode) {
    for (int i = 0; i < MArraySize(self->controls); ++i) {
        if (self->controls[i].controlCode == controlCode) {
            return self->controls + i;
        }
    }
    return NULL;
}

PTPResult PTPControl_SetControl(PTPControl* self, u16 controlCode, PTPPropValue value) {
    PtpControl* control = PTPControl_GetControl(self, controlCode);
    if (control) {
        return SDIO_ControlDevice(self, controlCode, control->dataType, value);
    } else {
        return PTP_GENERAL_ERROR;
    }
}

PTPResult PTPControl_SetControlToggle(PTPControl* self, u16 controlCode, b32 pressed) {
    PtpControl* control = PTPControl_GetControl(self, controlCode);
    if (control) {
        return SDIO_ControlDevice(self, controlCode, control->dataType, (PTPPropValue){.u16=pressed?2:1});
    } else {
        return PTP_GENERAL_ERROR;
    }
}

b32 PTPControl_GetEnumsForControl(PTPControl* self, u16 controlCode, PTPPropValueEnums* outEnums) {
    PtpControl* control = PTPControl_GetControl(self, controlCode);
    if (control->formFlag != PTP_FORM_FLAG_ENUM) {
        return FALSE;
    }
    return FALSE;
}
