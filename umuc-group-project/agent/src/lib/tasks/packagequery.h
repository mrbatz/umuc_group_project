#ifndef _PACKAGE_QUERY_H
#define _PACKAGE_QUERY_H

#include "dirwalk.h"

#define MAX_COMMAND_LENGTH 1024
#define LOCAL_QUERYING_ENABLED 1
#define REMOTE_QUERYING_ENABLED 0

extern int query_packages(char fullpath[MAX_PATH_LENGTH]);
extern int packagewalkV(int argc, va_list argv);
extern int packagewalkA(int argc, ...);
extern int packagewalk(char* root);
#endif
