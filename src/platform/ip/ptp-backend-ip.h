#pragma once

#include "ptp/ptp-backend.h"

#ifdef __cplusplus
extern "C" {
#endif

b32 PTPIpDeviceList_OpenBackend(PTPBackend* backend, int timeoutMilliseconds);

#ifdef __cplusplus
}
#endif
