#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#include "logger.h"
#include "date.h"

#define TYPE(x) #x,
const char* log_types[] = { LOG_TYPES };
#undef TYPE

extern void _logV(LogTypes type, const char* cfunc, const char* cfile, const long cline, const char* message, va_list argv)
{
    if (type > LOG_LEVEL){
        return;
    }

    FILE *log_file = NULL;
    if (!ATTACH_DAEMON)
    {
        log_file = fopen(LOG_NAME, "a");
    }

    char log_buffer[MAX_LOG_MESSAGE_SIZE] = { '\0' };
    char timestamp[DATE_SIZE] = { '\0' };
    if (get_timestamp(timestamp))
    {
        strncpy(timestamp, "UNKNOWN DATE/TIME", DATE_SIZE);
    }

    sprintf(log_buffer, "%s: %s - %s - %s - line %ld: %s\n", log_types[type], timestamp, cfunc, cfile, cline, message);

    if (log_file == NULL)
    {
        vprintf(log_buffer, argv);
        return;
    }

    vfprintf(log_file, log_buffer, argv);
    fclose(log_file);
}

extern void _log(LogTypes type, const char* cfunc, const char* cfile, const long cline, const char* message, ...)
{
    va_list argv;
    va_start(argv, message);
    _logV(type, cfunc, cfile, cline, message, argv);
    va_end(argv);
}
