/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "action.h"
#include "sync.h"
#include "ffi.h"

#include <tsint/error.h>
#include <tsint/exception.h>
#include <tsffi/function.h>
#include <tsffi/error.h>


static void  SignalAction     (void*);
static void  Alert            (void*, unsigned int, char*);
static void  SetExceptionText (void*, char*);
static void* AllocateMemory   (void*, size_t);
static void  FreeMemory       (void*, void*);

static void AddToPending  (struct tsint_action_state*, struct tsint_module_state*);
static void MoveToPending (struct tsint_action_state*, struct tsint_module_state*);
static void RemoveFromAll (struct tsint_action_state*, struct tsint_module_state*);


static struct tsffi_execif tsint_action_execif = {&SignalAction, &Alert, &SetExceptionText, &AllocateMemory, &FreeMemory};


static void  SignalAction (void* user_data)
{
    struct tsint_action_state* action_state;
    struct tsint_module_state* module_state;

    action_state = user_data;
    module_state = action_state->unit_state->module_state;

    TSInt_LockActionStateSync(module_state->sync_data);

    if(
       !(action_state->flags&TSINT_ACTION_STATE_FLAG_SIGNALED) &&
       !(action_state->flags&TSINT_ACTION_STATE_FLAG_FINISHED)
      )
    {
        action_state->flags |= TSINT_ACTION_STATE_FLAG_SIGNALED;

        if(action_state->next_action != NULL)
            action_state->next_action->previous_action = action_state->previous_action;
        if(action_state->previous_action != NULL)
            action_state->previous_action->next_action = action_state->next_action;
        else
            module_state->pending_actions = action_state->next_action;

        action_state->previous_action  = NULL;
        action_state->next_action      = module_state->signaled_actions;
        module_state->signaled_actions = action_state;

        if(action_state->next_action != NULL)
           action_state->next_action->previous_action = action_state;

        TSInt_SignalAction(module_state->sync_data);
    }

    TSInt_UnlockActionStateSync(module_state->sync_data);
}

static void Alert (void* user_data, unsigned int alert_severity, char* text)
{
    struct tsint_action_state* action_state;
    struct tsint_module_state* module_state;

    action_state = user_data;
    module_state = action_state->unit_state->module_state;

    module_state->module_execif->alert(module_state, alert_severity, text);
}

static void SetExceptionText (void* user_data, char* text)
{
    struct tsint_action_state* action_state;
    struct tsint_module_state* module_state;

    action_state = user_data;
    module_state = action_state->unit_state->module_state;

    module_state->module_execif->set_exception_text(module_state, text);
}

static void* AllocateMemory (void* user_data, size_t size)
{
    struct tsint_action_state* action_state;
    struct tsint_module_state* module_state;

    action_state = user_data;
    module_state = action_state->unit_state->module_state;

    return module_state->module_execif->allocate_memory(module_state, size);
}

static void FreeMemory (void* user_data, void* memory)
{
    struct tsint_action_state* action_state;
    struct tsint_module_state* module_state;

    action_state = user_data;
    module_state = action_state->unit_state->module_state;

    module_state->module_execif->free_memory(module_state, memory);
}

static void AddToPending (
                          struct tsint_action_state* action_state,
                          struct tsint_module_state* module_state
                         )
{
    TSInt_LockActionStateSync(module_state->sync_data);

    action_state->flags &= ~TSINT_ACTION_STATE_FLAG_SIGNALED;

    action_state->next_action     = module_state->pending_actions;
    module_state->pending_actions = action_state;
    action_state->previous_action = NULL;

    if(action_state->next_action != NULL)
       action_state->next_action->previous_action = action_state;

    TSInt_UnlockActionStateSync(module_state->sync_data);
}

static void MoveToPending (
                           struct tsint_action_state* action_state,
                           struct tsint_module_state* module_state
                          )
{
    TSInt_LockActionStateSync(module_state->sync_data);

    if(action_state->next_action != NULL)
        action_state->next_action->previous_action = action_state->previous_action;
    if(action_state->previous_action != NULL)
        action_state->previous_action->next_action = action_state->next_action;
    else
    {
        if(action_state->flags&TSINT_ACTION_STATE_FLAG_SIGNALED)
            module_state->signaled_actions = action_state->next_action;
        else
            module_state->pending_actions = action_state->next_action;
    }

    action_state->previous_action = NULL;
    action_state->next_action     = module_state->pending_actions;
    module_state->pending_actions = action_state;

    if(action_state->next_action != NULL)
       action_state->next_action->previous_action = action_state;

    action_state->flags &= ~TSINT_ACTION_STATE_FLAG_SIGNALED;

    TSInt_UnlockActionStateSync(module_state->sync_data);
}

