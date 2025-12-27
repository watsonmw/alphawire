#include "ptp/ptp-backend.h"

const char* PTPBackend_GetTypeAsStr(PTPBackendType type) {
    switch (type) {
        case PTP_BACKEND_WIA:
            return "WIA";
        case PTP_BACKEND_LIBUSBK:
            return "libusbk";
        case PTP_BACKEND_IOKIT:
            return "IOKit";
        case PTP_BACKEND_LIBUSB:
            return "libusb";
    }
    MBreakpointf("PTPBackend_GetTypeAsStr: Unknown PTPBackendType %d", type);
    return "Unknown";
}
