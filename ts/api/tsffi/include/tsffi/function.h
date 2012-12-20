/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSFFI_FUNCTION_H_
#define _TSFFI_FUNCTION_H_


#include <tsffi/primitives.h>
#include <tsffi/execif.h>


#define TSFFI_MAX_INPUT_ARGUMENTS 128

#define TSFFI_BOOL_FALSE 0
#define TSFFI_BOOL_TRUE  1

#define TSFFI_INIT_ACTION    0
#define TSFFI_RUNNING_ACTION 1
#define TSFFI_UPDATE_ACTION  2
#define TSFFI_QUERY_ACTION   3
#define TSFFI_STOP_ACTION    4

#define TSFFI_ACTION_STATE_PENDING   0
#define TSFFI_ACTION_STATE_TRIGGERED 1
#define TSFFI_ACTION_STATE_FINISHED  2


union tsffi_value
{
    tsffi_bool   bool_data;
    tsffi_int    int_data;
    tsffi_real   real_data;
    tsffi_string string_data;
};

struct tsffi_invocation_data
{
    struct tsffi_execif* execif;
    void*                execif_data;

    unsigned int unit_invocation_id;

    char*        unit_name;
    unsigned int unit_location;
};


typedef int (*tsffi_function) (
                               struct tsffi_invocation_data*,
                               void*,
                               union tsffi_value*,
                               union tsffi_value*
                              );

typedef int (*tsffi_action_controller) (
                                        struct tsffi_invocation_data*,
                                        unsigned int,
                                        void*,
                                        union tsffi_value*,
                                        unsigned int*,
                                        void**
                                       );


#endif