static void RemoveFromAll (
                           struct tsint_action_state* action_state,
                           struct tsint_module_state* module_state
                          )
{
    TSInt_LockActionStateSync(module_state->sync_data);

    if(!(action_state->flags&TSINT_ACTION_STATE_FLAG_FINISHED))
    {
        action_state->flags |= TSINT_ACTION_STATE_FLAG_FINISHED;

        if(action_state->next_action != NULL)
            action_state->next_action->previous_action = action_state->previous_action;
        if(action_state->previous_action != NULL)
            action_state->previous_action->next_action = action_state->next_action;
        else
        {
            if(action_state->flags&TSINT_ACTION_STATE_FLAG_SIGNALED)
                module_state->signaled_actions = action_state->next_action;
            else
                module_state->pending_actions = action_state->next_action;
        }
    }

    TSInt_UnlockActionStateSync(module_state->sync_data);
}


int TSInt_InitActions (
                       struct tsint_unit_state* unit_state,
                       struct tsdef_action**    exception_action
                      )
{
    struct tsdef_unit*                    unit;
    struct tsdef_action*                  current_action;
    struct tsdef_action*                  unwind_action;
    struct tsint_action_state*            action_state;
    struct tsint_module_state*            module_state;
    struct tsdef_function_call_list_node* current_signal;
    struct tsdef_function_call_list_node* unwind_signal;
    void**                                trigger_user_data;
    int                                   exception;

    unit         = unit_state->unit;
    action_state = unit_state->action_state;
    module_state = unit_state->module_state;

    current_action = unit->actions;
    while(current_action != NULL)
    {
        action_state->action     = current_action;
        action_state->unit_state = unit_state;
        action_state->flags      = 0;

        action_state->invocation_data.execif             = &tsint_action_execif;
        action_state->invocation_data.execif_data        = action_state;
        action_state->invocation_data.unit_invocation_id = unit_state->unit_id;
        action_state->invocation_data.unit_name          = unit->name;
        action_state->invocation_data.unit_location      = current_action->location;

        AddToPending(action_state, module_state);

        trigger_user_data = action_state->trigger_user_data;

        for(
            current_signal = current_action->trigger_list->start;
            current_signal != NULL;
            current_signal = current_signal->next_function_call
           )
        {
            struct tsdef_module_object*       module_object;
            struct tsdef_function_call*       function_call;
            struct tsffi_function_definition* ffi_definition;
            struct tsdef_module_ffi_group*    ffi_group;
            union tsffi_value*                ffi_arguments;

            *trigger_user_data = NULL;

            function_call  = current_signal->function_call;
            module_object  = function_call->module_object;
            ffi_definition = module_object->type.ffi.function_definition;
            ffi_group      = module_object->type.ffi.group;

            exception = TSInt_ExpListToFFIArguments(
                                                    function_call->arguments,
                                                    ffi_definition->argument_types,
                                                    unit_state,
                                                    &ffi_arguments
                                                   );
            if(exception != TSINT_EXCEPTION_NONE)
                goto create_arguments_failed;

            exception = ffi_definition->action_controller(
                                                          &action_state->invocation_data,
                                                          TSFFI_INIT_ACTION,
                                                          module_state->ffi_group_data[ffi_group->group_id],
                                                          ffi_arguments,
                                                          NULL,
                                                          trigger_user_data
                                                         );

            TSInt_DestroyFFIArguments(
                                      ffi_definition->argument_types,
                                      ffi_definition->argument_count,
                                      ffi_arguments
                                     );

            if(exception != TSFFI_ERROR_NONE)
            {
                exception = TSINT_EXCEPTION_FFI;

                goto init_action_failed;
            }

            trigger_user_data++;
        }

        unit_state->active_action_count++;
        action_state++;

        current_action = current_action->next_action;
    }

    return TSINT_EXCEPTION_NONE;

init_action_failed:
create_arguments_failed:
    action_state  = unit_state->action_state;
    unwind_action = unit->actions;
    while(1)
    {
        trigger_user_data = action_state->trigger_user_data;

        for(
            unwind_signal = unwind_action->trigger_list->start;
            unwind_signal != NULL && unwind_signal != current_signal;
            unwind_signal = unwind_signal->next_function_call
           )
        {
            struct tsdef_module_object*       module_object;
            struct tsdef_function_call*       function_call;
            struct tsffi_function_definition* ffi_definition;
            struct tsdef_module_ffi_group*    ffi_group;

            function_call  = unwind_signal->function_call;
            module_object  = function_call->module_object;
            ffi_definition = module_object->type.ffi.function_definition;
            ffi_group      = module_object->type.ffi.group;

            ffi_definition->action_controller(
                                              &action_state->invocation_data,
                                              TSFFI_STOP_ACTION,
                                              module_state->ffi_group_data[ffi_group->group_id],
                                              NULL,
                                              NULL,
                                              trigger_user_data
                                             );

            trigger_user_data++;
        }

        RemoveFromAll(action_state, module_state);

        if(unwind_action == current_action)
            break;

        unwind_action = unwind_action->next_action;
        action_state++;
    }

    *exception_action = current_action;

    return exception;
}

