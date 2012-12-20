/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "unit.h"
#include "assignment.h"
#include "expeval.h"
#include "block.h"
#include "statement.h"
#include "action.h"
#include "sync.h"

#include <tsint/error.h>
#include <tsint/exception.h>
#include <tsint/variable.h>
#include <tsint/value.h>

#include <malloc.h>
#include <stdlib.h>
#include <string.h>


static int RunBlock (
                     struct tsint_unit_state*,
                     struct tsdef_statement*,
                     struct tsint_controller_data*
                    );


static int RunBlock (
                     struct tsint_unit_state*      unit_state,
                     struct tsdef_statement*       statement,
                     struct tsint_controller_data* controller_data
                    )
{
    struct tsint_module_sync_data* sync_data;

    sync_data = unit_state->module_state->sync_data;

    while(statement != NULL)
    {
        int exception;

        unit_state->current_location  = statement->location;
        unit_state->current_statement = statement;

        if(controller_data != NULL && unit_state->mode != TSINT_CONTROL_RUN)
        {
            unit_state->mode = controller_data->function(
                                                         unit_state,
                                                         controller_data->user_data
                                                        );
            if(unit_state->mode == TSINT_CONTROL_HALT)
            {
                unit_state->exception = TSINT_EXCEPTION_HALT;

                goto exception_encountered;
            }
        }

        unit_state->exception = TSInt_TestAbortSignal(sync_data);
        if(unit_state->exception != TSINT_EXCEPTION_NONE)
        {
            unit_state->mode = TSINT_CONTROL_HALT;

            goto exception_encountered;
        }

        unit_state->exception = TSInt_ProcessStatement(&statement, unit_state);
        if(unit_state->exception != TSINT_EXCEPTION_NONE)
            goto exception_encountered;
    }

    return TSINT_EXCEPTION_NONE;

exception_encountered:
    return unit_state->exception;
}


int TSInt_ControlModeForInvokedUnit (int mode)
{
    switch(mode)
    {
    case TSINT_CONTROL_RUN:
    case TSINT_CONTROL_STEP:
        mode = TSINT_CONTROL_RUN;

        break;

    case TSINT_CONTROL_STEP_INTO:
        mode = TSINT_CONTROL_STEP_INTO;

        break;

    case TSINT_CONTROL_HALT:
        mode = TSINT_CONTROL_HALT;

        break;
    }

    return mode;
}

