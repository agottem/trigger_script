/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsint/module.h>
#include <tsint/error.h>
#include <tsint/exception.h>
#include <tsffi/error.h>

#include "sync.h"
#include "unit.h"
#include "block.h"
#include "statement.h"

#include <stdlib.h>
#include <malloc.h>


static void  Alert            (void*, unsigned int, char*);
static void  SetExceptionText (void*, char*);
static void* AllocateMemory   (void*, size_t);
static void  FreeMemory       (void*, void*);

static int ChangeState (struct tsint_module_state*, unsigned int);


static struct tsffi_execif tsint_module_execif = {NULL, &Alert, &SetExceptionText, &AllocateMemory, &FreeMemory};


static void Alert (void* user_data, unsigned int alert_severity, char* text)
{
    struct tsint_module_state* module_state;
    struct tsint_execif_data*  user_if_data;

    if(user_data == NULL)
        return;

    module_state = user_data;
    user_if_data = module_state->user_execif_data;
    if(user_if_data == NULL)
        return;

    user_if_data->execif->alert(user_if_data->user_data, alert_severity, text);
}

static void SetExceptionText (void* user_data, char* text)
{
    struct tsint_module_state* module_state;
    struct tsint_execif_data*  user_if_data;

    if(user_data == NULL)
        return;

    module_state = user_data;
    user_if_data = module_state->user_execif_data;
    if(user_if_data == NULL)
        return;

    user_if_data->execif->set_exception_text(user_if_data->user_data, text);
}

static void* AllocateMemory (void* user_data, size_t size)
{
    return malloc(size);
}

static void FreeMemory (void* user_data, void* memory)
{
    free(memory);
}

static int ChangeState (
                        struct tsint_module_state* module_state,
                        unsigned int               changed_state
                       )
{
    struct tsdef_module_ffi_group* module_group;
    struct tsdef_module*           module;
    struct tsffi_execif*           execif;
    void**                         ffi_group_data;

    module         = module_state->module;
    execif         = module_state->module_execif;
    ffi_group_data = module_state->ffi_group_data;

    for(
        module_group = module->referenced_ffi_groups;
        module_group != NULL;
        module_group = module_group->next_group
       )
    {
        struct tsffi_registration_group* group;

        group = module_group->group;

        if(group->state_function != NULL)
        {
            int error;

            error = group->state_function(
                                          execif,
                                          module_state,
                                          changed_state,
                                          group,
                                          ffi_group_data[module_group->group_id]
                                         );
            if(error != TSFFI_ERROR_NONE)
                return error;
        }
    }

    return TSFFI_ERROR_NONE;
}


int TSInt_AllocAbortSignal (struct tsint_module_abort_signal** abort_signal)
{
    struct tsint_module_abort_signal* allocated_signal;
    int                               error;

    allocated_signal = malloc(sizeof(struct tsint_module_abort_signal));
    if(allocated_signal == NULL)
        return TSINT_ERROR_MEMORY;

    error = TSInt_InitializeAbortSignal(allocated_signal);
    if(error != TSINT_ERROR_NONE)
        goto initialize_abort_signal_failed;

    *abort_signal = allocated_signal;
    return TSINT_ERROR_NONE;

initialize_abort_signal_failed:
    free(allocated_signal);

    return error;
}

void TSInt_FreeAbortSignal (struct tsint_module_abort_signal* abort_signal)
{
    TSInt_DestroyAbortSignal(abort_signal);
    free(abort_signal);
}

void TSInt_SignalAbort (struct tsint_module_abort_signal* abort_signal)
{
    TSInt_SetAbortSignal(abort_signal);
}

void TSInt_ClearAbort (struct tsint_module_abort_signal* abort_signal)
{
    TSInt_ClearAbortSignal(abort_signal);
}

