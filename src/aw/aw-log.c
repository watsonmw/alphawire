#include "aw/aw-log.h"
#include "mlib/mlib.h"

#include <stdarg.h>
#include <stdio.h>

void AwLog_Log(AwLog* logger, AwLogLevel level, const char *format, ...) {
    if (!logger || !logger->logFunc) {
        return;
    }

    if (level > logger->level) {
        return;
    }

    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logger->logFunc(logger, level, buffer);
}

char* AwLog_LevelAsStr(AwLogLevel level) {
    switch (level) {
        case AW_LOG_LEVEL_TRACE:
            return "TRACE";
        case AW_LOG_LEVEL_DEBUG:
            return "DEBUG";
        case AW_LOG_LEVEL_INFO:
            return "INFO";
        case AW_LOG_LEVEL_WARNING:
            return "WARNING";
        case AW_LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void AwLog_LogDefault(AwLog* logger, AwLogLevel level, const char *message) {
    MLogf("%s: %s", AwLog_LevelAsStr(level), message);
}
