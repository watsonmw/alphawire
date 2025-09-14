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
PTPResult PTP_EXPORT PTPControl_Init(PTPControl* self, PTPDevice* device, MAllocator* allocator);

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
PTPResult PTP_EXPORT PTPControl_Connect(PTPControl* self, SonyProtocolVersion version);

/**
 * Cleanup the PTPControl structure.
 * Depending on the underlying transport this may also close the PTP session, if it does not then
 * releasing the transport will close the PTP session.
 * After PTPControl_Cleanup() is called, the device transport should be closed.
 *
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTPResult PTP_EXPORT PTPControl_Cleanup(PTPControl* self);

//////////////////////////////////////////////////////////////////////////////////////////////
// Check support for various events, controls, and properties
//////////////////////////////////////////////////////////////////////////////////////////////

b32 PTP_EXPORT PTPControl_SupportsEvent(PTPControl* self, u16 eventCode);
b32 PTP_EXPORT PTPControl_SupportsControl(PTPControl* self, u16 controlCode);
b32 PTP_EXPORT PTPControl_SupportsProperty(PTPControl* self, u16 propertyCode);
b32 PTP_EXPORT PTPControl_PropertyEnabled(PTPControl* self, u16 propertyCode);


//////////////////////////////////////////////////////////////////////////////////////////////
// Generic property getter and setters
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Number of properties found.
 * @param self Pointer to the PTPControl instance to be checked.
 * @return number of properties found
 */
size_t PTP_EXPORT PTPControl_NumProperties(PTPControl* self);

/**
 * Get Property at index (no particular order)
 * Use with PTPControl_NumProperties() to list all properties.
 * @param self
 * @param index
 * @return property at index
 */
PTPProperty* PTP_EXPORT PTPControl_GetPropertyAtIndex(PTPControl* self, u16 index);

/**
 * Pull latest property values from device
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTPResult PTP_EXPORT PTPControl_UpdateProperties(PTPControl* self);

PTPProperty* PTP_EXPORT PTPControl_GetProperty(PTPControl* self, u16 propertyCode);
b32 PTP_EXPORT PTPControl_GetEnumsForProperty(PTPControl* self, u16 propertyCode, PTPPropValueEnums* outEnums);
b32 PTP_EXPORT PTPControl_GetPropertyAsStr(PTPControl* self, u16 propertyCode, MStr* strOut);
PTPResult PTP_EXPORT PTPControl_SetProperty(PTPControl* self, u16 propertyCode, PTPPropValue value);
b32 PTP_EXPORT PTPControl_SetPropertyU16(PTPControl* self, u16 propertyCode, u16 value);
b32 PTP_EXPORT PTPControl_SetPropertyU32(PTPControl* self, u16 propertyCode, u32 value);
b32 PTP_EXPORT PTPControl_SetPropertyU64(PTPControl* self, u16 propertyCode, u64 value);
b32 PTP_EXPORT PTPControl_SetPropertyStr(PTPControl* self, u16 propertyCode, MStr value);
b32 PTP_EXPORT PTPControl_SetPropertyFancy(PTPControl* self, u16 propertyCode, MStr value);
b32 PTP_EXPORT PTPControl_SetPropertyNotch(PTPControl* self, u16 propertyCode, i8 notch);


//////////////////////////////////////////////////////////////////////////////////////////////
// Generic controls getters and setters
//////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Number of controls found.
 * @param self Pointer to the PTPControl instance to be checked.
 * @return number of controls found
 */
size_t PTP_EXPORT PTPControl_NumControls(PTPControl* self);

/**
 * Get Control at index (no particular order)
 * Use with PTPControl_NumControls() to list all controls.
 * @param self
 * @param index
 * @return property at index
 */
PtpControl* PTP_EXPORT PTPControl_GetControlAtIndex(PTPControl* self, u16 index);

PtpControl* PTP_EXPORT PTPControl_GetControl(PTPControl* self, u16 controlCode);
PTPResult PTP_EXPORT PTPControl_SetControl(PTPControl* self, u16 controlCode, PTPPropValue value);
PTPResult PTP_EXPORT PTPControl_SetControlToggle(PTPControl* self, u16 controlCode, b32 pressed);
b32 PTP_EXPORT PTPControl_GetEnumsForControl(PTPControl* self, u16 controlCode, PTPPropValueEnums* outEnums);


//////////////////////////////////////////////////////////////////////////////////////////////
// File download / upload functions
//////////////////////////////////////////////////////////////////////////////////////////////
int PTP_EXPORT PTPControl_GetPendingFiles(PTPControl* self);
PTPResult PTP_EXPORT PTPControl_GetLiveViewImage(PTPControl* self, MMemIO* fileOut, LiveViewFrames* liveViewFramesOut);
PTPResult PTP_EXPORT PTPControl_GetCapturedImage(PTPControl* self, MMemIO* fileOut, PTPCapturedImageInfo* ciiOut);
PTPResult PTP_EXPORT PTPControl_GetCameraSettingsFile(PTPControl* self, MMemIO* fileOut);
PTPResult PTP_EXPORT PTPControl_PutCameraSettingsFile(PTPControl* self, MMemIO* fileIn);


//////////////////////////////////////////////////////////////////////////////////////////////
// Release temporary data
//////////////////////////////////////////////////////////////////////////////////////////////
void PTP_EXPORT PTP_FreeLiveViewFrames(MAllocator* mem, LiveViewFrames* liveViewFrames);
void PTP_EXPORT PTP_FreePropValueEnums(MAllocator* mem, PTPPropValueEnums* outEnums);


//////////////////////////////////////////////////////////////////////////////////////////////
// String conversion, value helpers
//////////////////////////////////////////////////////////////////////////////////////////////
char* PTP_EXPORT PTP_GetOperationStr(u16 operationCode);
char* PTP_EXPORT PTP_GetControlStr(u16 controlCode);
char* PTP_EXPORT PTP_GetEventStr(u16 eventCode);
char* PTP_EXPORT PTP_GetPropertyStr(u16 propCode);
char* PTP_EXPORT PTP_GetObjectFormatStr(u16 objectFormatCode);

char* PTP_EXPORT PTP_GetDataTypeStr(PTPDataType dataType);
char* PTP_EXPORT PTP_GetFormFlagStr(PTPFormFlag formFlag);
char* PTP_EXPORT PTP_GetPropIsEnabledStr(u8 propIsEnabled);

void PTP_EXPORT PTP_GetPropValueStr(PTPDataType dataType, PTPPropValue value, char* buffer, size_t bufferLen);
b32 PTP_EXPORT PTP_PropValueEq(PTPDataType dataType, PTPPropValue value1, PTPPropValue value2);

#ifdef __cplusplus
} // extern "C"
#endif