int TSInt_EvaluateAction (struct tsint_action_state* action_state, unsigned int* status)
{
    struct tsdef_action*                  action;
    struct tsint_unit_state*              unit_state;
    struct tsint_module_state*            module_state;
    struct tsdef_function_call_list_node* current_signal;
    unsigned int                          triggered_count;
    unsigned int                          finished_count;
    void**                                trigger_user_data;

    action       = action_state->action;
    unit_state   = action_state->unit_state;
    module_state = unit_state->module_state;

    AddToPending(action_state, module_state);

    triggered_count = 0;
    finished_count  = 0;

    trigger_user_data = action_state->trigger_user_data;

    for(
        current_signal = action->trigger_list->start;
        current_signal != NULL;
        current_signal = current_signal->next_function_call
       )
    {
        struct tsdef_module_object*       module_object;
        struct tsdef_function_call*       function_call;
        struct tsffi_function_definition* ffi_definition;
        struct tsdef_module_ffi_group*    ffi_group;
        unsigned int                      state;
        int                               exception;

        function_call  = current_signal->function_call;
        module_object  = function_call->module_object;
        ffi_definition = module_object->type.ffi.function_definition;
        ffi_group      = module_object->type.ffi.group;

        exception = ffi_definition->action_controller(
                                                      &action_state->invocation_data,
                                                      TSFFI_QUERY_ACTION,
                                                      module_state->ffi_group_data[ffi_group->group_id],
                                                      NULL,
                                                      &state,
                                                      trigger_user_data
                                                     );
        if(exception != TSFFI_ERROR_NONE)
            goto query_action_failed;

        switch(state)
        {
        case TSFFI_ACTION_STATE_TRIGGERED:
            triggered_count++;

            break;

        case TSFFI_ACTION_STATE_FINISHED:
            finished_count++;

            break;
        }

        trigger_user_data++;
    }

    if(finished_count == action->trigger_list->count)
    {
        RemoveFromAll(action_state, module_state);

        unit_state->active_action_count--;

        *status = TSINT_ACTION_FINISHED;
    }
    else
    {
        if(triggered_count != 0)
            *status = TSINT_ACTION_TRIGGERED;
        else
            *status = TSINT_ACTION_PENDING;
    }

    return TSINT_EXCEPTION_NONE;

query_action_failed:
    RemoveFromAll(action_state, module_state);

    return TSINT_EXCEPTION_FFI;
}

int TSInt_PrepActionRun (struct tsint_action_state* action_state)
{
    struct tsdef_action*                  action;
    struct tsint_unit_state*              unit_state;
    struct tsint_module_state*            module_state;
    struct tsdef_function_call_list_node* current_signal;
    void**                                trigger_user_data;

    action       = action_state->action;
    unit_state   = action_state->unit_state;
    module_state = unit_state->module_state;

    trigger_user_data = action_state->trigger_user_data;

    for(
        current_signal = action->trigger_list->start;
        current_signal != NULL;
        current_signal = current_signal->next_function_call
       )
    {
        struct tsdef_module_object*       module_object;
        struct tsdef_function_call*       function_call;
        struct tsffi_function_definition* ffi_definition;
        struct tsdef_module_ffi_group*    ffi_group;
        int                               exception;

        function_call  = current_signal->function_call;
        module_object  = function_call->module_object;
        ffi_definition = module_object->type.ffi.function_definition;
        ffi_group      = module_object->type.ffi.group;

        exception = ffi_definition->action_controller(
                                                      &action_state->invocation_data,
                                                      TSFFI_RUNNING_ACTION,
                                                      module_state->ffi_group_data[ffi_group->group_id],
                                                      NULL,
                                                      NULL,
                                                      trigger_user_data
                                                     );
        if(exception != TSFFI_ERROR_NONE)
            goto wait_action_failed;

        trigger_user_data++;
    }

    return TSINT_EXCEPTION_NONE;

wait_action_failed:
    RemoveFromAll(action_state, module_state);

    return TSINT_EXCEPTION_FFI;
}

