#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../logger.h"
#include "../packagequery.h"
#include "../package.h"
#include "../dirwalk.h"
#include "dpkg.h"

#define CCMD(x) \
    if (*x == '\0') \
    { \
        log(ERR, "Empty response for %s", #x); \
        return 1; \
    }

extern int query_dpkg(char fullpath[MAX_PATH_LENGTH])
{
    Package* p = NULL;
    FILE* stream = NULL;
    char command[MAX_COMMAND_LENGTH] = { '\0' };
    sprintf(command, "dpkg -S \"%s\"", fullpath);
    if ((stream = popen(command, "r")) == NULL)
    {
        return 1;
    }
    

    char package_name[MAX_PACKAGE_NAME] = { '\0' };
    int i = 0;
    char c = '\0';
    while ((c = fgetc(stream)) != EOF && c != ':' && i < MAX_PACKAGE_NAME)
    {
        package_name[i++] = c;
    }
    pclose(stream);

    CCMD(package_name);

    sprintf(command, "dpkg -l \"%s\"", package_name);
    if ((stream = popen(command, "r")) == NULL)
    {
        return 1;
    }

    int package_found = 0;
    char package_version[MAX_VERSION_LENGTH] = { '\0' };
    char package_arch[10] = { '\0' };
    i = 0;

    char command_output[8196] = { '\0' };
    fread(command_output, sizeof(char), 8196, stream); //have to make it searchable
    pclose(stream);

    CCMD(command_output);

    int len = strlen(package_name);
    int j = 0;
    for (i=0; i < 8196; i++)
    {
        if (!package_found && command_output[i] == package_name[0])
        {
            char check[len];
            check[len-1] = '\0';
            for (j=0; j < len; j++)
            {
                check[j] = command_output[j+i];
            }


            if (strncmp(package_name, check, len) == 0)
            {
                package_found = 1;
                i += len;
            }
        }

        if (!package_found)
        {
            continue;
        }
        else if (isspace(command_output[i]))
        {
            continue;
        }
        else if (command_output[i] == '\0')
        {
            break;
        }

        if (package_version[0] == '\0')
        {
            j = 0;
            c = command_output[i];
            while (!isspace(c) && c != '\0' && j < MAX_VERSION_LENGTH)
            {
                package_version[j++] = c;
                c = command_output[++i];
            }

            while (isspace(c) && c != '\0')
            {
                c = command_output[++i];
            }

            j = 0;
            while (!isspace(c) && c != '\0' && j < 10)
            {
                package_arch[j++] = c;
                c = command_output[++i];
            }
            break;
        }
    }
        
    if (strncmp(package_arch, "all", sizeof("all")) == 0)
    {
        i = 0;
        sprintf(command, "dpkg --print-architecture");
        if ((stream = popen(command, "r")) == NULL)
        {
            return 1;
        }
        
        while ((c = fgetc(stream)) != EOF && !isspace(c))
        {
            package_arch[i++] = c;
        }
        pclose(stream);
    }

    Suite su = UNK_SUITE;
    Arch ar = determine_arch(package_arch);

    p = new_package(fullpath, package_name, package_version, DPKG, su, ar);
    if (p == NULL)
    {
        log(ERR, "Failed to create new package from \n\tFile: %s\n\tPackage: %s\n\tVersion: %s\n\tArch: %s", fullpath, package_name, package_version, package_arch);
        return 1;
    }

    sprintf(command, "dpkg -L \"%s\"", package_name);
    if ((stream = popen(command, "r")) != NULL)
    {
        char other_file[MAX_PATH_LENGTH] = { '\0' };
        i = 0;
        while ((c = fgetc(stream)) != EOF)
        {
            if (isspace(c))
            {
                if (other_file[0] != '\0')
                {
                    if (add_file(p, other_file))
                    {
                        break;
                    }
                }

                while (i > 0)
                {
                    other_file[i--]  = '\0';
                }

                continue;
            }

            other_file[i++] = c;
        }
    }
    pclose(stream);

    if (rehash_package(p))
    {
        log(ERR, "Something went wrong.");
        destroy_package(p);
        return 1;
    }

    if (update_package(p))
    {
        destroy_package(p);
        return 1;
    }

    return 0;
}
