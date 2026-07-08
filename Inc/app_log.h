#pragma once

typedef enum AppLogLevel
{
    APP_LOG_DEBUG,
    APP_LOG_INFO,
    APP_LOG_WARNING,
    APP_LOG_ERROR
} AppLogLevel;

void app_log_initialize(const char *file_path);
void app_log_write(AppLogLevel level,
                   const char *module,
                   const char *format,
                   ...);

#define APP_LOG_INFO_MSG(module, ...) \
    app_log_write(APP_LOG_INFO, module, __VA_ARGS__)
#define APP_LOG_WARNING_MSG(module, ...) \
    app_log_write(APP_LOG_WARNING, module, __VA_ARGS__)
#define APP_LOG_ERROR_MSG(module, ...) \
    app_log_write(APP_LOG_ERROR, module, __VA_ARGS__)
