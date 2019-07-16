#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "date.h"
#include "logger.h"

extern int new_date(Date* tm) //struct tm* aliased to Date
{
    time_t ltime;
    ltime = time(NULL);
    Date* ltm = localtime(&ltime);
    if (ltm == NULL)
    {
        return 1;
    }

    memcpy(tm, ltm, sizeof(*ltm));

    return 0;
}

extern int get_timestamp(char timestamp[DATE_SIZE])
{
    Date tm;
    if (new_date(&tm))
    {
        return 1;
    }
    sprintf(timestamp, "%04d-%02d-%02d - %02d:%02d:%02d", tm.tm_year+1900, tm.tm_mon, 
        tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return 0;
}