int TSInt_UpdateAction (struct tsint_action_state* action_state)
{
    struct tsdef_action*                  action;
    struct tsint_unit_state*              unit_state;
    struct tsint_module_state*            module_state;
    struct tsdef_function_call_list_node* current_signal;
    void**                                trigger_user_data;
    int                                   exception;

    action       = action_state->action;
    unit_state   = action_state->unit_state;
    module_state = unit_state->module_state;

    MoveToPending(action_state, module_state);

    trigger_user_data = action_state->trigger_user_data;

    for(
        current_signal = action->trigger_list->start;
        current_signal != NULL;
        current_signal = current_signal->next_function_call
       )
    {
        struct tsdef_module_object*       module_object;
        struct tsdef_function_call*       function_call;
        struct tsffi_function_definition* ffi_definition;
        struct tsdef_module_ffi_group*    ffi_group;
        union tsffi_value*                ffi_arguments;

        function_call  = current_signal->function_call;
        module_object  = function_call->module_object;
        ffi_definition = module_object->type.ffi.function_definition;
        ffi_group      = module_object->type.ffi.group;

        exception = TSInt_ExpListToFFIArguments(
                                                function_call->arguments,
                                                ffi_definition->argument_types,
                                                unit_state,
                                                &ffi_arguments
                                               );
        if(exception != TSINT_EXCEPTION_NONE)
            goto create_arguments_failed;

        exception = ffi_definition->action_controller(
                                                      &action_state->invocation_data,
                                                      TSFFI_UPDATE_ACTION,
                                                      module_state->ffi_group_data[ffi_group->group_id],
                                                      ffi_arguments,
                                                      NULL,
                                                      trigger_user_data
                                                     );

        TSInt_DestroyFFIArguments(
                                  ffi_definition->argument_types,
                                  ffi_definition->argument_count,
                                  ffi_arguments
                                 );

        if(exception != TSFFI_ERROR_NONE)
        {
            exception = TSINT_EXCEPTION_FFI;

            goto update_action_failed;
        }

        trigger_user_data++;
    }

    return TSINT_EXCEPTION_NONE;

update_action_failed:
create_arguments_failed:
    RemoveFromAll(action_state, module_state);

    return exception;
}

void TSInt_StopActions (struct tsint_unit_state* unit_state)
{
    struct tsdef_unit*                    unit;
    struct tsdef_action*                  current_action;
    struct tsint_action_state*            action_state;
    struct tsint_module_state*            module_state;
    struct tsdef_function_call_list_node* current_signal;
    void**                                trigger_user_data;
    int                                   exception;

    unit         = unit_state->unit;
    action_state = unit_state->action_state;
    module_state = unit_state->module_state;

    current_action = unit->actions;
    while(current_action != NULL)
    {
        trigger_user_data = action_state->trigger_user_data;

        for(
            current_signal = current_action->trigger_list->start;
            current_signal != NULL;
            current_signal = current_signal->next_function_call
           )
        {
            struct tsdef_module_object*       module_object;
            struct tsdef_function_call*       function_call;
            struct tsffi_function_definition* ffi_definition;
            struct tsdef_module_ffi_group*    ffi_group;

            function_call  = current_signal->function_call;
            module_object  = function_call->module_object;
            ffi_definition = module_object->type.ffi.function_definition;
            ffi_group      = module_object->type.ffi.group;

            ffi_definition->action_controller(
                                              &action_state->invocation_data,
                                              TSFFI_STOP_ACTION,
                                              module_state->ffi_group_data[ffi_group->group_id],
                                              NULL,
                                              NULL,
                                              trigger_user_data
                                             );

            trigger_user_data++;
        }

        RemoveFromAll(action_state, module_state);

        action_state++;

        current_action = current_action->next_action;
    }
}
