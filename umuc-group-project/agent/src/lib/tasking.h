#ifndef _TASK_H
#define _TASK_H

#define TASKS TASK(PackageQuery)TASK(DoNothing)

#define TASK(x) x,
typedef enum TASKLIST {
    UNK_TASK,
    PackageQuery,
    DirWalk,
    MAX_TASKS
} Task;
#undef TASK

typedef enum TASK_STATUS {
    COMPLETED,
    STARTING,
    PENDING,
    WAITING,
    ABORTED,
    FAILED,
    ERROR,
    INCOMPATIBLE,
    UNEXPECTED_ARGUMENT
} TaskStatus;

extern TaskStatus doTaskV(Task t, int argc, va_list argv);
extern TaskStatus doTaskA(Task t, int argc, ...);

#endif
