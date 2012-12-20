/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_MODULE_H_
#define _TSINT_MODULE_H_


#include <tsdef/def.h>
#include <tsdef/module.h>
#include <tsint/controller.h>
#include <tsint/value.h>
#include <tsint/variable.h>
#include <tsint/execif.h>
#include <tsffi/execif.h>


#define TSINT_ACTION_STATE_FLAG_SIGNALED 0x01
#define TSINT_ACTION_STATE_FLAG_FINISHED 0x02

#define TSINT_UNIT_STATE_FLAG_FINISH 0x01


struct tsint_execution_stack
{
    struct tsdef_statement* return_statement;
    struct tsdef_block*     return_block;

    struct tsint_variable* variables;

    union statement_data
    {
        struct
        {
            union tsint_value to_value;
            union tsint_value step_value;

            unsigned int comparison_op;
            unsigned int step_op;
            unsigned int flags;
        }for_loop;
    }statement_data;
};

struct tsint_action_state
{
    struct tsdef_action*     action;
    struct tsint_unit_state* unit_state;

    unsigned int flags;

    struct tsffi_invocation_data invocation_data;
    void**                       trigger_user_data;

    struct tsint_action_state* next_action;
    struct tsint_action_state* previous_action;
};

struct tsint_unit_state
{
    struct tsdef_unit* unit;

    struct tsdef_statement* current_statement;
    struct tsdef_block*     current_block;
    unsigned int            current_location;

    struct tsint_execution_stack* execution_stack;
    unsigned int                  execution_stack_depth;
    unsigned int                  current_execution_depth;

    unsigned int unit_id;
    unsigned int flags;

    struct tsint_module_state* module_state;

    int mode;
    int exception;

    unsigned int active_action_count;

    struct tsint_unit_state* next_active_unit;
    struct tsint_unit_state* previous_active_unit;

    void**                    trigger_user_data;
    struct tsint_action_state action_state[];
};

struct tsint_module_state
{
    struct tsdef_module*              module;
    struct tsint_controller_data*     controller_data;
    struct tsint_execif_data*         user_execif_data;
    struct tsffi_execif*              module_execif;

    struct tsint_module_sync_data* sync_data;

    unsigned int next_unit_id;

    void** ffi_group_data;

    struct tsint_unit_state* active_units;

    struct tsint_action_state* pending_actions;
    struct tsint_action_state* signaled_actions;
};

struct tsint_module_abort_signal;


extern int  TSInt_AllocAbortSignal (struct tsint_module_abort_signal**);
extern void TSInt_FreeAbortSignal  (struct tsint_module_abort_signal*);

extern void TSInt_SignalAbort (struct tsint_module_abort_signal*);
extern void TSInt_ClearAbort  (struct tsint_module_abort_signal*);

extern int TSInt_InterpretModule (
                                  struct tsdef_module*,
                                  union tsint_value*,
                                  union tsint_value*,
                                  struct tsint_controller_data*,
                                  struct tsint_execif_data*,
                                  struct tsint_module_abort_signal*
                                 );


#endif

