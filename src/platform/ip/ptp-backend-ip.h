#pragma once

#include "ptp/ptp-backend.h"

#ifdef __cplusplus
extern "C" {
#endif

AwResult PTPIpDeviceList_OpenBackend(PTPBackend* backend, int timeoutMilliseconds);

#ifdef __cplusplus
}
#endif
