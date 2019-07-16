#include <stdio.h>
#include "logger.h"
#include "tasking.h"
#include "blacklist.h"
#include "tasks/packagequery.h"
#include "tasks/dirwalk.h"

#define TASK(x) #x,
const char* const TaskTypes[] = { TASKS };
#undef TASK

static int compatibilityCheck(Task t)
{
    return 0; //Not yet implemented
}

extern TaskStatus doTaskV(Task t, int argc, va_list argv)
{
    if (compatibilityCheck(t)) {
        return INCOMPATIBLE;
    }

    switch(t) {
        case DirWalk:
            return default_dirwalkV(argc, argv);
        case PackageQuery:
            return packagewalkV(argc, argv); //Only task for milestone 1

        default:
            break;
    }

    return ERROR; //Invalid task provided
}

extern TaskStatus doTaskA(Task t, int argc, ...)
{
    va_list argv;
    va_start(argv, argc);

    TaskStatus ret = doTaskV(t, argc, argv);
    va_end(argv);
    return ret;
}
