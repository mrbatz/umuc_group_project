#ifndef _LOG_H
#define _LOG_H
/*
 * The logger file is intended to be responsible for all logging that takes place on the agent.
 * Avoiding the use of STDOUT/STDERR because the agent is intended to be a daemon.
 */

#include <stdarg.h>

#define ATTACH_DAEMON 0
#define LOG_NAME "/tmp/agent.log"
#define LOG_LEVEL DBG
#define MAX_LOG_MESSAGE_SIZE 1024




#define LOG_TYPES TYPE(CRIT)TYPE(ERR)TYPE(WARN)TYPE(INFO)TYPE(DBG)
#define TYPE(x) x,
typedef enum LOGTYPE {
    LOG_TYPES TOP
} LogTypes;
#undef TYPE

extern char* log_name;
extern void _logV(LogTypes type, const char* cfunc, const char* cfile, const long cline, const char* message, va_list argv);
extern void _log(LogTypes type, const char* cfunc, const char* cfile, const long cline, const char* message, ...);

#define log(type, message, ...) _log(type, __FUNCTION__, __FILE__, __LINE__, (message), ##__VA_ARGS__)

#endif
