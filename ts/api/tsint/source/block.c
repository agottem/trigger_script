/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "block.h"

#include <tsint/error.h>
#include <tsint/exception.h>

#include <stdlib.h>
#include <malloc.h>


int TSInt_StartBlock (
                      struct tsdef_block*      block,
                      struct tsdef_statement*  return_statement,
                      struct tsint_unit_state* state,
                      struct tsdef_statement** current_statement
                     )
{
    struct tsint_execution_stack* stack;
    size_t                        alloc_size;
    unsigned int                  depth;
    unsigned int                  exception;

    if(state->execution_stack != NULL)
        depth = state->current_execution_depth+1;
    else
        depth = 0;

    if(depth >= state->execution_stack_depth)
    {
        struct tsint_execution_stack* resized_stack;
        unsigned int                  new_depth_count;

        new_depth_count = depth+1;

        alloc_size = new_depth_count*sizeof(struct tsint_execution_stack);

        resized_stack = realloc(state->execution_stack, alloc_size);
        if(resized_stack == NULL)
            goto stack_resize_failed;

        state->execution_stack       = resized_stack;
        state->execution_stack_depth = new_depth_count;

        stack = &resized_stack[depth];
    }
    else
        stack = &state->execution_stack[depth];

    stack->return_statement = return_statement;
    stack->return_block     = state->current_block;

    if(block->variable_count != 0)
    {
        struct tsint_variable* variables;
        unsigned int           index;

        alloc_size = block->variable_count*sizeof(struct tsint_variable);

        variables = malloc(alloc_size);
        if(variables == NULL)
            goto alloc_variables_failed;

        for(index = 0; index < block->variable_count; index++)
            variables[index].flags = 0;

        stack->variables = variables;
    }
    else
        stack->variables = NULL;

    state->current_block           = block;
    state->current_execution_depth = depth;

    *current_statement = block->statements;

    return TSINT_EXCEPTION_NONE;

alloc_variables_failed:
stack_resize_failed:
    return TSINT_EXCEPTION_OUT_OF_MEMORY;
}

void TSInt_FinishBlock (
                        struct tsint_unit_state* state,
                        struct tsdef_statement** statement
                       )
{
    struct tsdef_block*           current_scope;
    struct tsint_execution_stack* stack;
    struct tsdef_variable*        variable_def;
    struct tsint_variable*        variable_data_array;

    current_scope = state->current_block;
    stack         = &state->execution_stack[state->current_execution_depth];

    *statement           = stack->return_statement;
    state->current_block = stack->return_block;

    variable_data_array = stack->variables;

    for(
        variable_def = current_scope->variables;
        variable_def != NULL;
        variable_def = variable_def->next_variable
       )
    {
        struct tsint_variable* variable_data;

        variable_data = &variable_data_array[variable_def->index];
        if(variable_data->flags&TSINT_VARIABLE_FLAG_INITIALIZED)
            TSInt_DestroyValue(variable_data->value, variable_def->primitive_type);
    }

    if(variable_data_array != NULL)
        free(variable_data_array);

    if(state->current_execution_depth == 0)
    {
        free(state->execution_stack);

        state->execution_stack       = NULL;
        state->execution_stack_depth = 0;
    }
    else
    {
        struct tsdef_statement* parent_statement;

        parent_statement = current_scope->parent_statement;

        if(parent_statement != NULL && parent_statement->type == TSDEF_STATEMENT_TYPE_LOOP)
        {
            struct tsdef_loop* loop;

            loop = parent_statement->data.loop;

            if(loop->type == TSDEF_LOOP_TYPE_FOR)
            {
                stack = &state->execution_stack[state->current_execution_depth-1];

                if(stack->statement_data.for_loop.flags&TSINT_VARIABLE_FLAG_INITIALIZED)
                {
                    struct tsdef_for_loop* for_loop;
                    struct tsdef_variable* variable_def;
                    unsigned int           primitive_type;

                    for_loop = &loop->data.for_loop;

                    variable_def   = for_loop->variable->variable;
                    primitive_type = variable_def->primitive_type;

                    TSInt_DestroyValue(stack->statement_data.for_loop.to_value, primitive_type);
                    TSInt_DestroyValue(stack->statement_data.for_loop.step_value, primitive_type);
                }
            }
        }

        state->current_execution_depth--;
    }
}

