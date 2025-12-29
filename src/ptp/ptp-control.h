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
PTP_EXPORT PTPResult PTPControl_Init(PTPControl* self, PTPDevice* device, MAllocator* allocator);

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
PTP_EXPORT PTPResult PTPControl_Connect(PTPControl* self, SonyProtocolVersion version);

/**
 * Cleanup the PTPControl structure.
 * Depending on the underlying transport this may also close the PTP session, if it does not then
 * releasing the transport will close the PTP session.
 * After PTPControl_Cleanup() is called, the device transport should be closed.
 *
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT PTPResult PTPControl_Cleanup(PTPControl* self);

//////////////////////////////////////////////////////////////////////////////////////////////
// Check support for various events, controls, and properties
//////////////////////////////////////////////////////////////////////////////////////////////

PTP_EXPORT b32 PTPControl_SupportsEvent(PTPControl* self, u16 eventCode);
PTP_EXPORT b32 PTPControl_SupportsControl(PTPControl* self, u16 controlCode);
PTP_EXPORT b32 PTPControl_SupportsProperty(PTPControl* self, u16 propertyCode);
PTP_EXPORT b32 PTPControl_PropertyEnabled(PTPControl* self, u16 propertyCode);


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
PTP_EXPORT PTPProperty* PTPControl_GetPropertyAtIndex(PTPControl* self, u16 index);

/**
 * Pull latest property values from device
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTP_EXPORT PTPResult PTPControl_UpdateProperties(PTPControl* self);

/**
 * Get property information
 * @param propertyCode
 * @return PTPProperty if found, null if not available
 */
PTP_EXPORT PTPProperty* PTPControl_GetProperty(PTPControl* self, u16 propertyCode);

/**
 * Build a list of enums for property if enums are available.
 * Converts values to strings for display.
 * You must free memory allocated by this function by calling PTPControl_FreePropValueEnums / PTP_FreePropValueEnums.
 * @param propertyCode
 * @param allocator allocator to use for outEnums, when set to null to uses the default PTPControl allocator
 * @param outEnums
 * @page allocator Optional allocator to use (when NULL use PTPControl allocator).
 * @return TRUE if enums are available, false if no enums available for this property
 */
PTP_EXPORT b32 PTPControl_GetEnumsForProperty(PTPControl* self, u16 propertyCode, MAllocator* allocator, PTPPropValueEnums* outEnums);

/**
 * Free output enum returned by PTPControl_GetEnumsForProperty()
 */
PTP_EXPORT void PTPControl_FreePropValueEnums(PTPControl* self, PTPPropValueEnums* outEnums);

/**
 * Free output enum returned by PTPControl_GetEnumsForProperty()
 */
PTP_EXPORT void PTP_FreePropValueEnums(MAllocator* self, PTPPropValueEnums* outEnums);

PTP_EXPORT b32 PTPControl_GetPropertyAsStr(PTPControl* self, u16 propertyCode, MAllocator* allocator, MStr* strOut);
PTP_EXPORT PTPResult PTPControl_SetProperty(PTPControl* self, u16 propertyCode, PTPPropValue value);
PTP_EXPORT b32 PTPControl_SetPropertyU16(PTPControl* self, u16 propertyCode, u16 value);
PTP_EXPORT b32 PTPControl_SetPropertyU32(PTPControl* self, u16 propertyCode, u32 value);
PTP_EXPORT b32 PTPControl_SetPropertyU64(PTPControl* self, u16 propertyCode, u64 value);
PTP_EXPORT b32 PTPControl_SetPropertyStr(PTPControl* self, u16 propertyCode, MStr value);
PTP_EXPORT b32 PTPControl_SetPropertyFancy(PTPControl* self, u16 propertyCode, MStr value);
PTP_EXPORT PTPResult PTPControl_SetPropertyNotch(PTPControl* self, u16 propertyCode, i8 notch);
PTP_EXPORT b32 PTPControl_IsPropertyWritable(PTPControl* self, u16 propertyCode);


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
PTP_EXPORT PtpControl* PTPControl_GetControlAtIndex(PTPControl* self, u16 index);

PTP_EXPORT PtpControl* PTPControl_GetControl(PTPControl* self, u16 controlCode);
PTP_EXPORT PTPResult PTPControl_SetControl(PTPControl* self, u16 controlCode, PTPPropValue value);
PTP_EXPORT PTPResult PTPControl_SetControlToggle(PTPControl* self, u16 controlCode, b32 pressed);
PTP_EXPORT b32 PTPControl_GetEnumsForControl(PTPControl* self, u16 controlCode, PTPPropValueEnums* outEnums);


//////////////////////////////////////////////////////////////////////////////////////////////
// File download / upload functions
//////////////////////////////////////////////////////////////////////////////////////////////
PTP_EXPORT int PTPControl_GetPendingFiles(PTPControl* self);
PTP_EXPORT PTPResult PTPControl_GetLiveViewImage(PTPControl* self, MMemIO* fileOut, LiveViewFrames* liveViewFramesOut);
PTP_EXPORT PTPResult PTPControl_GetCapturedImage(PTPControl* self, MMemIO* fileOut, PTPCapturedImageInfo* ciiOut);
PTP_EXPORT PTPResult PTPControl_GetCameraSettingsFile(PTPControl* self, MMemIO* fileOut);
PTP_EXPORT PTPResult PTPControl_PutCameraSettingsFile(PTPControl* self, MMemIO* fileIn);
PTP_EXPORT void PTPControl_FreeLiveViewFrames(PTPControl* self, LiveViewFrames* liveViewFrames);

//////////////////////////////////////////////////////////////////////////////////////////////
// String conversion, value helpers
//////////////////////////////////////////////////////////////////////////////////////////////
PTP_EXPORT char* PTP_GetOperationStr(u16 operationCode);
PTP_EXPORT char* PTP_GetControlStr(u16 controlCode);
PTP_EXPORT char* PTP_GetEventStr(u16 eventCode);
PTP_EXPORT char* PTP_GetPropertyStr(u16 propCode);
PTP_EXPORT char* PTP_GetObjectFormatStr(u16 objectFormatCode);

PTP_EXPORT char* PTP_GetDataTypeStr(PTPDataType dataType);
PTP_EXPORT char* PTP_GetFormFlagStr(PTPFormFlag formFlag);
PTP_EXPORT char* PTP_GetPropIsEnabledStr(u8 propIsEnabled);

PTP_EXPORT void PTP_GetPropValueStr(PTPDataType dataType, PTPPropValue value, char* buffer, size_t bufferLen);
PTP_EXPORT b32 PTP_PropValueEq(PTPDataType dataType, PTPPropValue value1, PTPPropValue value2);
PTP_EXPORT b32 PTPProperty_Equals(PTPProperty* property, PTPPropValue value);

#ifdef __cplusplus
} // extern "C"
#endif
