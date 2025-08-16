#include "ptp/ptp-log.h"
#include "mlib/mlib.h"

#include <stdarg.h>
#include <stdio.h>

void PTPLog_Log(PTPLog* logger, PTPLogLevel level, const char *format, ...) {
    if (!logger || !logger->logFunc) {
        return;
    }

    if (level < logger->level) {
        return;
    }

    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logger->logFunc(logger, level, buffer);
}

char* PTPLog_LevelAsStr(PTPLogLevel level) {
    switch (level) {
        case PTP_LOG_LEVEL_TRACE:
            return "TRACE";
        case PTP_LOG_LEVEL_DEBUG:
            return "DEBUG";
        case PTP_LOG_LEVEL_INFO:
            return "INFO";
        case PTP_LOG_LEVEL_WARNING:
            return "WARNING";
        case PTP_LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void PTPLog_LogDefault(PTPLog* logger, PTPLogLevel level, const char *message) {
    MLogf("%s: %s", PTPLog_LevelAsStr(level), message);
}
