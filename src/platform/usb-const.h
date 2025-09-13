#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

#define USB_SONY_VID 0x054C
#define USB_EN_US 0x0409
#define USB_PROTOCOL_PTP 0x01
#define USB_CLASS_STILL_IMAGE 0x06
#define USB_SUBCLASS_STILL_IMAGE 0x01
#define USB_BULK_MAX_PACKET_SIZE 512
#define USB_TIMEOUT_DEFAULT_MILLISECONDS 20000

// PTP Container Types
#define PTP_CONTAINER_COMMAND  0x0001
#define PTP_CONTAINER_DATA     0x0002
#define PTP_CONTAINER_RESPONSE 0x0003
#define PTP_CONTAINER_EVENT    0x0004

typedef MSTRUCTPACKED(struct {
    u32 length;
    u16 type;
    u16 code;
    u32 transactionId;
}) PTPContainerHeader;

/**
 * Convert USB version to a string, the USB version is stored as BCD (binary code decimal).
 * @return number of bytes written, see snprintf()
 */
int PTP_EXPORT USB_BcdVersionAsString(u16 bcdVersion, char* dstStr, size_t dstStrLen);

#ifdef __cplusplus
}
#endif
