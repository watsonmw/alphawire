#include <windows.h>

#include "win-utils.h"
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

void sWinUtils_LogLastError(PTPLog* logger, const char* mesg) {
    if (WinUtils_GetLastErrorAsStr(logger->msgBuffer, sizeof(logger->msgBuffer))) {
        PTP_LOG_ERROR_F(logger, "%s: %s (0x%08x)", mesg, logger->msgBuffer, GetLastError());
    } else {
        PTP_LOG_ERROR(logger, mesg);
    }
}


// Helper function to convert BSTR to UTF-8
MStr WinUtils_BSTRToUTF8(BSTR bstr) {
    MStr r = {};
    if (!bstr) {
        return r;
    }

    // Get required buffer size
    int len = WideCharToMultiByte(CP_UTF8, 0, bstr, -1, NULL, 0, NULL, NULL);
    if (len == 0) {
        return r;
    }

    // Allocate buffer and convert
    MStrInit(r, len);
    if (!r.str) {
        return r;
    }

    int bytesWritten = WideCharToMultiByte(CP_UTF8, 0, bstr, -1, r.str, len, NULL, NULL);
    if (bytesWritten == 0) {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_INSUFFICIENT_BUFFER:
                MLogf("WideCharToMultiByte: ERROR_INSUFFICIENT_BUFFER");
                break;
            case ERROR_INVALID_FLAGS:
                MLogf("WideCharToMultiByte: ERROR_INVALID_FLAGS");
                break;
            case ERROR_INVALID_PARAMETER:
                MLogf("WideCharToMultiByte: ERROR_INVALID_PARAMETER");
                break;
            case ERROR_NO_UNICODE_TRANSLATION:
                MLogf("WideCharToMultiByte: ERROR_NO_UNICODE_TRANSLATION");
                break;
            default:
                MLogf("WideCharToMultiByte: %x", err);
                break;
        }
        MStrFree(r);
        return r;
    }

    return r;
}
