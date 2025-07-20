#include "win-utils.h"

#include <windows.h>

#include "mlib/mlib.h"

b32 WinUtils_GetLastErrorAsStr(char* buffer, size_t bufferSize)
{
    // Get the error code
    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0) {
        return FALSE;
    }

    size_t size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorMessageID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPSTR)buffer,
        bufferSize,
        NULL
    );

    // Strip \r or \n
    if (size > 0) {
        char* end = buffer + (size - 1);
        while (end > buffer) {
            if (*end == '\n' || *end == '\r') {
                *end = '\0';
            } else {
                break;
            }
            end--;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

void WinUtils_LogLastError(PTPLog* logger, const char* mesg) {
    if (WinUtils_GetLastErrorAsStr(logger->msgBuffer, sizeof(logger->msgBuffer))) {
        PTP_LOG_ERROR_F(logger, "%s: %s (0x%08x)", mesg, logger->msgBuffer, GetLastError());
    } else {
        PTP_LOG_ERROR(logger, mesg);
    }
}
