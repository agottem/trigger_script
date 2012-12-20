/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "statement.h"
#include "assignment.h"
#include "unit.h"
#include "block.h"
#include "expvalue.h"
#include "expeval.h"
#include "expop.h"
#include "ffi.h"

#include <tsdef/module.h>
#include <tsffi/register.h>
#include <tsffi/function.h>
#include <tsffi/error.h>
#include <tsint/error.h>
#include <tsint/exception.h>
#include <tsint/variable.h>
#include <tsint/value.h>

#include <stdlib.h>


#define STOP_LOOP     0
#define CONTINUE_LOOP 1


static int ContinueLoop (struct tsdef_loop*, unsigned int*, struct tsint_unit_state*);

static int ProcessFunctionCall (struct tsdef_statement**, struct tsint_unit_state*);
static int ProcessAssignment   (struct tsdef_statement**, struct tsint_unit_state*);
static int ProcessIfStatement  (struct tsdef_statement**, struct tsint_unit_state*);
static int ProcessLoopStart    (struct tsdef_statement**, struct tsint_unit_state*);
static int ProcessLoopFinish   (struct tsdef_statement**, struct tsint_unit_state*);
static int ProcessLoopContinue (struct tsdef_statement**, struct tsint_unit_state*);
static int ProcessLoopBreak    (struct tsdef_statement**, struct tsint_unit_state*);
static int ProcessFinish       (struct tsdef_statement**, struct tsint_unit_state*);


