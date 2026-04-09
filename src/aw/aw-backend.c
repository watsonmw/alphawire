#include "aw/aw-backend.h"

const char* AwBackend_GetTypeAsStr(AwBackendType type) {
    switch (type) {
        case AW_BACKEND_WIA:
            return "WIA";
        case AW_BACKEND_LIBUSBK:
            return "libusbk";
        case AW_BACKEND_IOKIT:
            return "IOKit";
        case AW_BACKEND_LIBUSB:
            return "libusb";
        case AW_BACKEND_IP:
            return "ip";
    }
    MBreakpointf("AwBackend_GetTypeAsStr: Unknown AwBackendType %d", type);
    return "Unknown";
}
