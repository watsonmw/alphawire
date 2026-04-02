#pragma once

#include "mlib/mlib.h"

#include "ptp/ptp-backend.h"
#include "ptp/ptp-const.h"
#include "ptp/ptp-log.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Struct to manage and control a Sony PTP (Picture Transfer Protocol) session.
 *
 * This structure is designed to encapsulate the state and resources needed to interact
 * with devices supporting PTP. It includes information such as the device transport,
 * session and transaction identifiers, vendor-specific extensions, supported operations,
 * and allocated memory buffers for data exchange.
 *
 * Key responsibilities:
 * - Managing connection and communication with the device through the associated transport.
 * - Handling protocol-level information such as supported operations and properties.
 * - Managing data buffers for incoming and outgoing PTP operations.
 */
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

    PTPProperty* properties;
    PtpControl* controls;

    u32 sessionId;
    u32 transactionId;

    u8* dataInMem;
    u32 dataInSize;
    u32 dataInCapacity;
    u8* dataOutMem;
    u32 dataOutSize;
    u32 dataOutCapacity;
    PTPRequestHeader ptpRequest;
    PTPResponseHeader ptpResponse;

    PTPEvent* eventQueue;  // Array of queued events

    MAllocator* allocator;
    PTPLog logger;
} PTPControl;