static int ContinueLoop (
                         struct tsdef_loop*       loop,
                         unsigned int*            continue_loop,
                         struct tsint_unit_state* state
                        )
{
    union tsint_value             result;
    struct tsdef_block*           loop_block;
    struct tsdef_while_loop*      while_loop;
    struct tsdef_for_loop*        for_loop;
    struct tsdef_variable*        variable_def;
    struct tsint_variable*        variable;
    struct tsint_execution_stack* stack;
    unsigned int                  comparison_op;
    unsigned int                  primitive_type;
    int                           exception;

    loop_block = &loop->block;

    switch(loop->type)
    {
    case TSDEF_LOOP_TYPE_WHILE:
        while_loop = &loop->data.while_loop;

        exception = TSInt_EvaluateExp(
                                     TSDEF_PRIMITIVE_TYPE_BOOL,
                                     while_loop->exp,
                                     state,
                                     &result
                                    );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        if(result.bool_data == TSDEF_BOOL_TRUE)
            *continue_loop = CONTINUE_LOOP;
        else
            *continue_loop = STOP_LOOP;

        TSInt_DestroyValue(result, TSDEF_PRIMITIVE_TYPE_BOOL);

        break;

    case TSDEF_LOOP_TYPE_FOR:
        for_loop = &loop->data.for_loop;
        stack    = &state->execution_stack[state->current_execution_depth-1];

        variable_def  = for_loop->variable->variable;
        comparison_op = stack->statement_data.for_loop.comparison_op;

        variable       = TSInt_LookupVariableAddress(variable_def, state);
        primitive_type = variable_def->primitive_type;

        switch(primitive_type)
        {
        case TSDEF_PRIMITIVE_TYPE_BOOL:
            exception = TSInt_BoolComparisonExpOp(
                                                  comparison_op,
                                                  variable->value,
                                                  stack->statement_data.for_loop.to_value,
                                                  &result
                                                 );

            break;

        case TSDEF_PRIMITIVE_TYPE_INT:
            exception = TSInt_IntComparisonExpOp(
                                                 comparison_op,
                                                 variable->value,
                                                 stack->statement_data.for_loop.to_value,
                                                 &result
                                                );

            break;

        case TSDEF_PRIMITIVE_TYPE_REAL:
            exception = TSInt_RealComparisonExpOp(
                                                  comparison_op,
                                                  variable->value,
                                                  stack->statement_data.for_loop.to_value,
                                                  &result
                                                 );

            break;
        }

        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        if(result.bool_data == TSDEF_BOOL_TRUE)
            *continue_loop = CONTINUE_LOOP;
        else
            *continue_loop = STOP_LOOP;

        TSInt_DestroyValue(result, TSDEF_PRIMITIVE_TYPE_BOOL);

        break;

    default:
        *continue_loop = CONTINUE_LOOP;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

static int ProcessFunctionCall (
                                struct tsdef_statement** statement,
                                struct tsint_unit_state* unit_state
                               )
{
    struct tsdef_statement*     function_statement;
    struct tsdef_function_call* function_call;
    struct tsdef_module_object* module_object;

    function_statement = *statement;
    function_call      = function_statement->data.function_call;
    module_object      = function_call->module_object;

    if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT)
    {
        struct tsdef_unit* unit;
        union tsint_value* arguments;
        int                mode;
        int                original_mode;
        int                exception;

        unit = module_object->type.unit;

        exception = TSInt_ExpListToValueArray(
                                              function_call->arguments,
                                              unit->input,
                                              unit_state,
                                              &arguments
                                             );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        mode          = TSInt_ControlModeForInvokedUnit(unit_state->mode);
        original_mode = mode;

        exception = TSInt_InvokeUnit(
                                     unit,
                                     arguments,
                                     NULL,
                                     &mode,
                                     unit_state->module_state
                                    );
        if(original_mode != mode)
            unit_state->mode = mode;

        TSInt_DestroyValueArray(unit->input, arguments);

        if(exception != TSINT_EXCEPTION_NONE)
            return exception;
    }
    else
    {
        struct tsffi_invocation_data      invocation_data;
        union tsffi_value                 ffi_output;
        struct tsffi_function_definition* ffi_function;
        union tsffi_value*                ffi_arguments;
        struct tsdef_module_ffi_group*    ffi_group;
        struct tsint_module_state*        module_state;
        int                               exception;

        ffi_function = module_object->type.ffi.function_definition;
        ffi_group    = module_object->type.ffi.group;

        module_state = unit_state->module_state;

        exception = TSInt_ExpListToFFIArguments(
                                                function_call->arguments,
                                                ffi_function->argument_types,
                                                unit_state,
                                                &ffi_arguments
                                               );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        invocation_data.execif             = module_state->module_execif;
        invocation_data.execif_data        = module_state;
        invocation_data.unit_invocation_id = unit_state->unit_id;
        invocation_data.unit_name          = unit_state->unit->name;

        if(unit_state->current_statement != NULL)
            invocation_data.unit_location = unit_state->current_statement->location;
        else
            invocation_data.unit_location = 0;

        exception = ffi_function->function(
                                           &invocation_data,
                                           module_state->ffi_group_data[ffi_group->group_id],
                                           &ffi_output,
                                           ffi_arguments
                                          );

        TSInt_DestroyFFIArguments(
                                  ffi_function->argument_types,
                                  ffi_function->argument_count,
                                  ffi_arguments
                                 );

        if(exception != TSFFI_ERROR_NONE)
            return TSINT_EXCEPTION_FFI;

        if(ffi_function->output_type != TSFFI_PRIMITIVE_TYPE_VOID)
            TSInt_DestroyFFIArgument(ffi_output, ffi_function->output_type);
    }

    *statement = function_statement->next_statement;

    return TSINT_EXCEPTION_NONE;
}

static int ProcessAssignment (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    struct tsdef_statement* assignment;
    int                     exception;

    assignment = *statement;

    exception = TSInt_PerformAssignment(assignment->data.assignment, state);
    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    *statement = assignment->next_statement;

    return TSINT_EXCEPTION_NONE;
}

static int ProcessIfStatement (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    struct tsdef_block*        if_block;
    struct tsdef_statement*    if_statement;
    struct tsdef_if_statement* if_data;
    struct tsdef_exp*          exp;
    unsigned int               exception;

    if_statement = *statement;
    if_data      = if_statement->data.if_statement;

    exp = if_data->exp;

    if(exp != NULL)
    {
        union tsint_value exp_value;

        exception = TSInt_EvaluateExp(
                                      TSDEF_PRIMITIVE_TYPE_BOOL,
                                      exp,
                                      state,
                                      &exp_value
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        if(exp_value.bool_data == TSDEF_BOOL_FALSE)
        {
            TSInt_DestroyValue(exp_value, TSDEF_PRIMITIVE_TYPE_BOOL);

            *statement = if_statement->next_statement;

            return TSINT_EXCEPTION_NONE;
        }

        TSInt_DestroyValue(exp_value, TSDEF_PRIMITIVE_TYPE_BOOL);
    }

    if_block = &if_data->block;

    do
    {
        if_statement = if_statement->next_statement;
    }while(
           if_statement != NULL &&
           if_statement->type == TSDEF_STATEMENT_TYPE_IF_STATEMENT &&
           if_statement->data.if_statement->flags&TSDEF_IF_STATEMENT_FLAG_ELSE
          );

    exception = TSInt_StartBlock(if_block, if_statement, state, statement);

    return exception;
}

static int ProcessLoopStart (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    union tsint_value             step_value;
    struct tsdef_block*           loop_block;
    struct tsdef_statement*       loop_statement;
    struct tsdef_loop*            loop;
    struct tsdef_for_loop*        for_loop;
    struct tsint_execution_stack* stack;
    unsigned int                  primitive_type;
    unsigned int                  continue_loop;
    int                           exception;

    loop_statement = *statement;
    loop           = loop_statement->data.loop;
    loop_block     = &loop->block;

    exception = TSInt_StartBlock(loop_block, loop_statement->next_statement, state, statement);
    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    switch(loop->type)
    {
    case TSDEF_LOOP_TYPE_FOR:
        for_loop = &loop->data.for_loop;
        stack    = &state->execution_stack[state->current_execution_depth-1];

        stack->statement_data.for_loop.flags = 0;

        if(for_loop->assignment != NULL)
        {
            exception = TSInt_PerformAssignment(for_loop->assignment, state);
            if(exception != TSINT_EXCEPTION_NONE)
                return exception;
        }

        primitive_type = for_loop->variable->variable->primitive_type;

        exception = TSInt_EvaluateExp(
                                      primitive_type,
                                      for_loop->to_exp,
                                      state,
                                      &stack->statement_data.for_loop.to_value
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        step_value.int_data = 1;

        exception = TSInt_ConvertValue(
                                       step_value,
                                       TSDEF_PRIMITIVE_TYPE_INT,
                                       primitive_type,
                                       &stack->statement_data.for_loop.step_value
                                      );
        if(exception != TSINT_ERROR_NONE)
        {
            TSInt_DestroyValue(stack->statement_data.for_loop.to_value, primitive_type);

            return TSINT_EXCEPTION_OUT_OF_MEMORY;
        }

        stack->statement_data.for_loop.flags |= TSINT_VARIABLE_FLAG_INITIALIZED;

        if(for_loop->flags&TSDEF_FOR_LOOP_FLAG_UP)
        {
            stack->statement_data.for_loop.comparison_op = TSDEF_COMPARISON_EXP_OP_LESS;
            stack->statement_data.for_loop.step_op       = TSDEF_PRIMARY_EXP_OP_ADD;
        }
        else
        {
            stack->statement_data.for_loop.comparison_op = TSDEF_COMPARISON_EXP_OP_GREATER;
            stack->statement_data.for_loop.step_op       = TSDEF_PRIMARY_EXP_OP_SUB;
        }

        break;

    case TSDEF_LOOP_TYPE_WHILE:
    default:
        break;
    }

    exception = ContinueLoop(loop, &continue_loop, state);
    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    if(continue_loop != CONTINUE_LOOP)
        TSInt_FinishBlock(state, statement);

    return TSINT_EXCEPTION_NONE;
}

static int ProcessLoopFinish (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    union tsint_value             result;
    struct tsdef_block*           loop_block;
    struct tsdef_loop*            loop;
    struct tsdef_statement*       loop_statement;
    struct tsdef_for_loop*        for_loop;
    struct tsdef_variable*        variable_def;
    struct tsint_variable*        variable;
    struct tsint_execution_stack* stack;
    unsigned int                  continue_loop;
    unsigned int                  step_op;
    unsigned int                  primitive_type;
    int                           exception;

    loop_statement = *statement;
    loop           = loop_statement->data.loop;

    loop_block = &loop->block;

    switch(loop->type)
    {
    case TSDEF_LOOP_TYPE_FOR:
        for_loop = &loop->data.for_loop;
        stack    = &state->execution_stack[state->current_execution_depth-1];

        variable_def = for_loop->variable->variable;
        step_op      = stack->statement_data.for_loop.step_op;

        variable       = TSInt_LookupVariableAddress(variable_def, state);
        primitive_type = variable_def->primitive_type;

        switch(primitive_type)
        {
        case TSDEF_PRIMITIVE_TYPE_BOOL:
            exception = TSInt_BoolPrimaryExpOp(
                                               step_op,
                                               variable->value,
                                               &stack->statement_data.for_loop.step_value,
                                               &result
                                              );

            break;

        case TSDEF_PRIMITIVE_TYPE_INT:
            exception = TSInt_IntPrimaryExpOp(
                                              step_op,
                                              variable->value,
                                              &stack->statement_data.for_loop.step_value,
                                              &result
                                             );

            break;

        case TSDEF_PRIMITIVE_TYPE_REAL:
            exception = TSInt_RealPrimaryExpOp(
                                               step_op,
                                               variable->value,
                                               &stack->statement_data.for_loop.step_value,
                                               &result
                                              );

            break;
        }

        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        TSInt_DestroyValue(variable->value, primitive_type);

        variable->value = result;

        break;

    case TSDEF_LOOP_TYPE_WHILE:
    default:
        break;
    }

    exception = ContinueLoop(loop, &continue_loop, state);
    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    if(continue_loop != CONTINUE_LOOP)
        TSInt_FinishBlock(state, statement);
    else
        *statement = loop->block.statements;

    return TSINT_EXCEPTION_NONE;
}

static int ProcessLoopContinue (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    int exception;

    while(state->current_block->parent_statement->type != TSDEF_STATEMENT_TYPE_LOOP)
        TSInt_FinishBlock(state, statement);

    *statement = state->current_block->parent_statement;

    exception = ProcessLoopFinish(statement, state);

    return exception;
}

static int ProcessLoopBreak (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    while(state->current_block->parent_statement->type != TSDEF_STATEMENT_TYPE_LOOP)
        TSInt_FinishBlock(state, statement);

    TSInt_FinishBlock(state, statement);

    return TSINT_EXCEPTION_NONE;
}

static int ProcessFinish (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    while(state->current_execution_depth > 0)
        TSInt_FinishBlock(state, statement);

    *statement = NULL;

    state->flags |= TSINT_UNIT_STATE_FLAG_FINISH;

    return TSINT_EXCEPTION_NONE;
}


int TSInt_ProcessStatement (struct tsdef_statement** statement, struct tsint_unit_state* state)
{
    struct tsint_execution_stack* return_stack;
    int                           exception;

    switch((*statement)->type)
    {
    case TSDEF_STATEMENT_TYPE_FUNCTION_CALL:
        exception = ProcessFunctionCall(statement, state);

        break;

    case TSDEF_STATEMENT_TYPE_ASSIGNMENT:
        exception = ProcessAssignment(statement, state);

        break;

    case TSDEF_STATEMENT_TYPE_IF_STATEMENT:
        exception = ProcessIfStatement(statement, state);

        break;

    case TSDEF_STATEMENT_TYPE_LOOP:
        exception = ProcessLoopStart(statement, state);

        break;

    case TSDEF_STATEMENT_TYPE_CONTINUE:
        exception = ProcessLoopContinue(statement, state);

        break;

    case TSDEF_STATEMENT_TYPE_BREAK:
        exception = ProcessLoopBreak(statement, state);

        break;

    case TSDEF_STATEMENT_TYPE_FINISH:
        exception = ProcessFinish(statement, state);

        break;
    }

    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    while(*statement == NULL && state->current_execution_depth > 0)
    {
        struct tsdef_statement* parent_statement;

        parent_statement = state->current_block->parent_statement;

        if(parent_statement != NULL && parent_statement->type == TSDEF_STATEMENT_TYPE_LOOP)
        {
            *statement = parent_statement;

            exception = ProcessLoopFinish(statement, state);
            if(exception != TSINT_EXCEPTION_NONE)
                return exception;
        }
        else
            TSInt_FinishBlock(state, statement);
    }

    return TSINT_EXCEPTION_NONE;
}

