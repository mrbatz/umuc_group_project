#ifndef _JSON_H
#define _JSON_H

#include <stddef.h>

typedef enum _JSON_VALUE_TYPE
{
    JSON_VALUE_OBJECT,
    JSON_VALUE_STRING,
    JSON_VALUE_NUMBER

} JSON_VALUE_TYPE;

typedef struct _JSON_VALUE
{
    JSON_VALUE_TYPE type;

    union ValueUnion {

      const char *data;
      void *memberArray;

    } vunion;

    size_t valueLen;

} JSON_VALUE, *PJSON_VALUE;

typedef struct _JSON_OBJECT
{
    const char *string;
    size_t stringLen;
    JSON_VALUE value;

} JSON_OBJECT, *PJSON_OBJECT;

// Exported functions
extern int JSON_Set_Object_String(PJSON_OBJECT targetObject, const char *string, const char *value);
extern int JSON_Set_Object_Number(PJSON_OBJECT targetObject, const char *string, const char *value);
extern int JSON_Set_Child_Object(PJSON_OBJECT targetObject, const char *string, JSON_OBJECT objectArray[], size_t arrayLen);
extern const char* JSON_Get_Structured_Buffer(PJSON_OBJECT objects, size_t arrayLen, size_t *buffSize);

#endif
