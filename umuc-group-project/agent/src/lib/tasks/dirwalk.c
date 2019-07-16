#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../logger.h"
#include "../tasking.h"
#include "dirwalk.h"

int depth = 0;

static int is_file(char filepath[255])
{
    return 0;
}

static int included_path(char actual_path[PATH_MAX], int argc, char** all_dirs) //recursion loop prevention
{
    int i;
    for (i=0; i < argc; i++)
    {
        int len = strnlen(actual_path, PATH_MAX);
        int len2 = strnlen(all_dirs[i], MAX_PATH_LENGTH);
        int shortest = (len < len2) ? len : len2;

        char dir[len2+2];
        strncpy(dir, all_dirs[i], len2+1);
        if (!(dir[len2-2] == '/'))
        {
            dir[len2-1] = '/';
            shortest++;
        }

        if (strncmp(actual_path, dir, shortest) == 0)
        {
            return 1;
        }
    }

    return 0;
}

static long walk_dirs(WalkFunction ptr, int argc, char** all_dirs, char* root)
{
    DIR *d;
    struct dirent *dir;
    struct stat sb;
    char fullpath[MAX_PATH_LENGTH];
    long total = 0;
    
    if ((d = opendir(root)) == NULL) {
        return COMPLETED;
    }

    while ((dir = readdir(d)))
    {
        if (strncmp(dir->d_name, ".", 2) == 0 || strncmp(dir->d_name, "..", 3) == 0)
        {
            continue;
        }

        sprintf(fullpath, "%s/%s", root, dir->d_name);
        if (lstat(fullpath, &sb) == -1)
        {
            continue;
        }

        if ((sb.st_mode & S_IFMT) == S_IFLNK)
        {
            char actual_path[PATH_MAX] = { '\0' };
            realpath(fullpath, actual_path);
            if (included_path(actual_path, argc, all_dirs))
            {
                continue;
            }
        }

        if ((sb.st_mode & S_IFMT) == S_IFDIR)
        {
            depth++;
            total += walk_dirs(ptr, argc, all_dirs, fullpath);
            depth--;
        }
        else
        {
            if (!ptr(fullpath))
            {
                if (FULLPATH_LOGGING)
                {
                    log(INFO, "Found: '%s'", fullpath);
                }
                else if (TREE_LOGGING)
                {
                    char depth_rep[depth+2];
                    int i;
                    for (i=0; i < depth; i++)
                    {
                        depth_rep[i] = '-';
                    }
                    depth_rep[depth] = '>';
                    depth_rep[depth+1] = '\0';
                    log(INFO, "%s %s", depth_rep, dir->d_name);
                }
    
                total++;
            }
        }
    }
    closedir(d);
    return total;
    
}

extern TaskStatus dirwalkV(WalkFunction ptr, int argc, va_list argv)
{
    long total = 0;

    char* root[argc];
    int i;
    for (i=0; i < argc; i++)
    {
        root[i] = va_arg(argv, char*);
    }

    for (i=0; i < argc; i++)
    {
        total = walk_dirs(ptr, argc, root, root[i]);
    }

    log(INFO, "Found %ld total files.", total);

    return COMPLETED;
}

extern TaskStatus dirwalkA(WalkFunction ptr, int argc, ...)
{
    va_list argv;
    va_start(argv, argc);
    TaskStatus ret = dirwalkV(ptr, argc, argv);
    va_end(argv);
    return ret;
}

extern TaskStatus dirwalk(WalkFunction ptr, char* root)
{
    return dirwalkA(ptr, 1, root);
}

extern TaskStatus default_dirwalkV(int argc, va_list argv)
{
    return dirwalkV(&is_file, argc, argv);
}

extern TaskStatus default_dirwalkA(int argc, ...)
{
    va_list argv;
    va_start(argv, argc);
    TaskStatus ret = default_dirwalkV(argc, argv);
    va_end(argv);
    return ret;
}

extern TaskStatus default_dirwalk(char* root)
{
    return default_dirwalkA(1, root);
}
