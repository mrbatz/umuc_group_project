#ifndef _DIRWALK_H
#define _DIRWALK_H

#define MAX_PATH_LENGTH 512
#define FULLPATH_LOGGING 1
#define TREE_LOGGING 1

#include "../tasking.h"

typedef int (*WalkFunction)(char fullpath[MAX_PATH_LENGTH]);

extern TaskStatus dirwalkV(WalkFunction ptr, int argc, va_list argv);
extern TaskStatus dirwalkA(WalkFunction ptr, int argc, ...);
extern TaskStatus dirwalk(WalkFunction ptr, char* root);

extern TaskStatus default_dirwalkV(int argc, va_list argv);
extern TaskStatus default_dirwalkA(int argc, ...);
extern TaskStatus default_dirwalk(char* root);
#endif
