/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsint/value.h>
#include <tsint/error.h>

#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define MAX_CONVERSION_STRING_LENGTH (3+DBL_MANT_DIG-DBL_MIN_EXP)


void TSInt_DestroyValue (union tsint_value value, unsigned int type)
{
    switch(type)
    {
    case TSDEF_PRIMITIVE_TYPE_BOOL:
    case TSDEF_PRIMITIVE_TYPE_INT:
    case TSDEF_PRIMITIVE_TYPE_REAL:
        break;

    case TSDEF_PRIMITIVE_TYPE_STRING:
        free(value.string_data);

        break;
    }
}

int TSInt_ConvertValue (
                        union tsint_value  value,
                        unsigned int       from_type,
                        unsigned int       to_type,
                        union tsint_value* result
                       )
{
    char converted_string[MAX_CONVERSION_STRING_LENGTH+1];

    switch(from_type)
    {
    case TSDEF_PRIMITIVE_TYPE_BOOL:
        switch(to_type)
        {
        case TSDEF_PRIMITIVE_TYPE_BOOL:
            result->bool_data = value.bool_data;

            break;

        case TSDEF_PRIMITIVE_TYPE_INT:
            result->int_data = (tsdef_int)value.bool_data;

            break;

        case TSDEF_PRIMITIVE_TYPE_REAL:
            result->real_data = (tsdef_real)value.bool_data;

            break;

        case TSDEF_PRIMITIVE_TYPE_STRING:
            if(value.bool_data == TSDEF_BOOL_TRUE)
                result->string_data = strdup(TSDEF_BOOL_TRUE_STRING);
            else
                result->string_data = strdup(TSDEF_BOOL_FALSE_STRING);

            if(result->string_data == NULL)
                return TSINT_ERROR_MEMORY;

            break;
        }

        break;

    case TSDEF_PRIMITIVE_TYPE_INT:
        switch(to_type)
        {
        case TSDEF_PRIMITIVE_TYPE_BOOL:
            if(value.int_data != 0)
                result->int_data = TSDEF_BOOL_TRUE;
            else
                result->int_data = TSDEF_BOOL_FALSE;

            break;

        case TSDEF_PRIMITIVE_TYPE_INT:
            result->int_data = value.int_data;

            break;

        case TSDEF_PRIMITIVE_TYPE_REAL:
            result->real_data = (tsdef_real)value.int_data;

            break;

        case TSDEF_PRIMITIVE_TYPE_STRING:
            sprintf(converted_string, "%d", value.int_data);

            result->string_data = strdup(converted_string);
            if(result->string_data == NULL)
                return TSINT_ERROR_MEMORY;

            break;
        }

        break;

    case TSDEF_PRIMITIVE_TYPE_REAL:
        switch(to_type)
        {
        case TSDEF_PRIMITIVE_TYPE_BOOL:
            if(value.real_data != 0)
                result->real_data = TSDEF_BOOL_TRUE;
            else
                result->real_data = TSDEF_BOOL_FALSE;

            break;

        case TSDEF_PRIMITIVE_TYPE_INT:
            result->int_data = (tsdef_int)value.real_data;

            break;

        case TSDEF_PRIMITIVE_TYPE_REAL:
            result->real_data = value.real_data;

            break;

        case TSDEF_PRIMITIVE_TYPE_STRING:
            sprintf(converted_string, "%f", value.real_data);

            result->string_data = strdup(converted_string);
            if(result->string_data == NULL)
                return TSINT_ERROR_MEMORY;

            break;
        }

        break;

    case TSDEF_PRIMITIVE_TYPE_STRING:
        switch(to_type)
        {
        case TSDEF_PRIMITIVE_TYPE_BOOL:
            if(strcmp(value.string_data, TSDEF_BOOL_TRUE_STRING) == 0)
                result->real_data = TSDEF_BOOL_TRUE;
            else
                result->real_data = TSDEF_BOOL_FALSE;

            break;

        case TSDEF_PRIMITIVE_TYPE_INT:
            result->int_data = (tsdef_int)atoi(value.string_data);

            break;

        case TSDEF_PRIMITIVE_TYPE_REAL:
            result->real_data = (tsdef_real)atof(value.string_data);

            break;

        case TSDEF_PRIMITIVE_TYPE_STRING:
            result->string_data = strdup(value.string_data);
            if(result->string_data == NULL)
                return TSINT_ERROR_MEMORY;

            break;
        }

        break;
    }

    return TSINT_ERROR_NONE;
}

