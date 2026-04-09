#pragma once

#include "mlib/mlib.h"
#include "aw/aw-log.h"

b32 WinUtils_GetLastErrorAsStr(char* buffer, size_t bufferSize);
void WinUtils_LogLastError(AwLog* logger, const char* mesg);
MStr WinUtils_BSTRToUTF8(MAllocator* allocator, BSTR bstr);
MStr WinUtils_BSTRWithSizeToUTF8(MAllocator* allocator, BSTR bstr, i32 size);
