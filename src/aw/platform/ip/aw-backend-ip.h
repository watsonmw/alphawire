#pragma once

#include "aw/aw-backend.h"

#ifdef __cplusplus
extern "C" {
#endif

AwResult AwIpDeviceList_OpenBackend(AwBackend* backend, int timeoutMilliseconds);

#ifdef __cplusplus
}
#endif
