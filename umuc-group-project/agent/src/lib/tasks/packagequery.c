#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../logger.h"
#include "dirwalk.h"
#include "packagequery.h"
#include "package.h"
#include "../date.h"
//#include "../http.h"
#include "../hash.h"
#include "package_managers/dpkg.h"
#include "package_managers/apt.h"
#include "package_managers/pacman.h"
#include "package_managers/pip.h"
#include "package_managers/pkg.h"
#include "package_managers/rpm.h"
#include "package_managers/yum.h"

int available_pacmans[END_PACMAN] = { 0 };

static int check_for_package_manager(PackageManager pm)
{
    int ret = 0;

    switch (pm)
    {
        case DPKG:
            if ((ret = system("/usr/bin/dpkg --help")) != 0)
            {
                log(WARN, "Failed to find dpkg. Error code: %d\n", ret);
                break;
            }
            log(INFO, "Found dpkg.\n");
            break;
        //ToDo
        default:
            break;
    }

    return ret;
}

static void determine_available_package_managers()
{
    int i;
    for (i=0; i < END_PACMAN; i++)
    {
        if (available_pacmans[i])
        {
            continue;
        }

        available_pacmans[i] = check_for_package_manager(i);
    }
}

static int query_local(char fullpath[MAX_PATH_LENGTH], PackageManager pm)
{
    switch (pm)
    {
        case DPKG:
            return query_dpkg(fullpath);
        case APT:
            return query_apt(fullpath);
        case PACMAN:
            return query_pacman(fullpath);
        case PIP:
            return query_pip(fullpath);
        case PKG:
            return query_pkg(fullpath);
        case RPM:
            return query_rpm(fullpath);
        case YUM:
            return query_yum(fullpath);
        default:
            return 1;
    }
}

static int query_remote(char fullpath[MAX_PATH_LENGTH], PackageManager pm)
{
    //https://packages.ubuntu.com/search?suite=bionic&arch=any&searchon=contents&keywords=
    return 0;
}

extern int query_packages(char fullpath[MAX_PATH_LENGTH])
{
//    int local[END_PACMAN][END_SUITE][END_ARCH];
//    int remote[END_PACMAN][END_SUITE][END_ARCH];

    if (LOCAL_QUERYING_ENABLED)
    {
        determine_available_package_managers();
    }

    int i;
    for (i=0; i < END_PACMAN; i++)
    {
        if (LOCAL_QUERYING_ENABLED && available_pacmans[i])
        {
            query_local(fullpath, i);
        }

        if (REMOTE_QUERYING_ENABLED)
        {
            query_remote(fullpath, i);
        }
    }

    return 0;
}

extern int packagewalkV(int argc, va_list argv)
{
    if (!dirwalkV(&query_packages, argc, argv))
    {
        return ship_packages();
    }

    return 1;
}

extern int packagewalkA(int argc, ...)
{
    va_list argv;
    va_start(argv, argc);

    int ret = packagewalkV(argc, argv);
    va_end(argv);
    return ret;
}

extern int packagewalk(char* root)
{
    return packagewalkA(1, root);
}
