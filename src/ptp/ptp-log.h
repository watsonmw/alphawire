#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ptp-const.h"

#define PTP_LOG_LEVEL_TRACE_VALUE 4
#define PTP_LOG_LEVEL_DEBUG_VALUE 3
#define PTP_LOG_LEVEL_INFO_VALUE 2
#define PTP_LOG_LEVEL_WARNING_VALUE 1
#define PTP_LOG_LEVEL_ERROR_VALUE 0

typedef enum {
    PTP_LOG_LEVEL_TRACE = PTP_LOG_LEVEL_TRACE_VALUE,
    PTP_LOG_LEVEL_DEBUG = PTP_LOG_LEVEL_DEBUG_VALUE,
    PTP_LOG_LEVEL_INFO = PTP_LOG_LEVEL_INFO_VALUE,
    PTP_LOG_LEVEL_WARNING = PTP_LOG_LEVEL_WARNING_VALUE,
    PTP_LOG_LEVEL_ERROR = PTP_LOG_LEVEL_ERROR_VALUE
} PTPLogLevel;

struct PTPLog;
typedef void (*PTPLog_Log_Func)(struct PTPLog* logger, PTPLogLevel level, const char *message);

typedef struct PTPLog {
    PTPLogLevel level;
    PTPLog_Log_Func logFunc;
    char msgBuffer[1024];
} PTPLog;

void PTP_EXPORT PTPLog_Log(PTPLog* logger, PTPLogLevel level, const char *fmt, ...);
void PTP_EXPORT PTPLog_LogDefault(PTPLog* logger, PTPLogLevel level, const char *message);
char* PTP_EXPORT PTPLog_LevelAsStr(PTPLogLevel level);

#if PTP_LOG_LEVEL >= PTP_LOG_LEVEL_TRACE_VALUE
#define PTP_LOG_TRACE(logger, msg) { PTPLog_Log(logger, PTP_LOG_LEVEL_TRACE, msg); }
#define PTP_LOG_TRACE_F(logger, fmt, ...) { PTPLog_Log(logger, PTP_LOG_LEVEL_TRACE, fmt, __VA_ARGS__); }
#else
#define PTP_LOG_TRACE(logger, fmt, ...)
#define PTP_LOG_TRACE_F(logger, fmt, ...)
#endif

#if PTP_LOG_LEVEL >= PTP_LOG_LEVEL_DEBUG_VALUE
#define PTP_LOG_DEBUG(logger, msg) { PTPLog_Log(logger, PTP_LOG_LEVEL_DEBUG, msg); }
#define PTP_LOG_DEBUG_F(logger, fmt, ...) { PTPLog_Log(logger, PTP_LOG_LEVEL_DEBUG, fmt, __VA_ARGS__); }
#else
#define PTP_LOG_DEBUG(logger, fmt, ...)
#define PTP_LOG_DEBUG_F(logger, fmt, ...)
#endif

#if PTP_LOG_LEVEL >= PTP_LOG_LEVEL_INFO_VALUE
#define PTP_LOG_INFO(logger, msg) PTPLog_Log(logger, PTP_LOG_LEVEL_INFO, msg)
#define PTP_LOG_INFO_F(logger, fmt, ...) PTPLog_Log(logger, PTP_LOG_LEVEL_INFO, fmt, __VA_ARGS__)
#else
#define PTP_LOG_INFO(logger, fmt, ...)
#define PTP_LOG_INFO_F(logger, fmt, ...)
#endif

#if PTP_LOG_LEVEL >= PTP_LOG_LEVEL_WARNING_VALUE
#define PTP_LOG_WARNING(logger, msg) PTPLog_Log(logger, PTP_LOG_LEVEL_WARNING, msg)
#define PTP_LOG_WARNING_F(logger, fmt, ...) PTPLog_Log(logger, PTP_LOG_LEVEL_WARNING, fmt, __VA_ARGS__)
#else
#define PTP_LOG_WARNING(logger, fmt, ...)
#define PTP_LOG_WARNING_F(logger, fmt, ...)
#endif

#if PTP_LOG_LEVEL >= PTP_LOG_LEVEL_ERROR_VALUE
#define PTP_LOG_ERROR(logger, msg) PTPLog_Log(logger, PTP_LOG_LEVEL_ERROR, msg)
#define PTP_LOG_ERROR_F(logger, fmt, ...) PTPLog_Log(logger, PTP_LOG_LEVEL_ERROR, fmt, __VA_ARGS__)
#else
#define PTP_LOG_ERROR(logger, fmt, ...)
#define PTP_LOG_ERROR_F(logger, fmt, ...)
#endif

#define PTP_TRACE(msg) PTP_LOG_TRACE(&self->logger, msg)
#define PTP_TRACE_F(fmt, ...) PTP_LOG_TRACE_F(&self->logger, fmt, __VA_ARGS__)
#define PTP_DEBUG(msg) PTP_LOG_DEBUG(&self->logger, msg)
#define PTP_DEBUG_F(fmt, ...) PTP_LOG_DEBUG_F(&self->logger, fmt, __VA_ARGS__)
#define PTP_INFO(msg) PTP_LOG_INFO(&self->logger, msg)
#define PTP_INFO_F(fmt, ...) PTP_LOG_INFO_F(&self->logger, fmt, __VA_ARGS__)
#define PTP_WARNING(msg) PTP_LOG_WARNING(&self->logger, msg)
#define PTP_WARNING_F(fmt, ...) PTP_LOG_WARNING_F(&self->logger, fmt, __VA_ARGS__)
#define PTP_ERROR(msg) PTP_LOG_ERROR(&self->logger, msg)
#define PTP_ERROR_F(fmt, ...) PTP_LOG_ERROR_F(&self->logger, fmt, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
