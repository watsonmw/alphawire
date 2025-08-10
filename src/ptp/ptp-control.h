#pragma once

#include "ptp-log.h"
#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

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
    PTPDeviceTransport* deviceTransport;
    PTPLog logger;

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
 *    PTPDeviceList_Open(&ptpDeviceList);
 *    // Poll for devices
 *    PTPDeviceList_RefreshList(&ptpDeviceList);
 *    // If one or more devices
 *    if (MArraySize(ptpDeviceList.devices) {
 *        PTPDeviceInfo* deviceInfo = ptpDeviceList.devices;
 *        PTPDevice* device = NULL;
 *        // Establish transport for first device
 *        PTPDeviceList_ConnectDevice(&ptpDeviceList, deviceInfo, &device);
 *        PTPControl ptp{};
 *        // Init control structure
 *        PTPControl_Init(&ptp, device);
 *        // Connect to device with given mode
 *        PTPControl_Connect(&ptp, SDI_EXTENSION_VERSION_300);
 *    }
 * @endcode
 *
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTPResult PTPControl_Init(PTPControl* self, PTPDevice* device);

/**
 * Establishes a connection with a Sony device over PTP (Picture Transfer Protocol), on the previously setup transport.
 *
 * Opens a new session to the device, performs authentication, retrieves device and property information, and prepares
 * the device for control operations.
 *
 * @param version The Sony protocol version to connect with.  Year 2020+ cameras support SDI_EXTENSION_VERSION_300,
 *                which has more properties and controls, along with other API imrpovements over the
 *                SDI_EXTENSION_VERSION_200 protocol.
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTPResult PTPControl_Connect(PTPControl* self, SonyProtocolVersion version);

/**
 * Cleanup the PTPControl structure.
 * Depending on the underlying transport this may also close the PTP session, if it does not then releasing the
 * transport will close the PTP session.
 * After PTPControl_Cleanup() is called, the device transport should be closed.
 *
 * @return Returns PTP_OK on success, or an appropriate error code on failure.
 */
PTPResult PTPControl_Cleanup(PTPControl* self);


//////////////////////////////////////////////////////////////////////////////////////////////
// Check support for various events, controls, and properties
//////////////////////////////////////////////////////////////////////////////////////////////

b32 PTPControl_SupportsEvent(PTPControl* self, u16 eventCode);
b32 PTPControl_SupportsControl(PTPControl* self, u16 controlCode);
b32 PTPControl_SupportsProperty(PTPControl* self, u16 propertyCode);
b32 PTPControl_PropertyEnabled(PTPControl* self, u16 propertyCode);


//////////////////////////////////////////////////////////////////////////////////////////////
// Generic property getter and setters
//////////////////////////////////////////////////////////////////////////////////////////////
PTPResult PTPControl_UpdateProperties(PTPControl* self);
PTPProperty* PTPControl_GetProperty(PTPControl* self, u16 propertyCode);
b32 PTPControl_GetEnumsForProperty(PTPControl* self, u16 propertyCode, PTPPropValueEnums* outEnums);
b32 PTPControl_GetPropertyAsStr(PTPControl* self, u16 propertyCode, MStr* strOut);
PTPResult PTPControl_SetProperty(PTPControl* self, u16 propertyCode, PTPPropValue value);
b32 PTPControl_SetPropertyU16(PTPControl* self, u16 propertyCode, u16 value);
b32 PTPControl_SetPropertyU32(PTPControl* self, u16 propertyCode, u32 value);
b32 PTPControl_SetPropertyU64(PTPControl* self, u16 propertyCode, u64 value);
b32 PTPControl_SetPropertyStr(PTPControl* self, u16 propertyCode, MStr value);
b32 PTPControl_SetPropertyFancy(PTPControl* self, u16 propertyCode, MStr value);
b32 PTPControl_SetPropertyNotch(PTPControl* self, u16 propertyCode, i8 notch);


//////////////////////////////////////////////////////////////////////////////////////////////
// Generic controls getters and setters
//////////////////////////////////////////////////////////////////////////////////////////////
PtpControl* PTPControl_GetControl(PTPControl* self, u16 controlCode);
PTPResult PTPControl_SetControl(PTPControl* self, u16 controlCode, PTPPropValue value);
PTPResult PTPControl_SetControlToggle(PTPControl* self, u16 controlCode, b32 pressed);
b32 PTPControl_GetEnumsForControl(PTPControl* self, u16 controlCode, PTPPropValueEnums* outEnums);


//////////////////////////////////////////////////////////////////////////////////////////////
// File download / upload functions
//////////////////////////////////////////////////////////////////////////////////////////////
int PTPControl_GetPendingFiles(PTPControl* self);
PTPResult PTPControl_GetLiveViewImage(PTPControl* self, MMemIO* fileOut, LiveViewFrames* liveViewFramesOut);
PTPResult PTPControl_GetCapturedImage(PTPControl* self, MMemIO* fileOut, PTPCapturedImageInfo* ciiOut);
PTPResult PTPControl_GetCameraSettingsFile(PTPControl* self, MMemIO* fileOut);
PTPResult PTPControl_PutCameraSettingsFile(PTPControl* self, MMemIO* fileIn);


//////////////////////////////////////////////////////////////////////////////////////////////
// Release temporary data
//////////////////////////////////////////////////////////////////////////////////////////////
void PTP_FreeLiveViewFrames(LiveViewFrames* liveViewFrames);
void PTP_FreePropValueEnums(PTPPropValueEnums* outEnums);


//////////////////////////////////////////////////////////////////////////////////////////////
// String conversion, value helpers
//////////////////////////////////////////////////////////////////////////////////////////////
char* PTP_GetOperationStr(u16 operationCode);
char* PTP_GetControlStr(u16 controlCode);
char* PTP_GetEventStr(u16 eventCode);
char* PTP_GetPropertyStr(u16 propCode);
char* PTP_GetObjectFormatStr(u16 objectFormatCode);

char* PTP_GetDataTypeStr(PTPDataType dataType);
char* PTP_GetFormFlagStr(PTPFormFlag formFlag);
char* PTP_GetPropIsEnabledStr(u8 propIsEnabled);

void PTP_GetPropValueStr(PTPDataType dataType, PTPPropValue value, char* buffer, size_t bufferLen);
b32 PTP_PropValueEq(PTPDataType dataType, PTPPropValue value1, PTPPropValue value2);

#ifdef __cplusplus
} // extern "C"
#endif
