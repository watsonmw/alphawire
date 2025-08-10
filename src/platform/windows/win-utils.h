#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-log.h"

b32 WinUtils_GetLastErrorAsStr(char* buffer, size_t bufferSize);
void WinUtils_LogLastError(PTPLog* logger, const char* mesg);
MStr WinUtils_BSTRToUTF8(BSTR bstr);