//////////////////////////////////////////////////////////////////////////////////////////////
// Setup & Cleanup
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Initialize PTPControl struct with the specified device.
 * Follow up with a PTPControl_Connect() to start communicating over the device's transport.
 *
 * List available devices and connect to the first one:
 *
 * @code{.c}
 *    #include "ptp/ptp-control.h"
 *    #include "ptp/ptp-device-list.h"
 *    ...
 *    PTPDeviceList ptpDeviceList{};
 *    // Open device list
 *    PTPDeviceList_Open(&ptpDeviceList, &allocator);
 *    // Poll for devices
 *    PTPDeviceList_RefreshList(&ptpDeviceList);
 *    // If one or more devices
 *    if (MArraySize(ptpDeviceList.devices) {
 *        PTPDeviceInfo* deviceInfo = ptpDeviceList.devices;
 *        PTPDevice* device = NULL;
 *        // Establish transport for first device
 *        PTPDeviceList_OpenDevice(&ptpDeviceList, deviceInfo, &device);
 *        PTPControl ptp{};
 *        // Init control structure
 *        PTPControl_Init(&ptp, device, &allocator);
 *        // Connect to device with given mode
 *        PTPControl_Connect(&ptp, SDI_EXTENSION_VERSION_300);
 *    }
 * @endcode
 *
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_Init(PTPControl* self, PTPDevice* device, MAllocator* allocator);

/**
 * Establishes a connection with a Sony device over PTP (Picture Transfer Protocol), on the previously setup transport.
 *
 * Opens a new session to the device, performs authentication, retrieves device and property information, and prepares
 * the device for control operations.
 *
 * @param version The Sony protocol version to connect with.  Year 2020+ cameras support SDI_EXTENSION_VERSION_300,
 *                which has more properties and controls, along with other API improvements over the
 *                SDI_EXTENSION_VERSION_200 protocol.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_Connect(PTPControl* self, SonyProtocolVersion version);

/**
 * Cleanup the PTPControl structure.
 * Depending on the underlying transport this may also close the PTP session, if it does not then
 * releasing the transport will close the PTP session.
 * After PTPControl_Cleanup() is called, the device transport should be closed.
 *
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_Cleanup(PTPControl* self);

//////////////////////////////////////////////////////////////////////////////////////////////
// Check support for various events, controls, and properties
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Check if the device supports a specific PTP event.
 * @param eventCode The PTP event code to check.
 * @return TRUE if the event is supported, FALSE otherwise.
 */
PTP_EXPORT b32 PTPControl_SupportsEvent(PTPControl* self, u16 eventCode);

/**
 * Check if the device supports a specific control operation.
 * @param controlCode The control code to check.
 * @return TRUE if the control is supported, FALSE otherwise.
 */
PTP_EXPORT b32 PTPControl_SupportsControl(PTPControl* self, u16 controlCode);

/**
 * Check if the device supports a specific property.
 * @param propCode The property code to check.
 * @return TRUE if the property is supported, FALSE otherwise.
 */
PTP_EXPORT b32 PTPControl_SupportsProperty(PTPControl* self, u16 propCode);

/**
 * Check if a feature state property indicates that a feature is enabled/disabled.
 * @param property Feature enable prop such as DPC_REMOTE_TOUCH_ENABLE, DPC_CAMERA_SETTING_SAVE_ENABLED,
 *                 DPC_MANUAL_FOCUS_ADJUST_ENABLED
 * @return TRUE if the feature is available and currently enabled.
 */
PTP_EXPORT b32 PTPControl_PropertyEnabled(PTPControl* self, PTPProperty* property);

/**
 * Check if a feature state property indicates that a feature is enabled/disabled.
 * @param propCode Feature enable prop such as DPC_REMOTE_TOUCH_ENABLE, DPC_CAMERA_SETTING_SAVE_ENABLED,
 *                 DPC_MANUAL_FOCUS_ADJUST_ENABLED
 * @return TRUE if the feature is available and currently enabled.
 */
PTP_EXPORT b32 PTPControl_PropertyEnabledByCode(PTPControl* self, u16 propCode);

//////////////////////////////////////////////////////////////////////////////////////////////
// Generic property getter and setters
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Number of properties found.
 * @return number of properties found
 */
PTP_EXPORT size_t PTPControl_NumProperties(PTPControl* self);

/**
 * Get Property at index (no particular order)
 * Use with PTPControl_NumProperties() to list all properties.
 * @param index
 * @return property at index
 */
PTP_EXPORT PTPProperty* PTPControl_GetPropertyByIndex(PTPControl* self, u16 index);

/**
 * Pull latest property values from device
 * @param fullRefresh When set to TRUE: Refresh all properties, FALSE: Refresh only changed properties
 *                    NOTE: Most property changes are correctly tracked by the camera, but PTPControl_GetPendingFiles()
 *                    may not update when 'fullRefresh' is set to FALSE.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_UpdateProperties(PTPControl* self, b32 fullRefresh);

/**
 * Get property by property code
 * @param propertyCode
 * @return PTPProperty if found, null if not available
 */
PTP_EXPORT PTPProperty* PTPControl_GetPropertyByCode(PTPControl* self, u16 propertyCode);

/**
 * Get property by id
 * @param id
 * @return PTPProperty if found, null if not available
 */
PTP_EXPORT PTPProperty* PTPControl_GetPropertyById(PTPControl* self, const char* id);

/**
 * Build a list of enums for property if enums are available.
 * Converts values to strings for display.
 * You must free memory allocated by this function by calling PTPControl_FreePropValueEnums / PTP_FreePropValueEnums.
 * @param property
 * @param allocator allocator to use for outEnums, when set to null to uses the default PTPControl allocator
 * @param outEnums
 * @page allocator Optional allocator to use (when NULL use PTPControl allocator).
 * @return TRUE if enums are available, false if no enums available for this property
 */
PTP_EXPORT b32 PTPControl_GetEnumsForProperty(PTPControl* self, PTPProperty* property, MAllocator* allocator, PTPPropValueEnums* outEnums);

/**
 * Free output enum returned by PTPControl_GetEnumsForProperty()
 */
PTP_EXPORT void PTPControl_FreePropValueEnums(PTPControl* self, PTPPropValueEnums* outEnums);

/**
 * Free output enum returned by PTPControl_GetEnumsForProperty()
 */
PTP_EXPORT void PTP_FreePropValueEnums(MAllocator* self, PTPPropValueEnums* outEnums);

/**
 * Get the property value as a human-readable string.
 * @param property The property to convert.
 * @param allocator Allocator to use for the output string.
 * @param outStr Output string containing the property value.
 * @return TRUE on success, FALSE on failure.
 */
PTP_EXPORT b32 PTPControl_GetPropertyValueAsStr(PTPControl* self, PTPProperty* property, MAllocator* allocator, MStr* outStr);

/**
 * Check if a property can be written/modified.
 * @param property The property to check.
 * @return TRUE if the property is writable, FALSE otherwise.
 */
PTP_EXPORT b32 PTPControl_IsPropertyWritable(PTPControl* self, PTPProperty* property);

/**
 * Check if a property uses notch-style adjustment (for older Sony 2.0 interface).
 * @param property The property to check.
 * @return TRUE if the property uses notch adjustment, FALSE otherwise.
 */
PTP_EXPORT b32 PTPControl_IsPropertyNotch(PTPControl* self, PTPProperty* property);

/**
 * Get the string identifier for a property.
 * @param property The property to query.
 * @param strOut Output string containing the property ID.
 * @return TRUE on success, FALSE on failure.
 */
PTP_EXPORT b32 PTPControl_GetPropertyId(PTPControl* self, PTPProperty* property, MStr* strOut);

/**
 * Set a property value on the device.
 * @param property The property to modify.
 * @param value The new value to set.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_SetPropertyValue(PTPControl* self, PTPProperty* property, PTPPropValue value);

/**
 * Set a property value using a string representation.
 * @param property The property to modify.
 * @param value The string value to set.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_SetPropertyStr(PTPControl* self, PTPProperty* property, MStr value);

/**
 * Adjust 'notch' properties that are used for some shot settings when using older Sony 2.0 PTP interface.
 * This is the only interface supported by pre-2020 cameras.
 * @param property
 * @param notch 1: notch one setting up, -1: notch one setting down, 2: notch two setting up twice,
 *              -2: notch two setting down twice
 * @return
 */
PTP_EXPORT AwResult PTPControl_SetPropertyNotch(PTPControl* self, PTPProperty* property, i8 notch);

MINLINE AwResult PTPControl_SetPropertyU8(PTPControl* self, PTPProperty* property, u8 value) {
    return PTPControl_SetPropertyValue(self, property, M_STRUCT(PTPPropValue){.u8 = value});
}

MINLINE AwResult PTPControl_SetPropertyU16(PTPControl* self, PTPProperty* property, u16 value) {
    return PTPControl_SetPropertyValue(self, property, M_STRUCT(PTPPropValue){.u16 = value});
}

MINLINE AwResult PTPControl_SetPropertyU32(PTPControl* self, PTPProperty* property, u32 value) {
    return PTPControl_SetPropertyValue(self, property, M_STRUCT(PTPPropValue){.u32 = value});
}

MINLINE AwResult PTPControl_SetPropertyU64(PTPControl* self, PTPProperty* property, u64 value) {
    return PTPControl_SetPropertyValue(self, property, M_STRUCT(PTPPropValue){.u64 = value});
}

MINLINE AwResult PTPControl_SetPropertyStrRaw(PTPControl* self, PTPProperty* property, MStr value) {
    return PTPControl_SetPropertyValue(self, property, M_STRUCT(PTPPropValue){.str = value});
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Generic controls getters and setters
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Number of controls found.
 * @param self Pointer to the PTPControl instance to be checked.
 * @return number of controls found
 */
PTP_EXPORT size_t PTPControl_NumControls(PTPControl* self);

/**
 * Get Control at index (no particular order)
 * Use with PTPControl_NumControls() to list all controls.
 * @param index
 * @return property at index
 */
PTP_EXPORT PtpControl* PTPControl_GetControlByIndex(PTPControl* self, u16 index);

/**
 * Get a control by its control code.
 * @param controlCode The control code to look up.
 * @return Pointer to the control if found, NULL otherwise.
 */
PTP_EXPORT PtpControl* PTPControl_GetControlByCode(PTPControl* self, u16 controlCode);

/**
 * Set a control value on the device.
 * @param controlCode The control code to modify.
 * @param value The new value to set.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_SetControlValue(PTPControl* self, u16 controlCode, PTPPropValue value);

/**
 * Toggle a control on or off.
 * @param controlCode The control code to toggle.
 * @param pressed TRUE to press/enable, FALSE to release/disable.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_SetControlToggle(PTPControl* self, u16 controlCode, b32 pressed);

/**
 * Get available enum values for a control.
 * @param controlCode The control code to query.
 * @param outEnums Output structure containing the available enum values.
 * @return TRUE if enums are available, FALSE otherwise.
 */
PTP_EXPORT b32 PTPControl_GetEnumsForControl(PTPControl* self, u16 controlCode, PTPPropValueEnums* outEnums);


//////////////////////////////////////////////////////////////////////////////////////////////
// File download / upload
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Get the number of image files queued for download on camera.
 *
 * Call PTPControl_UpdateProperties(TRUE) before calling this function to ensure local properties are uptodate.
 * 'TRUE' is needed as a partial/incremental refresh of properties may not include the updated count for pending files
 * for some cameras.
 */
PTP_EXPORT int PTPControl_GetPendingFiles(PTPControl* self);

/**
 * Downloads an image from the cameras buffer.
 *
 * NOTE: downloading an image while there are no images available can cause issues on older pre-2020 camera, including
 * camera crashes and the pending file count to go negative.
 *
 * @param outFile The image contents after it has been downloaded
 * @param outCii Info about the downloaded image
 * @return
 */
PTP_EXPORT AwResult PTPControl_GetCapturedImage(PTPControl* self, MMemIO* outFile, PTPCapturedImageInfo* outCii);


//////////////////////////////////////////////////////////////////////////////////////////////
// Live View
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Get the live view image from the camera (if available)
 *
 * May have to retry if calling straight away after connecting to the camera.
 *
 * @param outFile MemIO to write the live view image to, required must not be NULL.
 * @param outLiveViewFrames NULL or output pointer to the focus frames (if available for your camera model and mode)
 * @return
 */
PTP_EXPORT AwResult PTPControl_GetLiveViewImage(PTPControl* self, MMemIO* outFile, LiveViewFrames* outLiveViewFrames);

/**
 * Free live view frames returned by PTPControl_GetLiveViewImage()
 * @param liveViewFrames the live view frames to free
 */
PTP_EXPORT void PTPControl_FreeLiveViewFrames(PTPControl* self, LiveViewFrames* liveViewFrames);

/**
 * Get the on-screen display as PNG (if available).  2025+ cameras.
 * @param outFile The PNG image
 * @return
 */
PTP_EXPORT AwResult PTPControl_GetOSDImage(PTPControl* self, MMemIO* outFile);


//////////////////////////////////////////////////////////////////////////////////////////////
// Camera Settings File Get / Put
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Download camera settings file from the device.
 * @param outFile MemIO to write the settings file to.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_GetCameraSettingsFile(PTPControl* self, MMemIO* outFile);

/**
 * Upload camera settings file to the device.
 * @param file MemIO containing the settings file data.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_PutCameraSettingsFile(PTPControl* self, MMemIO* file);


//////////////////////////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Read pending PTP events from the device.
 * @param timeoutMilliseconds Maximum time to wait for events in milliseconds.
 * @param alloc Allocator to use for the event array.
 * @param outEvents Output pointer to array of events (caller must free).
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_ReadEvents(PTPControl* self, int timeoutMilliseconds, MAllocator* alloc, PTPEvent** outEvents);


//////////////////////////////////////////////////////////////////////////////////////////////
// Magnifier
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Magnifier zoom ratio information.
 */
typedef struct AwMagnifierRatio {
    float ratio;     // Direct zoom ratio multiplier (e.g., 1.5)
    i32 ratioByTen;  // Ratio multiplied by 10 (e.g., 15 for 1.5x)
} AwMagnifierRatio;

/**
 * Live view magnifier state and capabilities.
 */
typedef struct AwMagnifier {
    i32 x;                        // X position of magnifier viewport
    i32 y;                        // Y position of magnifier viewport
    i32 width;                    // Width of magnifier viewport
    i32 height;                   // Height of magnifier viewport
    AwMagnifierRatio ratio;       // Current zoom ratio
    b32 canSet;                   // Whether magnifier can be controlled
    size_t numRatios;             // Number of available zoom ratios
    i32 ratioIndex;               // Index of current ratio in ratios array
    AwMagnifierRatio ratios[8];   // Available zoom ratios
} AwMagnifier;

/**
 * Parameters for setting magnifier position and zoom.
 */
typedef struct AwMagnifierSet {
    i32 x;
    i32 y;
    AwMagnifierRatio ratio;
} AwMagnifierSet;

typedef struct AwPosFloat2 {
    float x;
    float y;
} AwPosFloat2;

typedef struct AwPosInt2 {
    i32 x;
    i32 y;
} AwPosInt2;

/**
 * Get current magnifier state from the device.
 * @param outMagnifier Output structure to receive magnifier state.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_GetMagnifier(PTPControl* self, AwMagnifier* outMagnifier);

/**
 * Set magnifier position and zoom level.
 * @param magnifier Magnifier parameters to set.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT AwResult PTPControl_SetMagnifier(PTPControl* self, AwMagnifierSet magnifier);

/**
 * Calculate new viewport position when moving magnifier by a relative amount.
 * @param magnifier Current magnifier state.
 * @param pos Relative movement position (normalized coordinates).
 * @return New integer viewport position.
 */
PTP_EXPORT AwPosInt2 AwMagnifierMoveViewport(AwMagnifier* magnifier, AwPosFloat2 pos);

//////////////////////////////////////////////////////////////////////////////////////////////
// Remote button press
//////////////////////////////////////////////////////////////////////////////////////////////

PTP_EXPORT b32 PTPControl_RemoteButtonEnable(PTPControl* self);
PTP_EXPORT AwResult PTPControl_RemoteButtonPress(PTPControl* self, u16 button, b32 pressed);

//////////////////////////////////////////////////////////////////////////////////////////////
// String conversion, value helpers
//////////////////////////////////////////////////////////////////////////////////////////////
PTP_EXPORT char* PTP_GetOperationLabel(u16 operationCode);
PTP_EXPORT char* PTP_GetControlLabel(u16 controlCode);
PTP_EXPORT char* PTP_GetEventLabel(u16 eventCode);
PTP_EXPORT char* PTP_GetPropertyLabel(u16 propCode);
PTP_EXPORT char* PTP_GetObjectFormatStr(u16 objectFormatCode);

PTP_EXPORT char* PTP_GetDataTypeStr(PTPDataType dataType);
PTP_EXPORT char* PTP_GetFormFlagStr(PTPFormFlag formFlag);
PTP_EXPORT char* PTP_GetPropIsEnabledStr(u8 propIsEnabled);

PTP_EXPORT void PTP_GetPropValueStr(PTPDataType dataType, PTPPropValue value, char* outBuffer, size_t bufferLen);
PTP_EXPORT b32 PTP_PropValueEq(PTPDataType dataType, PTPPropValue value1, PTPPropValue value2);
PTP_EXPORT b32 PTPProperty_Equals(PTPProperty* property, PTPPropValue value);

#ifdef __cplusplus
} // extern "C"
#endif
