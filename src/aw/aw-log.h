#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "aw-const.h"

#define AW_LOG_LEVEL_TRACE_VALUE 4
#define AW_LOG_LEVEL_DEBUG_VALUE 3
#define AW_LOG_LEVEL_INFO_VALUE 2
#define AW_LOG_LEVEL_WARNING_VALUE 1
#define AW_LOG_LEVEL_ERROR_VALUE 0

typedef enum {
    AW_LOG_LEVEL_TRACE = AW_LOG_LEVEL_TRACE_VALUE,
    AW_LOG_LEVEL_DEBUG = AW_LOG_LEVEL_DEBUG_VALUE,
    AW_LOG_LEVEL_INFO = AW_LOG_LEVEL_INFO_VALUE,
    AW_LOG_LEVEL_WARNING = AW_LOG_LEVEL_WARNING_VALUE,
    AW_LOG_LEVEL_ERROR = AW_LOG_LEVEL_ERROR_VALUE
} AwLogLevel;

struct AwLog;
typedef void (*AwPLog_Log_Func)(struct AwLog* logger, AwLogLevel level, const char *message);

typedef struct AwLog {
    AwLogLevel level;
    AwPLog_Log_Func logFunc;
    void* userData;
    char msgBuffer[1024];
} AwLog;

AW_EXPORT void AwLog_Log(AwLog* logger, AwLogLevel level, const char *fmt, ...);
AW_EXPORT void AwLog_LogDefault(AwLog* logger, AwLogLevel level, const char *message);
AW_EXPORT char* AwLog_LevelAsStr(AwLogLevel level);

#if AW_LOG_LEVEL >= AW_LOG_LEVEL_TRACE_VALUE
#define AW_LOG_TRACE(logger, msg) { AwLog_Log(logger, AW_LOG_LEVEL_TRACE, msg); }
#define AW_LOG_TRACE_F(logger, fmt, ...) { AwLog_Log(logger, AW_LOG_LEVEL_TRACE, fmt, __VA_ARGS__); }
#else
#define AW_LOG_TRACE(logger, fmt, ...)
#define AW_LOG_TRACE_F(logger, fmt, ...)
#endif

#if AW_LOG_LEVEL >= AW_LOG_LEVEL_DEBUG_VALUE
#define AW_LOG_DEBUG(logger, msg) { AwLog_Log(logger, AW_LOG_LEVEL_DEBUG, msg); }
#define AW_LOG_DEBUG_F(logger, fmt, ...) { AwLog_Log(logger, AW_LOG_LEVEL_DEBUG, fmt, __VA_ARGS__); }
#else
#define AW_LOG_DEBUG(logger, fmt, ...)
#define AW_LOG_DEBUG_F(logger, fmt, ...)
#endif

#if AW_LOG_LEVEL >= AW_LOG_LEVEL_INFO_VALUE
#define AW_LOG_INFO(logger, msg) AwLog_Log(logger, AW_LOG_LEVEL_INFO, msg)
#define AW_LOG_INFO_F(logger, fmt, ...) AwLog_Log(logger, AW_LOG_LEVEL_INFO, fmt, __VA_ARGS__)
#else
#define AW_LOG_INFO(logger, fmt, ...)
#define AW_LOG_INFO_F(logger, fmt, ...)
#endif

#if AW_LOG_LEVEL >= AW_LOG_LEVEL_WARNING_VALUE
#define AW_LOG_WARNING(logger, msg) AwLog_Log(logger, AW_LOG_LEVEL_WARNING, msg)
#define AW_LOG_WARNING_F(logger, fmt, ...) AwLog_Log(logger, AW_LOG_LEVEL_WARNING, fmt, __VA_ARGS__)
#else
#define AW_LOG_WARNING(logger, fmt, ...)
#define AW_LOG_WARNING_F(logger, fmt, ...)
#endif

#if AW_LOG_LEVEL >= AW_LOG_LEVEL_ERROR_VALUE
#define AW_LOG_ERROR(logger, msg) AwLog_Log(logger, AW_LOG_LEVEL_ERROR, msg)
#define AW_LOG_ERROR_F(logger, fmt, ...) AwLog_Log(logger, AW_LOG_LEVEL_ERROR, fmt, __VA_ARGS__)
#else
#define AW_LOG_ERROR(logger, fmt, ...)
#define AW_LOG_ERROR_F(logger, fmt, ...)
#endif

#define AW_TRACE(msg) AW_LOG_TRACE(&self->logger, msg)
#define AW_TRACE_F(fmt, ...) AW_LOG_TRACE_F(&self->logger, fmt, __VA_ARGS__)
#define AW_DEBUG(msg) AW_LOG_DEBUG(&self->logger, msg)
#define AW_DEBUG_F(fmt, ...) AW_LOG_DEBUG_F(&self->logger, fmt, __VA_ARGS__)
#define AW_INFO(msg) AW_LOG_INFO(&self->logger, msg)
#define AW_INFO_F(fmt, ...) AW_LOG_INFO_F(&self->logger, fmt, __VA_ARGS__)
#define AW_WARNING(msg) AW_LOG_WARNING(&self->logger, msg)
#define AW_WARNING_F(fmt, ...) AW_LOG_WARNING_F(&self->logger, fmt, __VA_ARGS__)
#define AW_ERROR(msg) AW_LOG_ERROR(&self->logger, msg)
#define AW_ERROR_F(fmt, ...) AW_LOG_ERROR_F(&self->logger, fmt, __VA_ARGS__)

#ifdef __cplusplus
}
#endif
