#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sPTPControl {
    PTPDeviceTransport* deviceTransport;

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

    PtpProperty* properties;
    PtpControl* controls;

    u32 sessionId;
    u32 transactionId;

    u8* dataInMem;
    u32 dataInSize;
    u32 dataInCapacity;
    u8* dataOutMem;
    u32 dataOutSize;
    u32 dataOutCapacity;
    PtpRequest ptpRequest;
    PtpResponse ptpResponse;
} PTPControl;

PTPResult PTPControl_Init(PTPControl* self, PTPDeviceTransport* deviceTransport);
PTPResult PTPControl_Connect(PTPControl* self, SonyProtocolVersion version);
PTPResult PTPControl_Cleanup(PTPControl* self);
PTPResult PtpControl_UpdateProperties(PTPControl* self);

b32 PtpControl_SupportsEvent(PTPControl* self, u16 eventCode);
b32 PtpControl_SupportsControl(PTPControl* self, u16 controlCode);
b32 PtpControl_SupportsProperty(PTPControl* self, u16 propertyCode);
b32 PtpControl_PropertyEnabled(PTPControl* self, u16 propertyCode);

PtpProperty* PtpControl_GetProperty(PTPControl* self, u16 propertyCode);
b32 PTPControl_GetEnumsForProperty(PTPControl* self, u16 propertyCode, PropValueEnums* outEnums);
b32 PTPControl_GetPropertyAsStr(PTPControl* self, u16 propertyCode, MStr* strOut);
PTPResult PTPControl_SetProperty(PTPControl* self, u16 propertyCode, PropValue value);
b32 PTPControl_SetPropertyU16(PTPControl* self, u16 propertyCode, u16 value);
b32 PTPControl_SetPropertyU32(PTPControl* self, u16 propertyCode, u32 value);
b32 PTPControl_SetPropertyU64(PTPControl* self, u16 propertyCode, u64 value);
b32 PTPControl_SetPropertyStr(PTPControl* self, u16 propertyCode, MStr value);
b32 PTPControl_SetPropertyFancy(PTPControl* self, u16 propertyCode, MStr value);

PtpControl* PTPControl_GetControl(PTPControl* self, u16 controlCode);
PTPResult PTPControl_SetControl(PTPControl* self, u16 controlCode, PropValue value);
PTPResult PTPControl_SetControlToggle(PTPControl* self, u16 controlCode, b32 pressed);
b32 PTPControl_GetEnumsForControl(PTPControl* self, u16 controlCode, PropValueEnums* outEnums);

int PtpControl_GetPendingFiles(PTPControl* self);
PTPResult PtpControl_GetLiveViewImage(PTPControl* self, MMemIO* fileOut, LiveViewFrames* liveViewFramesOut);
PTPResult PtpControl_GetCapturedImage(PTPControl* self, MMemIO* fileOut, PtpCapturedImageInfo* ciiOut);
PTPResult PtpControl_GetCameraSettingsFile(PTPControl* self, MMemIO* fileOut);
PTPResult PtpControl_PutCameraSettingsFile(PTPControl* self, MMemIO* fileIn);

void Ptp_FreeLiveViewFrames(LiveViewFrames* liveViewFrames);
void Ptp_FreePropValueEnums(PropValueEnums* outEnums);

char* Ptp_GetOperationStr(u16 operationCode);
char* Ptp_GetControlStr(u16 controlCode);
char* Ptp_GetEventStr(u16 eventCode);
char* Ptp_GetPropertyStr(u16 propCode);
char* Ptp_GetObjectFormatStr(u16 objectFormatCode);

char* Ptp_GetDataTypeStr(PTPDataType dataType);
char* Ptp_GetFormFlagStr(PTPFormFlag formFlag);
char* Ptp_GetPropIsEnabledStr(u8 propIsEnabled);

void Ptp_GetPropValueStr(PTPDataType dataType, PropValue value, char* buffer, size_t bufferLen);
b32 Ptp_PropValueEq(PTPDataType dataType, PropValue value1, PropValue value2);

#ifdef __cplusplus
} // extern "C"
#endif