int TSInt_InterpretModule (
                           struct tsdef_module*              module,
                           union tsint_value*                arguments,
                           union tsint_value*                output,
                           struct tsint_controller_data*     controller_data,
                           struct tsint_execif_data*         execif_data,
                           struct tsint_module_abort_signal* abort_signal
                          )
{
    struct tsint_module_state      state;
    struct tsint_module_sync_data  sync_data;
    struct tsdef_module_ffi_group* module_group;
    struct tsdef_module_ffi_group* unwind_module_group;
    int                            mode;
    int                            error;

    if(controller_data == NULL)
        mode = TSINT_CONTROL_RUN;
    else
        mode = TSINT_CONTROL_STEP_INTO;

    error = TSInt_InitializeSyncData(&sync_data, abort_signal);
    if(error != TSINT_ERROR_NONE)
        goto initialize_sync_data_failed;

    state.module           = module;
    state.controller_data  = controller_data;
    state.user_execif_data = execif_data;
    state.module_execif    = &tsint_module_execif;
    state.sync_data        = &sync_data;
    state.next_unit_id     = 1;
    state.active_units     = NULL;
    state.pending_actions  = NULL;
    state.signaled_actions = NULL;

    if(module->registered_ffi_group_count > 0)
    {
        size_t alloc_size;

        alloc_size           = sizeof(void*)*module->registered_ffi_group_count;
        state.ffi_group_data = malloc(alloc_size);
        if(state.ffi_group_data == NULL)
        {
            error = TSINT_ERROR_MEMORY;

            goto allocate_group_data_failed;
        }

        for(
            module_group = module->referenced_ffi_groups;
            module_group != NULL;
            module_group = module_group->next_group
           )
        {
            struct tsffi_registration_group* group;

            group = module_group->group;

            if(group->begin_function != NULL)
            {
                error = group->begin_function(
                                              state.module_execif,
                                              &state,
                                              group,
                                              &state.ffi_group_data[module_group->group_id]
                                             );
                if(error != TSFFI_ERROR_NONE)
                {
                    error = TSINT_ERROR_BEGIN_GROUP;

                    goto begin_group_failed;
                }
            }
            else
                state.ffi_group_data[module_group->group_id] = NULL;
        }
    }

    error = TSInt_InvokeUnit(module->main_unit, arguments, output, &mode, &state);
    if(error != TSINT_EXCEPTION_NONE)
        goto unit_exception;

    while(state.active_units != NULL)
    {
        struct tsint_action_state* signaled_actions;
        struct tsint_action_state* current_action;

        error = ChangeState(&state, TSFFI_MODULE_SLEEPING);
        if(error != TSFFI_ERROR_NONE)
            goto unit_exception;

        error = TSInt_ListenForAction(state.sync_data);
        if(error != TSINT_EXCEPTION_NONE)
            goto unit_exception;

        error = ChangeState(&state, TSFFI_MODULE_RUNNING);
        if(error != TSFFI_ERROR_NONE)
            goto unit_exception;

        TSInt_LockActionStateSync(&sync_data);
        TSInt_ClearSignal(state.sync_data);

        signaled_actions       = state.signaled_actions;
        state.signaled_actions = NULL;

        TSInt_UnlockActionStateSync(&sync_data);

        while(signaled_actions != NULL)
        {
            current_action   = signaled_actions;
            signaled_actions = signaled_actions->next_action;

            error = TSInt_ProcessUnitAction(current_action, &mode);
            if(error != TSINT_EXCEPTION_NONE)
                goto unit_exception;
        }
    }

    for(
        module_group = module->referenced_ffi_groups;
        module_group != NULL;
        module_group = module_group->next_group
       )
    {
        struct tsffi_registration_group* group;

        group = module_group->group;

        if(group->end_function != NULL)
        {
            group->end_function(
                                state.module_execif,
                                &state,
                                TSFFI_ERROR_NONE,
                                group,
                                state.ffi_group_data[module_group->group_id]
                               );
        }
    }

    if(module->registered_ffi_group_count > 0)
        free(state.ffi_group_data);

    TSInt_DestroySyncData(&sync_data);

    return TSINT_EXCEPTION_NONE;

unit_exception:
    error = TSINT_ERROR_EXCEPTION;

    while(state.active_units != NULL)
        TSInt_StopUnit(state.active_units);

begin_group_failed:
    for(
        unwind_module_group = module->referenced_ffi_groups;
        unwind_module_group != NULL && unwind_module_group != module_group;
        unwind_module_group = unwind_module_group->next_group
       )
    {
        struct tsffi_registration_group* group;

        group = unwind_module_group->group;

        if(group->end_function != NULL)
        {
            group->end_function(
                                state.module_execif,
                                &state,
                                TSFFI_ERROR_EXCEPTION,
                                group,
                                state.ffi_group_data[unwind_module_group->group_id]
                               );
        }
    }

    if(module->registered_ffi_group_count > 0)
        free(state.ffi_group_data);

allocate_group_data_failed:
    TSInt_DestroySyncData(&sync_data);

initialize_sync_data_failed:
    return error;
}

