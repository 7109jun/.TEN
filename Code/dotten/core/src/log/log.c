#include "ten/log.h"
#include "internal/error_internal.h"

#include <stdarg.h>
#include <stdio.h>

#define TEN_LOG_MESSAGE_BUF_SIZE 4096

static ten_log_level_t g_ten_log_min_level = TEN_LOG_TRACE;
static int g_ten_log_console_enabled = 1;
static FILE *g_ten_log_file = NULL;
static ten_log_callback_t g_ten_log_callback = NULL;
static void *g_ten_log_callback_userdata = NULL;

static const char *ten__log_level_name(ten_log_level_t level)
{
    switch (level)
    {
        case TEN_LOG_TRACE: return "TRACE";
        case TEN_LOG_DEBUG: return "DEBUG";
        case TEN_LOG_INFO:  return "INFO";
        case TEN_LOG_WARN:  return "WARN";
        case TEN_LOG_ERROR: return "ERROR";
        case TEN_LOG_FATAL: return "FATAL";
        default:            return "UNKNOWN";
    }
}

static void ten__log_write(ten_log_level_t level, const char *format, va_list args)
{
    char message[TEN_LOG_MESSAGE_BUF_SIZE];

    vsnprintf(message, sizeof(message), format, args);

    if (g_ten_log_console_enabled)
    {
        FILE *stream = (level >= TEN_LOG_WARN) ? stderr : stdout;
        fprintf(stream, "[%s] %s\n", ten__log_level_name(level), message);
    }

    if (g_ten_log_file != NULL)
    {
        fprintf(g_ten_log_file, "[%s] %s\n", ten__log_level_name(level), message);
        fflush(g_ten_log_file);
    }

    if (g_ten_log_callback != NULL)
    {
        g_ten_log_callback(level, message, g_ten_log_callback_userdata);
    }
}

void ten_log(ten_log_level_t level, const char *format, ...)
{
    va_list args;

    if (format == NULL)
    {
        ten__set_error(TEN_ERROR_NULL_ARGUMENT);
        return;
    }

    if (level < g_ten_log_min_level)
    {
        ten__set_error(TEN_OK);
        return;
    }

    va_start(args, format);
    ten__log_write(level, format, args);
    va_end(args);

    ten__set_error(TEN_OK);
}

#define TEN_DEFINE_LOG_LEVEL_FN(name, level_value)                  \
    void ten_log_##name(const char *format, ...)                    \
    {                                                                \
        va_list args;                                                \
        if (format == NULL)                                          \
        {                                                             \
            ten__set_error(TEN_ERROR_NULL_ARGUMENT);                   \
            return;                                                     \
        }                                                                \
        if ((level_value) < g_ten_log_min_level)                          \
        {                                                                  \
            ten__set_error(TEN_OK);                                         \
            return;                                                          \
        }                                                                     \
        va_start(args, format);                                                \
        ten__log_write((level_value), format, args);                           \
        va_end(args);                                                           \
        ten__set_error(TEN_OK);                                                  \
    }

TEN_DEFINE_LOG_LEVEL_FN(trace, TEN_LOG_TRACE)
TEN_DEFINE_LOG_LEVEL_FN(debug, TEN_LOG_DEBUG)
TEN_DEFINE_LOG_LEVEL_FN(info, TEN_LOG_INFO)
TEN_DEFINE_LOG_LEVEL_FN(warn, TEN_LOG_WARN)
TEN_DEFINE_LOG_LEVEL_FN(error, TEN_LOG_ERROR)
TEN_DEFINE_LOG_LEVEL_FN(fatal, TEN_LOG_FATAL)

#undef TEN_DEFINE_LOG_LEVEL_FN

void ten_log_set_level(ten_log_level_t level)
{
    g_ten_log_min_level = level;
}

void ten_log_set_console(int enabled)
{
    g_ten_log_console_enabled = enabled;
}

int ten_log_set_file(const char *path)
{
    if (g_ten_log_file != NULL)
    {
        fclose(g_ten_log_file);
        g_ten_log_file = NULL;
    }

    if (path == NULL)
    {
        ten__set_error(TEN_OK);
        return (int)TEN_OK;
    }

    g_ten_log_file = fopen(path, "a");
    if (g_ten_log_file == NULL)
    {
        ten__set_error(TEN_ERROR_IO);
        return (int)TEN_ERROR_IO;
    }

    ten__set_error(TEN_OK);
    return (int)TEN_OK;
}

void ten_log_set_callback(ten_log_callback_t callback, void *userdata)
{
    g_ten_log_callback = callback;
    g_ten_log_callback_userdata = userdata;
}