int TSInt_ExpListToValueArray (
                               struct tsdef_exp_list*   exp_list,
                               struct tsdef_input*      input,
                               struct tsint_unit_state* state,
                               union tsint_value**      created_value_array
                              )
{
    union tsint_value*               value_array;
    union tsint_value*               original_value_array;
    struct tsdef_exp_list_node*      exp_node;
    struct tsdef_variable_list_node* input_node;
    struct tsdef_variable_list_node* unwind_node;
    size_t                           alloc_size;
    int                              exception;

    if(exp_list == NULL)
    {
        *created_value_array = NULL;

        return TSINT_EXCEPTION_NONE;
    }

    alloc_size = sizeof(union tsint_value)*exp_list->count;
    value_array = malloc(alloc_size);
    if(value_array == NULL)
        return TSINT_EXCEPTION_OUT_OF_MEMORY;

    original_value_array = value_array;

    input_node = input->input_variables->start;
    for(exp_node = exp_list->start; exp_node != NULL; exp_node = exp_node->next_exp)
    {
        exception = TSInt_EvaluateExp(
                                      input_node->variable->variable->primitive_type,
                                      exp_node->exp,
                                      state,
                                      value_array
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            goto evaluate_exp_failed;

        value_array++;
        input_node = input_node->next_variable;
    }

    *created_value_array = original_value_array;

    return TSINT_EXCEPTION_NONE;

evaluate_exp_failed:
    value_array = original_value_array;

    for(
        unwind_node = input->input_variables->start;
        unwind_node != input_node;
        unwind_node = unwind_node->next_variable
       )
    {
        TSInt_DestroyValue(*value_array, unwind_node->variable->variable->primitive_type);

        value_array++;
    }

    free(original_value_array);

    return exception;
}

void TSInt_DestroyValueArray (struct tsdef_input* input, union tsint_value* value_array)
{
    struct tsdef_variable_list_node* input_node;
    union tsint_value*               current_value;

    if(value_array == NULL)
        return;

    current_value = value_array;
    for(
        input_node = input->input_variables->start;
        input_node != NULL;
        input_node = input_node->next_variable
       )
    {
        TSInt_DestroyValue(*current_value, input_node->variable->variable->primitive_type);

        current_value++;
    }

    free(value_array);
}

int TSInt_InvokeUnit (
                      struct tsdef_unit*         unit,
                      union tsint_value*         arguments,
                      union tsint_value*         output_value,
                      int*                       mode,
                      struct tsint_module_state* module_state
                     )
{
    struct tsint_unit_state*         unit_state;
    struct tsint_controller_data*    controller_data;
    struct tsdef_block*              block;
    struct tsdef_statement*          statement;
    struct tsdef_input*              input;
    struct tsdef_variable_list_node* input_node;
    struct tsdef_output*             output;
    struct tsdef_action*             action;
    void**                           trigger_user_data;
    size_t                           alloc_size;
    int                              exception;

    alloc_size = sizeof(struct tsint_unit_state)+sizeof(struct tsint_action_state)*unit->action_count;
    unit_state = malloc(alloc_size);
    if(unit_state == NULL)
        goto allocate_unit_state_failed;

    alloc_size = 0;
    for(action = unit->actions; action != NULL; action = action->next_action)
        alloc_size += sizeof(void*)*action->trigger_list->count;

    if(alloc_size != 0)
    {
        struct tsint_action_state* action_state;
        unsigned int               data_offset;

        trigger_user_data = malloc(alloc_size);
        if(trigger_user_data == NULL)
            goto allocate_trigger_user_data_failed;

        unit_state->trigger_user_data = trigger_user_data;

        action_state = unit_state->action_state;
        data_offset  = 0;
        for(action = unit->actions; action != NULL; action = action->next_action)
        {
            action_state->trigger_user_data = &trigger_user_data[data_offset];

            data_offset += action->trigger_list->count;
            action_state++;
        }
    }
    else
        trigger_user_data = NULL;

    block = &unit->global_block;

    unit_state->unit                  = unit;
    unit_state->current_statement     = NULL;
    unit_state->current_block         = NULL;
    unit_state->current_location      = 0;
    unit_state->execution_stack       = NULL;
    unit_state->execution_stack_depth = 0;
    unit_state->unit_id               = module_state->next_unit_id;
    unit_state->flags                 = 0;
    unit_state->module_state          = module_state;
    unit_state->mode                  = *mode;
    unit_state->exception             = TSINT_EXCEPTION_NONE;

    module_state->next_unit_id++;

    controller_data = module_state->controller_data;

    unit_state->exception = TSInt_StartBlock(block, NULL, unit_state, &statement);
    if(unit_state->exception != TSINT_EXCEPTION_NONE)
        goto exception_encountered;

    input = unit->input;
    if(input != NULL)
    {
        union tsint_value* current_argument;

        unit_state->current_location = input->location;

        current_argument = arguments;
        for(
            input_node = input->input_variables->start;
            input_node != NULL;
            input_node = input_node->next_variable
           )
        {
            struct tsint_variable* variable;
            struct tsdef_variable* variable_def;

            variable_def = input_node->variable->variable;
            variable     = TSInt_LookupVariableAddress(input_node->variable->variable, unit_state);

            if(variable_def->primitive_type == TSDEF_PRIMITIVE_TYPE_STRING)
            {
                variable->value.string_data = strdup(current_argument->string_data);
                if(variable->value.string_data == NULL)
                {
                    unit_state->exception = TSINT_EXCEPTION_OUT_OF_MEMORY;

                    goto exception_encountered;
                }
            }
            else
                variable->value = *current_argument;

            variable->flags |= TSINT_VARIABLE_FLAG_INITIALIZED;

            current_argument++;
        }
    }

    output = unit->output;
    if(output != NULL)
    {
        unit_state->current_location = output->location;
        unit_state->exception        = TSInt_PerformAssignment(
                                                               output->output_variable_assignment,
                                                               unit_state
                                                              );
        if(unit_state->exception != TSINT_EXCEPTION_NONE)
            goto exception_encountered;
    }

    unit_state->exception = RunBlock(unit_state, statement, controller_data);
    if(unit_state->exception != TSINT_EXCEPTION_NONE)
        goto exception_encountered;

    if(output != NULL && output_value != NULL)
    {
        struct tsint_variable* variable;
        struct tsdef_variable* variable_def;

        variable_def = output->output_variable_assignment->lvalue->variable;

        variable = TSInt_LookupVariableAddress(
                                               variable_def,
                                               unit_state
                                              );

        if(variable_def->primitive_type == TSDEF_PRIMITIVE_TYPE_STRING)
        {
            output_value->string_data = strdup(variable->value.string_data);
            if(output_value->string_data == NULL)
            {
                unit_state->exception = TSINT_EXCEPTION_OUT_OF_MEMORY;

                goto exception_encountered;
            }
        }
        else
            *output_value = variable->value;
    }

    *mode = unit_state->mode;

    if(unit->actions != NULL && !(unit_state->flags&TSINT_UNIT_STATE_FLAG_FINISH))
    {
        struct tsdef_action* exception_action;

        unit_state->active_action_count = 0;

        unit_state->exception = TSInt_InitActions(unit_state, &exception_action);
        if(unit_state->exception != TSINT_EXCEPTION_NONE)
        {
            unit_state->current_location = exception_action->location;

            goto exception_encountered;
        }

        unit_state->next_active_unit     = module_state->active_units;
        unit_state->previous_active_unit = NULL;
        if(unit_state->next_active_unit != NULL)
            unit_state->next_active_unit->previous_active_unit = unit_state;

        module_state->active_units = unit_state;
    }
    else
    {
        TSInt_FinishBlock(unit_state, &statement);

        if(trigger_user_data != NULL)
            free(trigger_user_data);

        free(unit_state);
    }

    return TSINT_EXCEPTION_NONE;

exception_encountered:
    if(unit_state->mode != TSINT_CONTROL_HALT && controller_data != NULL)
    {
        controller_data->function(
                                  unit_state,
                                  controller_data->user_data
                                 );
        unit_state->mode = TSINT_CONTROL_HALT;
    }

    while(unit_state->execution_stack_depth > 0)
        TSInt_FinishBlock(unit_state, &statement);

    *mode     = unit_state->mode;
    exception = unit_state->exception;

    if(trigger_user_data != NULL)
        free(trigger_user_data);

    free(unit_state);

    return exception;

allocate_trigger_user_data_failed:
    free(unit_state);

allocate_unit_state_failed:
    return TSINT_EXCEPTION_OUT_OF_MEMORY;
}

int TSInt_ProcessUnitAction (struct tsint_action_state* action_state, int* mode)
{
    struct tsdef_block*           block;
    struct tsdef_action*          action;
    struct tsdef_statement*       statement;
    struct tsint_controller_data* controller_data;
    struct tsint_module_state*    module_state;
    struct tsint_unit_state*      unit_state;
    unsigned int                  status;
    int                           exception;

    action                       = action_state->action;
    unit_state                   = action_state->unit_state;
    unit_state->current_location = action->location;
    unit_state->mode             = *mode;
    module_state                 = unit_state->module_state;
    controller_data              = module_state->controller_data;

    unit_state->exception = TSInt_EvaluateAction(action_state, &status);
    if(unit_state->exception != TSINT_EXCEPTION_NONE)
        goto evaluation_failed;

    switch(status)
    {
    case TSINT_ACTION_PENDING:
        break;

    case TSINT_ACTION_TRIGGERED:
        unit_state->exception = TSInt_PrepActionRun(action_state);
        if(unit_state->exception != TSINT_EXCEPTION_NONE)
            goto wait_action_failed;

        block = &action->block;

        unit_state->exception = TSInt_StartBlock(block, NULL, unit_state, &statement);
        if(unit_state->exception != TSINT_EXCEPTION_NONE)
            goto start_block_failed;

        unit_state->exception = RunBlock(unit_state, statement, controller_data);
        if(unit_state->exception != TSINT_EXCEPTION_NONE)
            goto run_block_failed;

        *mode = unit_state->mode;

        if(unit_state->flags&TSINT_UNIT_STATE_FLAG_FINISH)
            TSInt_StopUnit(unit_state);
        else
        {
            unit_state->exception = TSInt_UpdateAction(action_state);
            if(unit_state->exception != TSINT_EXCEPTION_NONE)
                goto update_action_failed;
        }

        break;

    case TSINT_ACTION_FINISHED:
        *mode = unit_state->mode;

        if(unit_state->active_action_count == 0)
            TSInt_StopUnit(unit_state);

        break;
    }

    return TSINT_EXCEPTION_NONE;

update_action_failed:
run_block_failed:
start_block_failed:
wait_action_failed:
evaluation_failed:
    if(unit_state->mode != TSINT_CONTROL_HALT && controller_data != NULL)
    {
        controller_data->function(
                                  unit_state,
                                  controller_data->user_data
                                 );
        unit_state->mode = TSINT_CONTROL_HALT;
    }

    *mode     = unit_state->mode;
    exception = unit_state->exception;

    TSInt_StopUnit(unit_state);

    return exception;
}

void TSInt_StopUnit (struct tsint_unit_state* unit_state)
{
    struct tsint_module_state* module_state;
    struct tsdef_statement*    statement;

    TSInt_StopActions(unit_state);

    module_state = unit_state->module_state;

    if(unit_state->next_active_unit != NULL)
        unit_state->next_active_unit->previous_active_unit = unit_state->previous_active_unit;
    if(unit_state->previous_active_unit != NULL)
        unit_state->previous_active_unit->next_active_unit = unit_state->next_active_unit;
    else
        module_state->active_units = unit_state->next_active_unit;

    while(unit_state->execution_stack_depth > 0)
        TSInt_FinishBlock(unit_state, &statement);

    free(unit_state->trigger_user_data);
    free(unit_state);
}

