/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "expop.h"

#include <tsint/exception.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>


int TSInt_BoolPrimaryExpOp (
                            unsigned int       op,
                            union tsint_value  value1,
                            union tsint_value* value2,
                            union tsint_value* result
                           )
{
    switch(op)
    {
    case TSDEF_PRIMARY_EXP_OP_VALUE:
        result->bool_data = value1.bool_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_ADD:
        if(value1.bool_data == TSDEF_BOOL_TRUE || value2->bool_data == TSDEF_BOOL_TRUE)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_PRIMARY_EXP_OP_SUB:
        if(value1.bool_data != value2->bool_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_PRIMARY_EXP_OP_MUL:
        if(value1.bool_data == TSDEF_BOOL_TRUE && value2->bool_data == TSDEF_BOOL_TRUE)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_PRIMARY_EXP_OP_DIV:
        if(value2->bool_data == TSDEF_BOOL_FALSE)
            return TSINT_EXCEPTION_DIVIDE_BY_ZERO;
        else
        {
            if(value1.bool_data == TSDEF_BOOL_TRUE)
                result->bool_data = TSDEF_BOOL_TRUE;
            else
                result->bool_data = TSDEF_BOOL_FALSE;
        }

        break;

    case TSDEF_PRIMARY_EXP_OP_POW:
        if(value1.bool_data == TSDEF_BOOL_TRUE)
            result->bool_data = TSDEF_BOOL_TRUE;
        else if(value2->bool_data == TSDEF_BOOL_FALSE)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_PRIMARY_EXP_OP_MOD:
        if(value2->bool_data == TSDEF_BOOL_FALSE)
            return TSINT_EXCEPTION_DIVIDE_BY_ZERO;
        else
        {
            if(value1.bool_data == TSDEF_BOOL_TRUE)
                result->bool_data = TSDEF_BOOL_FALSE;
            else
                result->bool_data = TSDEF_BOOL_FALSE;
        }

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_IntPrimaryExpOp (
                           unsigned int       op,
                           union tsint_value  value1,
                           union tsint_value* value2,
                           union tsint_value* result
                          )
{
    switch(op)
    {
    case TSDEF_PRIMARY_EXP_OP_VALUE:
        result->int_data = value1.int_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_ADD:
        result->int_data = value1.int_data+value2->int_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_SUB:
        if(value2 != NULL)
            result->int_data = value1.int_data-value2->int_data;
        else
            result->int_data = -value1.int_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_MUL:
        result->int_data = value1.int_data*value2->int_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_DIV:
        if(value2->int_data == 0)
            return TSINT_EXCEPTION_DIVIDE_BY_ZERO;
        else
            result->int_data = value1.int_data/value2->int_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_POW:
        result->int_data = (tsdef_int)powf(value1.int_data, value2->int_data);

        break;

    case TSDEF_PRIMARY_EXP_OP_MOD:
        if(value2->int_data == 0)
            return TSINT_EXCEPTION_DIVIDE_BY_ZERO;
        else
            result->int_data = value1.int_data%value2->int_data;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_RealPrimaryExpOp (
                            unsigned int       op,
                            union tsint_value  value1,
                            union tsint_value* value2,
                            union tsint_value* result
                           )
{
    double fractional_part;

    switch(op)
    {
    case TSDEF_PRIMARY_EXP_OP_VALUE:
        result->real_data = value1.real_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_ADD:
        result->real_data = value1.real_data+value2->real_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_SUB:
        if(value2 != NULL)
            result->real_data = value1.real_data-value2->real_data;
        else
            result->real_data = -value1.real_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_MUL:
        result->real_data = value1.real_data*value2->real_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_DIV:
        result->real_data = value1.real_data/value2->real_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_POW:
        result->real_data = powf(value1.real_data, value2->real_data);

        break;

    case TSDEF_PRIMARY_EXP_OP_MOD:
        modf(value1.real_data/value2->real_data, &fractional_part);

        result->real_data = (tsdef_real)fractional_part*value2->real_data;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_StringPrimaryExpOp (
                              unsigned int       op,
                              union tsint_value  left_value,
                              union tsint_value* right_value,
                              union tsint_value* result
                             )
{
    char*  result_data;
    size_t left_length;
    size_t right_length;
    size_t result_length;

    switch(op)
    {
    case TSDEF_PRIMARY_EXP_OP_VALUE:
        result->string_data = strdup(left_value.string_data);
        if(result->string_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_PRIMARY_EXP_OP_ADD:
        left_length   = strlen(left_value.string_data);
        right_length  = strlen(right_value->string_data);
        result_length = left_length+right_length+1;

        result_data = malloc(result_length);
        if(result_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        strcpy(result_data, left_value.string_data);
        strcpy(&result_data[left_length], right_value->string_data);

        result->string_data = result_data;

        break;

    case TSDEF_PRIMARY_EXP_OP_SUB:
    case TSDEF_PRIMARY_EXP_OP_MUL:
    case TSDEF_PRIMARY_EXP_OP_DIV:
    case TSDEF_PRIMARY_EXP_OP_POW:
    case TSDEF_PRIMARY_EXP_OP_MOD:
        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_BoolComparisonExpOp  (
                                unsigned int       op,
                                union tsint_value  left_value,
                                union tsint_value  right_value,
                                union tsint_value* result
                               )
{
    switch(op)
    {
    case TSDEF_COMPARISON_EXP_OP_EQUAL:
        if(left_value.bool_data == right_value.bool_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_NOT_EQUAL:
        if(left_value.bool_data != right_value.bool_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER:
        if(left_value.bool_data > right_value.bool_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER_EQUAL:
        if(left_value.bool_data >= right_value.bool_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS:
        if(left_value.bool_data < right_value.bool_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS_EQUAL:
        if(left_value.bool_data <= right_value.bool_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_IntComparisonExpOp  (
                               unsigned int       op,
                               union tsint_value  left_value,
                               union tsint_value  right_value,
                               union tsint_value* result
                              )
{
    switch(op)
    {
    case TSDEF_COMPARISON_EXP_OP_EQUAL:
        if(left_value.int_data == right_value.int_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_NOT_EQUAL:
        if(left_value.int_data != right_value.int_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER:
        if(left_value.int_data > right_value.int_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER_EQUAL:
        if(left_value.int_data >= right_value.int_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS:
        if(left_value.int_data < right_value.int_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS_EQUAL:
        if(left_value.int_data <= right_value.int_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_RealComparisonExpOp  (
                                unsigned int       op,
                                union tsint_value  left_value,
                                union tsint_value  right_value,
                                union tsint_value* result
                               )
{
    switch(op)
    {
    case TSDEF_COMPARISON_EXP_OP_EQUAL:
        if(left_value.real_data == right_value.real_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_NOT_EQUAL:
        if(left_value.real_data != right_value.real_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER:
        if(left_value.real_data > right_value.real_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER_EQUAL:
        if(left_value.real_data >= right_value.real_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS:
        if(left_value.real_data < right_value.real_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS_EQUAL:
        if(left_value.real_data <= right_value.real_data)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_StringComparisonExpOp  (
                                unsigned int       op,
                                union tsint_value  left_value,
                                union tsint_value  right_value,
                                union tsint_value* result
                               )
{
    int delta;

    delta = strcmp(left_value.string_data, right_value.string_data);

    switch(op)
    {
    case TSDEF_COMPARISON_EXP_OP_EQUAL:
        if(delta == 0)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_NOT_EQUAL:
        if(delta != 0)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER:
        if(delta > 0)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_GREATER_EQUAL:
        if(delta >= 0)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS:
        if(delta < 0)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_COMPARISON_EXP_OP_LESS_EQUAL:
        if(delta <= 0)
            result->bool_data = TSDEF_BOOL_TRUE;
        else
            result->bool_data = TSDEF_BOOL_FALSE;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

