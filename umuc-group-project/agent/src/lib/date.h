#ifndef _DATE_H
#define _DATE_H

#include <time.h>

#define DATE_SIZE 22

typedef struct tm Date;

extern int new_date(Date* tm);
extern int get_timestamp(char timestamp[DATE_SIZE]);

#endif
