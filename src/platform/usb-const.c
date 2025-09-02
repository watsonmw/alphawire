#include "usb-const.h"

#include <stdio.h>

int USB_BcdVersionAsString(u16 bcdVersion, char* dstStr, size_t dstStrLen) {
    u8 major = (bcdVersion >> 8) & 0xFF;
    u8 minor = ((bcdVersion >> 4) & 0x0F);
    u8 subMinor = (bcdVersion & 0x0F);
    if (subMinor) {
        return snprintf(dstStr, dstStrLen, "%d.%d.%d", major, minor, subMinor);
    } else {
        return snprintf(dstStr, dstStrLen, "%d.%d", major, minor);
    }
}
