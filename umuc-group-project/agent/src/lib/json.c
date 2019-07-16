#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "json.h"

static size_t Get_Buffer_Size(PJSON_OBJECT objects, size_t depth, size_t arrayLen);
static void Fill_Buffer(PJSON_OBJECT object, char **buffer, size_t depth, size_t arrayLen);

int JSON_Set_Object_String(PJSON_OBJECT targetObject, const char *string, const char *value)
{
    if (targetObject == NULL) { return -1; }
    if (string == NULL) { return -1; }
    if (value == NULL) { return -1; }

    targetObject->string = string;
    targetObject->stringLen = strlen(string);

    PJSON_VALUE targetValue = &targetObject->value;

    targetValue->type = JSON_VALUE_STRING;
    targetValue->vunion.data = value;
    targetValue->valueLen = strlen(value);

    return 0;
}

int JSON_Set_Object_Number(PJSON_OBJECT targetObject, const char *string, const char *value)
{
    if (targetObject == NULL) { return -1; }
    if (string == NULL) { return -1; }
    if (value == NULL) { return -1; }

    targetObject->string = string;
    targetObject->stringLen = strlen(string);

    PJSON_VALUE targetValue = &targetObject->value;

    targetValue->type = JSON_VALUE_NUMBER;
    targetValue->vunion.data = value;
    targetValue->valueLen = strlen(value);

    return 0;
}

int JSON_Set_Child_Object(PJSON_OBJECT targetObject, const char *string, JSON_OBJECT objectArray[], size_t arrayLen)
{
    if (targetObject == NULL) { return -1; }
    if (string == NULL) { return -1; }
    if (objectArray == NULL) { return -1; }

    targetObject->string = string;
    targetObject->stringLen = strlen(string);

    PJSON_VALUE targetValue = &targetObject->value;

    targetValue->type = JSON_VALUE_OBJECT;
    targetValue->vunion.memberArray = (void *)objectArray;
    targetValue->valueLen = arrayLen;

    return 0;
}

const char* JSON_Get_Structured_Buffer(PJSON_OBJECT objects, size_t arrayLen, size_t *buffSize)
{
    size_t bufferSize = 0;
    char *buffer, *beginning = NULL;

    bufferSize = Get_Buffer_Size(objects, 1, arrayLen) + 3;

    *buffSize = bufferSize;

    buffer = (char *)malloc(sizeof(char) * bufferSize);

    beginning = buffer;

    strncpy(buffer, "{\n", 2);
    buffer += 2;

    Fill_Buffer(objects, &buffer, 1, arrayLen);

    strncpy(buffer, "}", 1);

    return beginning;
}

static size_t Get_Buffer_Size(PJSON_OBJECT objects, size_t depth, size_t arrayLen)
{
    size_t size = 0;

    for (int x = 0; x < arrayLen; x++)
    {
        size += objects->stringLen;

        PJSON_VALUE value = &objects->value;

        switch (value->type)
        {
            case JSON_VALUE_OBJECT :

                size += Get_Buffer_Size(value->vunion.memberArray, depth + 1, value->valueLen) + depth + 3;
                break;

            case JSON_VALUE_STRING :

                size += value->valueLen + 2;
                break;

            case JSON_VALUE_NUMBER :

                size += value->valueLen;
                break;

            default :

                break;
        }

        if ((x + 1) != arrayLen)
        {
            size += depth + 7;
        }

        else
        {
            size += depth + 6;
        }


        objects++;
    }

    return size;
}

static void Fill_Buffer(PJSON_OBJECT object, char **buffer, size_t depth, size_t arrayLen)
{
    for (int x = 0; x < arrayLen; x++)
    {
        memset(*buffer, '\t', depth);
        (*buffer) += depth;

        strncpy(*buffer, "\"", 1);
        (*buffer)++;

        strncpy(*buffer, object->string, object->stringLen);
        (*buffer) += object->stringLen;

        strncpy(*buffer, "\"", 1);
        (*buffer)++;

        strncpy(*buffer, " : ", 3);
        (*buffer) += 3;

        PJSON_VALUE value = &object->value;

        switch (value->type)
        {
            case JSON_VALUE_OBJECT :

                strncpy(*buffer, "{\n", 2);
                (*buffer) += 2;

                Fill_Buffer((PJSON_OBJECT)value->vunion.memberArray, buffer, depth + 1, value->valueLen);

                memset(*buffer, '\t', depth);
                (*buffer) += depth;

                strncpy(*buffer, "}", 1);
                (*buffer)++;

                break;

            case JSON_VALUE_STRING :

                strncpy(*buffer, "\"", 1);
                (*buffer)++;
                strncpy(*buffer, (const char*)value->vunion.data, value->valueLen);
                (*buffer) += value->valueLen;
                strncpy(*buffer, "\"", 1);
                (*buffer)++;

                break;

            case JSON_VALUE_NUMBER :

                strncpy(*buffer, (const char*)value->vunion.data, value->valueLen);
                (*buffer) += value->valueLen;

                break;

            default :

                break;
        }

        if ((x + 1) != arrayLen)
        {
            strncpy(*buffer, ",", 1);
            (*buffer)++;
        }

        strncpy(*buffer, "\n", 1);
        (*buffer)++;

        object++;
    }
}
