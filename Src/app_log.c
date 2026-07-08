#include "app_log.h"

#include "config.h"
#include "storage.h"
#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static char log_file_path[512];

static const char *app_log_level_name(AppLogLevel level)
{
    switch (level)
    {
    case APP_LOG_DEBUG: return "DEBUG";
    case APP_LOG_INFO: return "INFO";
    case APP_LOG_WARNING: return "WARN";
    case APP_LOG_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

static void app_log_truncate_if_needed(void)
{
    FILE *file;
    long size;

    file = storage_open_read(log_file_path);
    if (file == NULL)
        return;
    if (fseek(file, 0, SEEK_END) != 0)
    {
        (void)storage_close(file);
        return;
    }
    size = ftell(file);
    (void)storage_close(file);
    if (size > LOG_FILE_MAX_BYTES)
    {
        file = storage_open_write(log_file_path);
        if (file != NULL)
        {
            (void)fprintf(file, "--- log truncated after reaching %d bytes ---\n",
                          LOG_FILE_MAX_BYTES);
            (void)storage_close(file);
        }
    }
}

void app_log_initialize(const char *file_path)
{
    if (file_path == NULL)
        return;
    utils_copy_string(log_file_path, file_path, sizeof(log_file_path));
    app_log_truncate_if_needed();
}

void app_log_write(AppLogLevel level,
                   const char *module,
                   const char *format,
                   ...)
{
    FILE *file;
    time_t now;
    struct tm local_time;
    char time_text[32];
    va_list arguments;

    if (log_file_path[0] == '\0' || format == NULL)
        return;
    file = storage_open_append(log_file_path);
    if (file == NULL)
        return;
    now = time(NULL);
#ifdef _WIN32
    (void)localtime_s(&local_time, &now);
#else
    (void)localtime_r(&now, &local_time);
#endif
    (void)strftime(time_text, sizeof(time_text), "%Y-%m-%d %H:%M:%S", &local_time);
    (void)fprintf(file, "[%s] [%s] [%s] ", time_text,
                  app_log_level_name(level), module == NULL ? "app" : module);
    va_start(arguments, format);
    (void)vfprintf(file, format, arguments);
    va_end(arguments);
    (void)fputc('\n', file);
    (void)storage_close(file);
}
