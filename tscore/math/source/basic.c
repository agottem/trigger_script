/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <math/basic.h>

#include <tsffi/error.h>

#include <math.h>


int Math_MinBool (
                  struct tsffi_invocation_data* invocation_data,
                  void*                         group_data,
                  union tsffi_value*            output,
                  union tsffi_value*            input
                 )
{
    tsffi_bool left_val;
    tsffi_bool right_val;

    left_val  = input[0].bool_data;
    right_val = input[1].bool_data;

    if(left_val < right_val)
        output->bool_data = left_val;
    else
        output->bool_data = right_val;

    return TSFFI_ERROR_NONE;
}

int Math_MinInt (
                 struct tsffi_invocation_data* invocation_data,
                 void*                         group_data,
                 union tsffi_value*            output,
                 union tsffi_value*            input
                )
{
    tsffi_int left_val;
    tsffi_int right_val;

    left_val  = input[0].int_data;
    right_val = input[1].int_data;

    if(left_val < right_val)
        output->int_data = left_val;
    else
        output->int_data = right_val;

    return TSFFI_ERROR_NONE;
}

int Math_MinReal (
                  struct tsffi_invocation_data* invocation_data,
                  void*                         group_data,
                  union tsffi_value*            output,
                  union tsffi_value*            input
                 )
{
    tsffi_real left_val;
    tsffi_real right_val;

    left_val  = input[0].real_data;
    right_val = input[1].real_data;

    if(left_val < right_val)
        output->real_data = left_val;
    else
        output->real_data = right_val;

    return TSFFI_ERROR_NONE;
}

int Math_MaxBool (
                  struct tsffi_invocation_data* invocation_data,
                  void*                         group_data,
                  union tsffi_value*            output,
                  union tsffi_value*            input
                 )
{
    tsffi_bool left_val;
    tsffi_bool right_val;

    left_val  = input[0].bool_data;
    right_val = input[1].bool_data;

    if(left_val > right_val)
        output->bool_data = left_val;
    else
        output->bool_data = right_val;

    return TSFFI_ERROR_NONE;
}

int Math_MaxInt (
                 struct tsffi_invocation_data* invocation_data,
                 void*                         group_data,
                 union tsffi_value*            output,
                 union tsffi_value*            input
                )
{
    tsffi_int left_val;
    tsffi_int right_val;

    left_val  = input[0].int_data;
    right_val = input[1].int_data;

    if(left_val > right_val)
        output->int_data = left_val;
    else
        output->int_data = right_val;

    return TSFFI_ERROR_NONE;
}

int Math_MaxReal (
                  struct tsffi_invocation_data* invocation_data,
                  void*                         group_data,
                  union tsffi_value*            output,
                  union tsffi_value*            input
                 )
{
    tsffi_real left_val;
    tsffi_real right_val;

    left_val  = input[0].real_data;
    right_val = input[1].real_data;

    if(left_val > right_val)
        output->real_data = left_val;
    else
        output->real_data = right_val;

    return TSFFI_ERROR_NONE;
}

int Math_Ceil (
               struct tsffi_invocation_data* invocation_data,
               void*                         group_data,
               union tsffi_value*            output,
               union tsffi_value*            input
              )
{
    double val;

    val = (double)input[0].real_data;

    output->int_data = (tsffi_int)ceil(val);

    return TSFFI_ERROR_NONE;
}

int Math_Floor (
                struct tsffi_invocation_data* invocation_data,
                void*                         group_data,
                union tsffi_value*            output,
                union tsffi_value*            input
               )
{
    double val;

    val = (double)input[0].real_data;

    output->int_data = (tsffi_int)floor(val);

    return TSFFI_ERROR_NONE;
}

int Math_Round (
                struct tsffi_invocation_data* invocation_data,
                void*                         group_data,
                union tsffi_value*            output,
                union tsffi_value*            input
               )
{
    double val;

    val = (double)input[0].real_data;

    output->int_data = (tsffi_int)ceil(val-0.5);

    return TSFFI_ERROR_NONE;
}

