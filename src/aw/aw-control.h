#pragma once

#include "mlib/mlib.h"

#include "aw/aw-backend.h"
#include "aw/aw-const.h"
#include "aw/aw-log.h"

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

    AwPtpProperty* properties;
    AwPtpControl* controls;

    u32 sessionId;
    u32 transactionId;

    u8* dataInMem;
    u32 dataInSize;
    u32 dataInCapacity;
    u8* dataOutMem;
    u32 dataOutSize;
    u32 dataOutCapacity;
    AwPtpRequestHeader ptpRequest;
    AwPtpResponseHeader ptpResponse;

    AwPtpEvent* eventQueue;  // Array of queued events

    MAllocator* allocator;
    AwLog logger;
} AwControl;

//////////////////////////////////////////////////////////////////////////////////////////////
// Setup & Cleanup
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Initialize AwControl struct with the specified device.
 * Follow up with a AwControl_Connect() to start communicating over the device's transport.
 *
 * List available devices and connect to the first one:
 *
 * @code{.c}
 *    #include "aw/aw-control.h"
 *    #include "aw/aw-device-list.h"
 *    ...
 *    AwDeviceList deviceList{};
 *    // Open device list
 *    AwDeviceList_Open(&deviceList, &allocator);
 *    // Poll for devices
 *    AwDeviceList_RefreshList(&deviceList);
 *    // If one or more devices
 *    if (MArraySize(deviceList.devices) {
 *        AwDeviceInfo* deviceInfo = deviceList.devices;
 *        AwDevice* device = NULL;
 *        // Establish transport for first device
 *        AwDeviceList_OpenDevice(&deviceList, deviceInfo, &device);
 *        AwControl aw{};
 *        // Init control structure
 *        AwControl_Init(&aw, device, &allocator);
 *        // Connect to device with given mode
 *        AwControl_Connect(&aw, SDI_EXTENSION_VERSION_300);
 *    }
 * @endcode
 *
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_Init(AwControl* self, AwDevice* device, MAllocator* allocator);

/**
 * Establishes a connection with a Sony device over PTP (Picture Transfer Protocol), on the previously setup transport.
 *
 * Opens a new session to the device, performs authentication, retrieves device and property information, and prepares
 * the device for control operations.
 *
 * @param version The Sony protocol version to connect with.  Year 2020+ cameras support SDI_EXTENSION_VERSION_300,
 *                which has more properties and controls, along with other API improvements over the
 *                SDI_EXTENSION_VERSION_200 protocol.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_Connect(AwControl* self, AwSonyProtocolVersion version);

/**
 * Cleanup the AwControl structure.
 * Depending on the underlying transport this may also close the PTP session, if it does not then
 * releasing the transport will close the PTP session.
 * After AwControl_Cleanup() is called, the device transport should be closed.
 *
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_Cleanup(AwControl* self);

//////////////////////////////////////////////////////////////////////////////////////////////
// Check support for various events, controls, and properties
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Check if the device supports a specific PTP event.
 * @param eventCode The PTP event code to check.
 * @return TRUE if the event is supported, FALSE otherwise.
 */
AW_EXPORT b32 AwControl_SupportsEvent(AwControl* self, u16 eventCode);

/**
 * Check if the device supports a specific control operation.
 * @param controlCode The control code to check.
 * @return TRUE if the control is supported, FALSE otherwise.
 */
AW_EXPORT b32 AwControl_SupportsControl(AwControl* self, u16 controlCode);

/**
 * Check if the device supports a specific property.
 * @param propCode The property code to check.
 * @return TRUE if the property is supported, FALSE otherwise.
 */
AW_EXPORT b32 AwControl_SupportsProperty(AwControl* self, u16 propCode);

/**
 * Check if a feature state property indicates that a feature is enabled/disabled.
 * @param property Feature enable prop such as DPC_REMOTE_TOUCH_ENABLE, DPC_CAMERA_SETTING_SAVE_ENABLED,
 *                 DPC_MANUAL_FOCUS_ADJUST_ENABLED
 * @return TRUE if the feature is available and currently enabled.
 */
AW_EXPORT b32 AwControl_PropertyEnabled(AwControl* self, AwPtpProperty* property);

/**
 * Check if a feature state property indicates that a feature is enabled/disabled.
 * @param propCode Feature enable prop such as DPC_REMOTE_TOUCH_ENABLE, DPC_CAMERA_SETTING_SAVE_ENABLED,
 *                 DPC_MANUAL_FOCUS_ADJUST_ENABLED
 * @return TRUE if the feature is available and currently enabled.
 */
AW_EXPORT b32 AwControl_PropertyEnabledByCode(AwControl* self, u16 propCode);

//////////////////////////////////////////////////////////////////////////////////////////////
// Generic property getter and setters
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Number of properties found.
 * @return number of properties found
 */
AW_EXPORT size_t AwControl_NumProperties(AwControl* self);

/**
 * Get Property at index (no particular order)
 * Use with AwControl_NumProperties() to list all properties.
 * @param index
 * @return property at index
 */
AW_EXPORT AwPtpProperty* AwControl_GetPropertyByIndex(AwControl* self, u16 index);

/**
 * Pull latest property values from device
 * @param fullRefresh When set to TRUE: Refresh all properties, FALSE: Refresh only changed properties
 *                    NOTE: Most property changes are correctly tracked by the camera, but AwControl_GetPendingFiles()
 *                    may not update when 'fullRefresh' is set to FALSE.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_UpdateProperties(AwControl* self, b32 fullRefresh);

/**
 * Get property by property code
 * @param propertyCode
 * @return PTPProperty if found, null if not available
 */
AW_EXPORT AwPtpProperty* AwControl_GetPropertyByCode(AwControl* self, u16 propertyCode);

/**
 * Get property by id
 * @param id
 * @return PTPProperty if found, null if not available
 */
AW_EXPORT AwPtpProperty* AwControl_GetPropertyById(AwControl* self, const char* id);

/**
 * Build a list of enums for property if enums are available.
 * Converts values to strings for display.
 * You must free memory allocated by this function by calling AwControl_FreePropValueEnums / PTP_FreePropValueEnums.
 * @param property
 * @param allocator allocator to use for outEnums, when set to null to uses the default AwControl allocator
 * @param outEnums
 * @page allocator Optional allocator to use (when NULL use AwControl allocator).
 * @return TRUE if enums are available, false if no enums available for this property
 */
AW_EXPORT b32 AwControl_GetEnumsForProperty(AwControl* self, AwPtpProperty* property, MAllocator* allocator, AwPtpPropValueEnums* outEnums);

/**
 * Free output enum returned by AwControl_GetEnumsForProperty()
 */
AW_EXPORT void AwControl_FreePropValueEnums(AwControl* self, AwPtpPropValueEnums* outEnums);

/**
 * Free output enum returned by AwControl_GetEnumsForProperty()
 */
AW_EXPORT void AwPtp_FreePropValueEnums(MAllocator* self, AwPtpPropValueEnums* outEnums);

/**
 * Get the property value as a human-readable string.
 * @param property The property to convert.
 * @param allocator Allocator to use for the output string.
 * @param outStr Output string containing the property value.
 * @return TRUE on success, FALSE on failure.
 */
AW_EXPORT b32 AwControl_GetPropertyValueAsStr(AwControl* self, AwPtpProperty* property, MAllocator* allocator, MStr* outStr);

/**
 *
 * @param property
 * @param outValue
 * @return AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_GetPropertyValueAsBool(AwControl* self, AwPtpProperty* property, b32* outValue);

/**
 * Check if a property can be written/modified.
 * @param property The property to check.
 * @return TRUE if the property is writable, FALSE otherwise.
 */
AW_EXPORT b32 AwControl_IsPropertyWritable(AwControl* self, AwPtpProperty* property);

/**
 * Check if a property uses notch-style adjustment (for older Sony 2.0 interface).
 * @param property The property to check.
 * @return TRUE if the property uses notch adjustment, FALSE otherwise.
 */
AW_EXPORT b32 AwControl_IsPropertyNotch(AwControl* self, AwPtpProperty* property);

/**
 * Get the string identifier for a property.
 * @param property The property to query.
 * @param strOut Output string containing the property ID.
 * @return TRUE on success, FALSE on failure.
 */
AW_EXPORT b32 AwControl_GetPropertyId(AwControl* self, AwPtpProperty* property, MStr* strOut);

/**
 * Set a property value on the device.
 * @param property The property to modify.
 * @param value The new value to set.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_SetPropertyValue(AwControl* self, AwPtpProperty* property, AwPtpPropValue value);

/**
 * Set a property value using a string representation.
 * @param property The property to modify.
 * @param value The string value to set.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_SetPropertyStr(AwControl* self, AwPtpProperty* property, MStr value);

/**
 * Set a property that has a bool representation
 * @param property
 * @param value TRUE or FALSE
 * @return AW_RESULT_OK if it was set, AW_RESULT_UNSUPPORTED if the property does support a bool representation
 */
AW_EXPORT AwResult AwControl_SetPropertyBool(AwControl* self, AwPtpProperty* property, b32 value);

/**
 * Adjust 'notch' properties that are used for some shot settings when using older Sony 2.0 PTP interface.
 * This is the only interface supported by pre-2020 cameras.
 * @param property
 * @param notch 1: notch one setting up, -1: notch one setting down, 2: notch two setting up twice,
 *              -2: notch two setting down twice
 * @return
 */
AW_EXPORT AwResult AwControl_SetPropertyNotch(AwControl* self, AwPtpProperty* property, i8 notch);

MINLINE AwResult AwControl_SetPropertyU8(AwControl* self, AwPtpProperty* property, u8 value) {
    return AwControl_SetPropertyValue(self, property, M_STRUCT(AwPtpPropValue){.u8 = value});
}

MINLINE AwResult AwControl_SetPropertyU16(AwControl* self, AwPtpProperty* property, u16 value) {
    return AwControl_SetPropertyValue(self, property, M_STRUCT(AwPtpPropValue){.u16 = value});
}

MINLINE AwResult AwControl_SetPropertyU32(AwControl* self, AwPtpProperty* property, u32 value) {
    return AwControl_SetPropertyValue(self, property, M_STRUCT(AwPtpPropValue){.u32 = value});
}

MINLINE AwResult AwControl_SetPropertyU64(AwControl* self, AwPtpProperty* property, u64 value) {
    return AwControl_SetPropertyValue(self, property, M_STRUCT(AwPtpPropValue){.u64 = value});
}

MINLINE AwResult AwControl_SetPropertyStrRaw(AwControl* self, AwPtpProperty* property, MStr value) {
    return AwControl_SetPropertyValue(self, property, M_STRUCT(AwPtpPropValue){.str = value});
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Generic controls getters and setters
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Number of controls found.
 * @param self Pointer to the AwControl instance to be checked.
 * @return number of controls found
 */
AW_EXPORT size_t AwControl_NumControls(AwControl* self);

/**
 * Get Control at index (no particular order)
 * Use with AwControl_NumControls() to list all controls.
 * @param index
 * @return property at index
 */
AW_EXPORT AwPtpControl* AwControl_GetControlByIndex(AwControl* self, u16 index);

/**
 * Get a control by its control code.
 * @param controlCode The control code to look up.
 * @return Pointer to the control if found, NULL otherwise.
 */
AW_EXPORT AwPtpControl* AwControl_GetControlByCode(AwControl* self, u16 controlCode);

/**
 * Set a control value on the device.
 * @param controlCode The control code to modify.
 * @param value The new value to set.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_SetControlValue(AwControl* self, u16 controlCode, AwPtpPropValue value);

/**
 * Toggle a control on or off.
 * @param controlCode The control code to toggle.
 * @param pressed TRUE to press/enable, FALSE to release/disable.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_SetControlToggle(AwControl* self, u16 controlCode, b32 pressed);

/**
 * Get available enum values for a control.
 * @param controlCode The control code to query.
 * @param outEnums Output structure containing the available enum values.
 * @return TRUE if enums are available, FALSE otherwise.
 */
AW_EXPORT b32 AwControl_GetEnumsForControl(AwControl* self, u16 controlCode, AwPtpPropValueEnums* outEnums);


//////////////////////////////////////////////////////////////////////////////////////////////
// File download / upload
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Get the number of image files queued for download on camera.
 *
 * Call AwControl_UpdateProperties(TRUE) before calling this function to ensure local properties are uptodate.
 * 'TRUE' is needed as a partial/incremental refresh of properties may not include the updated count for pending files
 * for some cameras.
 */
AW_EXPORT int AwControl_GetPendingFiles(AwControl* self);

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
AW_EXPORT AwResult AwControl_GetCapturedImage(AwControl* self, MMemIO* outFile, AwPtpCapturedImageInfo* outCii);


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
AW_EXPORT AwResult AwControl_GetLiveViewImage(AwControl* self, MMemIO* outFile, AwLiveViewFrames* outLiveViewFrames);

/**
 * Free live view frames returned by AwControl_GetLiveViewImage()
 * @param liveViewFrames the live view frames to free
 */
AW_EXPORT void AwControl_FreeLiveViewFrames(AwControl* self, AwLiveViewFrames* liveViewFrames);

/**
 * Get the on-screen display as PNG (if available).  2025+ cameras.
 * @param outFile The PNG image
 * @return
 */
AW_EXPORT AwResult AwControl_GetOSDImage(AwControl* self, MMemIO* outFile);


//////////////////////////////////////////////////////////////////////////////////////////////
// Camera Settings File Get / Put
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Download camera settings file from the device.
 * @param outFile MemIO to write the settings file to.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_GetCameraSettingsFile(AwControl* self, MMemIO* outFile);

/**
 * Upload camera settings file to the device.
 * @param file MemIO containing the settings file data.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_PutCameraSettingsFile(AwControl* self, MMemIO* file);


//////////////////////////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Read pending PTP events from the device.
 * @param timeoutMilliseconds Maximum time to wait for events in milliseconds.
 * @param alloc Allocator to use for the event array.
 * @param outEvents Output pointer to array of events (caller must free).
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_ReadEvents(AwControl* self, int timeoutMilliseconds, MAllocator* alloc, AwPtpEvent** outEvents);


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
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_GetMagnifier(AwControl* self, AwMagnifier* outMagnifier);

/**
 * Set magnifier position and zoom level.
 * @param magnifier Magnifier parameters to set.
 * @return Returns AW_RESULT_OK on success, or an appropriate error code on failure.
 */
AW_EXPORT AwResult AwControl_SetMagnifier(AwControl* self, AwMagnifierSet magnifier);

/**
 * Calculate new viewport position when moving magnifier by a relative amount.
 * @param magnifier Current magnifier state.
 * @param pos Relative movement position (normalized coordinates).
 * @return New integer viewport position.
 */
AW_EXPORT AwPosInt2 AwMagnifierMoveViewport(AwMagnifier* magnifier, AwPosFloat2 pos);

//////////////////////////////////////////////////////////////////////////////////////////////
// Remote button press
//////////////////////////////////////////////////////////////////////////////////////////////

AW_EXPORT b32 AwControl_RemoteButtonEnable(AwControl* self);
AW_EXPORT AwResult AwControl_RemoteButtonPress(AwControl* self, u16 button, b32 pressed);

//////////////////////////////////////////////////////////////////////////////////////////////
// String conversion, value helpers
//////////////////////////////////////////////////////////////////////////////////////////////
AW_EXPORT char* AwGetOperationLabel(u16 operationCode);
AW_EXPORT char* AwGetControlLabel(u16 controlCode);
AW_EXPORT char* AwGetEventLabel(u16 eventCode);
AW_EXPORT char* AwGetPropertyLabel(u16 propCode);
AW_EXPORT char* AwGetObjectFormatStr(u16 objectFormatCode);

AW_EXPORT char* AwPtpGetDataTypeStr(PtpDataType dataType);
AW_EXPORT char* AwPtpGetFormFlagStr(PtpFormFlag formFlag);
AW_EXPORT char* AwPtpGetPropIsEnabledStr(u8 propIsEnabled);

AW_EXPORT void AwPtpGetPropValueStr(PtpDataType dataType, AwPtpPropValue value, char* outBuffer, size_t bufferLen);
AW_EXPORT b32 AwPtpPropValueEq(PtpDataType dataType, AwPtpPropValue value1, AwPtpPropValue value2);
AW_EXPORT b32 AwPtpPropEquals(AwPtpProperty* property, AwPtpPropValue value);

#ifdef __cplusplus
} // extern "C"
#endif
