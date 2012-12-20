/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "parserutil.h"
#include "tokens.h"

#include <tsdef/error.h>
#include <tsdef/deferror.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>


static int HandleError (int, unsigned int, struct tsdef_parser_state*);


static int HandleError (int def_error, unsigned int location, struct tsdef_parser_state* state)
{
    int util_error;

    util_error = TSDEF_UTIL_CONTINUE_PARSE;

    switch(def_error)
    {
    case TSDEF_DEF_ERROR_SYNTAX:
    case TSDEF_DEF_ERROR_FLOW_CONTROL_OUTSIDE_LOOP:
        util_error = TSDEF_UTIL_ABORT_STATEMENT;

        break;

    default:
        util_error = TSDEF_UTIL_ABORT_PARSE;

        break;
    }

    if(state->error_list != NULL)
    {
        int error;

        error = TSDef_AppendDefErrorList(
                                         def_error,
                                         state->unit->name,
                                         location,
                                         0,
                                         NULL,
                                         state->error_list
                                        );
        if(error != TSDEF_ERROR_NONE)
            util_error = TSDEF_UTIL_ABORT_PARSE;
    }

    state->error_count++;

    return util_error;
}


void yyerror (struct tsdef_parser_state* state, char* error_text)
{
}

void TSDef_Parser_ProcessError (struct tsdef_parser_state* state)
{
    HandleError(TSDEF_DEF_ERROR_SYNTAX, state->current_line_number-1, state);
}

int TSDef_Parser_Input (struct tsdef_variable_list* variable_list, struct tsdef_parser_state* state)
{
    int error;

    error = TSDef_DefineInput(variable_list, state->current_line_number-1, state->unit, NULL);
    if(error != TSDEF_ERROR_NONE)
        goto define_input_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

define_input_failed:
    TSDef_DestroyVariableList(variable_list);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

int TSDef_Parser_Output (struct tsdef_assignment* assignment, struct tsdef_parser_state* state)
{
    int error;

    error = TSDef_DefineOutput(assignment, state->current_line_number-1, state->unit, NULL);
    if(error != TSDEF_ERROR_NONE)
        goto define_output_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

define_output_failed:
    TSDef_DestroyAssignment(assignment);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

int TSDef_Parser_Action (
                         struct tsdef_function_call_list* function_call_list,
                         struct tsdef_parser_state*       state
                        )
{
    struct tsdef_action* action;
    int                  error;

    error = TSDef_AddAction(function_call_list, state->current_line_number-1, state->unit, &action);
    if(error != TSDEF_ERROR_NONE)
        goto add_action_failed;

    state->current_block = &action->block;

    return TSDEF_UTIL_CONTINUE_PARSE;

add_action_failed:
    TSDef_DestroyFunctionCallList(function_call_list);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

void TSDef_Parser_EndAction (struct tsdef_parser_state* state)
{
    state->current_block = &state->unit->global_block;
}

int TSDef_Parser_AddFunctionCall (
                                  struct tsdef_function_call* function_call,
                                  struct tsdef_parser_state*  state
                                 )
{
    struct tsdef_block* current_block;
    int                 error;

    current_block = state->current_block;

    error = TSDef_AppendStatement(
                                  TSDEF_STATEMENT_TYPE_FUNCTION_CALL,
                                  function_call,
                                  state->current_line_number-1,
                                  current_block,
                                  NULL
                                 );
    if(error != TSDEF_ERROR_NONE)
        goto append_statement_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

append_statement_failed:
    TSDef_DestroyFunctionCall(function_call);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

int TSDef_Parser_Assignment (
                             struct tsdef_variable_reference* variable,
                             struct tsdef_exp*                exp,
                             struct tsdef_parser_state*       state,
                             struct tsdef_assignment**        created_assignment
                            )
{
    struct tsdef_assignment* assignment;
    int                      error;

    error = TSDef_DefineAssignment(variable, exp, &assignment);
    if(error != TSDEF_ERROR_NONE)
        goto define_assignment_failed;

    *created_assignment = assignment;

    return TSDEF_UTIL_CONTINUE_PARSE;

define_assignment_failed:
    TSDef_DestroyVariableReference(variable);
    TSDef_DestroyExp(exp);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_AddAssignment (
                                struct tsdef_assignment*   assignment,
                                struct tsdef_parser_state* state
                               )
{
    struct tsdef_block* current_block;
    int                 error;

    current_block = state->current_block;

    error = TSDef_AppendStatement(
                                  TSDEF_STATEMENT_TYPE_ASSIGNMENT,
                                  assignment,
                                  state->current_line_number-1,
                                  current_block,
                                  NULL
                                 );
    if(error != TSDEF_ERROR_NONE)
        goto append_statement_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

append_statement_failed:
    TSDef_DestroyAssignment(assignment);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

int TSDef_Parser_AddIfStatement (
                                 struct tsdef_exp*          exp,
                                 unsigned int               flags,
                                 struct tsdef_parser_state* state
                                )
{
    struct tsdef_if_statement* if_statement;
    struct tsdef_block*        current_block;
    int                        error;

    current_block = state->current_block;

    error = TSDef_DefineIfStatement(exp, flags, &if_statement);
    if(error != TSDEF_ERROR_NONE)
    {
        TSDef_DestroyExp(exp);

        goto define_statement_failed;
    }

    error = TSDef_AppendStatement(
                                  TSDEF_STATEMENT_TYPE_IF_STATEMENT,
                                  if_statement,
                                  state->current_line_number-1,
                                  current_block,
                                  NULL
                                 );
    if(error != TSDEF_ERROR_NONE)
    {
        TSDef_DestroyIfStatement(if_statement);

        goto append_statement_failed;
    }

    state->current_block = &if_statement->block;

    return TSDEF_UTIL_CONTINUE_PARSE;

append_statement_failed:
define_statement_failed:
    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

int TSDef_Parser_AddLoop (struct tsdef_parser_state* state, struct tsdef_loop** created_loop)
{
    struct tsdef_loop*  loop;
    struct tsdef_block* current_block;
    int                 error;

    current_block = state->current_block;

    error = TSDef_DeclareLoop(&loop);
    if(error != TSDEF_ERROR_NONE)
        goto declare_loop_failed;

    error = TSDef_AppendStatement(
                                  TSDEF_STATEMENT_TYPE_LOOP,
                                  loop,
                                  state->current_line_number-1,
                                  current_block,
                                  NULL
                                 );
    if(error != TSDEF_ERROR_NONE)
        goto append_statement_failed;

    state->current_block = &loop->block;

    if(created_loop != NULL)
        *created_loop = loop;

    return TSDEF_UTIL_CONTINUE_PARSE;

append_statement_failed:
    TSDef_DestroyLoop(loop);

declare_loop_failed:
    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

int TSDef_Parser_AddWhileLoop (struct tsdef_exp* exp, struct tsdef_parser_state* state)
{
    struct tsdef_loop* loop;
    int                error;

    error = TSDef_Parser_AddLoop(state, &loop);
    if(error != TSDEF_UTIL_CONTINUE_PARSE)
    {
        TSDef_DestroyExp(exp);

        return error;
    }

    TSDef_MakeLoopWhile(exp, loop);

    return TSDEF_UTIL_CONTINUE_PARSE;
}

int TSDef_Parser_AddForLoop (
                             struct tsdef_assignment*         assignment,
                             struct tsdef_variable_reference* variable,
                             struct tsdef_exp*                exp,
                             unsigned int                     flags,
                             struct tsdef_parser_state*       state
                            )
{
    struct tsdef_loop* loop;
    int                error;

    error = TSDef_Parser_AddLoop(state, &loop);
    if(error != TSDEF_UTIL_CONTINUE_PARSE)
        goto add_for_loop_failed;

    TSDef_MakeLoopFor(variable, assignment, exp, flags, loop);

    return TSDEF_UTIL_CONTINUE_PARSE;

add_for_loop_failed:
    if(assignment != NULL)
        TSDef_DestroyAssignment(assignment);
    if(variable != NULL)
        TSDef_DestroyVariableReference(variable);

    TSDef_DestroyExp(exp);

    return error;
}

int TSDef_Parser_AddLoopFlowControl (unsigned int type, struct tsdef_parser_state* state)
{
    struct tsdef_block* scan_scope;
    struct tsdef_block* current_block;
    int                 error;
    int                 def_error;

    current_block = state->current_block;

    scan_scope = state->current_block;
    while(scan_scope != NULL)
    {
        if(
           scan_scope->parent_statement != NULL &&
           scan_scope->parent_statement->type == TSDEF_STATEMENT_TYPE_LOOP
          )
        {
            break;
        }

        scan_scope = scan_scope->parent_block;
    }

    if(scan_scope == NULL)
    {
        def_error = TSDEF_DEF_ERROR_FLOW_CONTROL_OUTSIDE_LOOP;

        goto no_loop_in_scope;
    }

    error = TSDef_AppendStatement(
                                  type,
                                  NULL,
                                  state->current_line_number-1,
                                  current_block,
                                  NULL
                                 );
    if(error != TSDEF_ERROR_NONE)
    {
        def_error = TSDEF_DEF_ERROR_INTERNAL;

        goto append_statement_failed;
    }

    return TSDEF_UTIL_CONTINUE_PARSE;

append_statement_failed:
no_loop_in_scope:
    error = HandleError(def_error, state->current_line_number-1, state);

    return error;
}

void TSDef_Parser_EndBlock (struct tsdef_parser_state* state)
{
    struct tsdef_block* current_block;

    current_block        = state->current_block;
    state->current_block = current_block->parent_block;
}

int TSDef_Parser_AddFinish (struct tsdef_parser_state* state)
{
    int error;

    error = TSDef_AppendStatement(
                                  TSDEF_STATEMENT_TYPE_FINISH,
                                  NULL,
                                  state->current_line_number-1,
                                  state->current_block,
                                  NULL
                                 );
    if(error != TSDEF_ERROR_NONE)
        goto append_statement_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

append_statement_failed:
    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number-1, state);

    return error;
}

int TSDef_Parser_ExpList (
                          struct tsdef_exp*          exp,
                          struct tsdef_exp_list*     existing_list,
                          struct tsdef_parser_state* state,
                          struct tsdef_exp_list**    created_list
                         )
{
    int error;

    error = TSDef_ConstructExpList(exp, existing_list, created_list);
    if(error != TSDEF_ERROR_NONE)
        goto construct_list_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

construct_list_failed:
    if(existing_list != NULL)
        TSDef_DestroyExpList(existing_list);

    TSDef_DestroyExp(exp);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_Exp (
                      unsigned int               type,
                      void*                      exp_data,
                      struct tsdef_parser_state* state,
                      struct tsdef_exp**         created_exp
                     )
{
    struct tsdef_exp* exp;
    int               error;

    error = TSDef_CreateExp(type, exp_data, &exp);
    if(error != TSDEF_ERROR_NONE)
        goto create_exp_failed;

    *created_exp = exp;

    return TSDEF_UTIL_CONTINUE_PARSE;

create_exp_failed:
    switch(type)
    {
    case TSDEF_EXP_TYPE_PRIMARY:
        TSDef_DestroyPrimaryExp(exp_data);

        break;

    case TSDEF_EXP_TYPE_COMPARISON:
        TSDef_DestroyComparisonExp(exp_data);

        break;

    case TSDEF_EXP_TYPE_LOGICAL:
        TSDef_DestroyLogicalExp(exp_data);

        break;
    }

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_LogicalExp (
                             unsigned int               op,
                             struct tsdef_exp*          left_exp,
                             unsigned int               left_exp_flags,
                             struct tsdef_exp*          right_exp,
                             unsigned int               right_exp_flags,
                             struct tsdef_logical_exp*  remaining_exp,
                             struct tsdef_parser_state* state,
                             struct tsdef_logical_exp** logical_exp
                            )
{
    struct tsdef_logical_exp* exp;
    int                       error;

    error = TSDef_ConstructLogicalExp(
                                      op,
                                      left_exp,
                                      left_exp_flags,
                                      right_exp,
                                      right_exp_flags,
                                      remaining_exp,
                                      &exp
                                     );
    if(error != TSDEF_ERROR_NONE)
        goto construct_exp_failed;

    *logical_exp = exp;

    return TSDEF_UTIL_CONTINUE_PARSE;

construct_exp_failed:
    TSDef_DestroyExp(left_exp);

    if(right_exp != NULL)
        TSDef_DestroyExp(right_exp);
    if(remaining_exp != NULL)
        TSDef_DestroyLogicalExp(remaining_exp);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_ComparisonExp (
                                unsigned int                  op,
                                struct tsdef_primary_exp*     left_exp,
                                struct tsdef_primary_exp*     right_exp,
                                struct tsdef_comparison_exp*  remaining_exp,
                                struct tsdef_parser_state*    state,
                                struct tsdef_comparison_exp** comparison_exp
                               )
{
    struct tsdef_comparison_exp* exp;
    int                          error;

    error = TSDef_ConstructComparisonExp(op, left_exp, right_exp, remaining_exp, &exp);
    if(error != TSDEF_ERROR_NONE)
        goto construct_exp_failed;

    *comparison_exp = exp;

    return TSDEF_UTIL_CONTINUE_PARSE;

construct_exp_failed:
    TSDef_DestroyPrimaryExp(left_exp);

    if(right_exp != NULL)
        TSDef_DestroyPrimaryExp(right_exp);
    if(remaining_exp != NULL)
        TSDef_DestroyComparisonExp(remaining_exp);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_PrimaryExp (
                             unsigned int                 op,
                             struct tsdef_exp_value_type* exp_value_type,
                             struct tsdef_primary_exp*    remaining_exp,
                             struct tsdef_parser_state*   state,
                             struct tsdef_primary_exp**   primary_exp
                            )
{
    struct tsdef_primary_exp* exp;
    int                       error;

    error = TSDef_ConstructPrimaryExp(op, exp_value_type, remaining_exp, &exp);
    if(error != TSDEF_ERROR_NONE)
        goto construct_exp_failed;

    *primary_exp = exp;

    return TSDEF_UTIL_CONTINUE_PARSE;

construct_exp_failed:
    TSDef_DestroyExpValueType(exp_value_type);

    if(remaining_exp != NULL)
        TSDef_DestroyPrimaryExp(remaining_exp);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_SetPrimaryExpFlag (
                                    unsigned int               flags,
                                    struct tsdef_primary_exp*  exp,
                                    struct tsdef_parser_state* state
                                   )
{
    int error;

    error = TSDef_SetPrimaryExpFlag(flags, exp);
    if(error != TSDEF_ERROR_NONE)
        goto set_flag_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

set_flag_failed:
    TSDef_DestroyPrimaryExp(exp);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_ExpValueType (
                               unsigned int                  type,
                               void*                         data,
                               struct tsdef_parser_state*    state,
                               struct tsdef_exp_value_type** exp_value_type
                              )
{
    struct tsdef_exp_value_type* value_type;
    unsigned int                 error;

    if(type == TSDEF_EXP_VALUE_TYPE_STRING && data == NULL)
        goto lexer_allocation_failed;

    error = TSDef_CreateExpValueType(type, data, &value_type);
    if(error != TSDEF_ERROR_NONE)
        goto create_value_type_failed;

    *exp_value_type = value_type;

    return TSDEF_UTIL_CONTINUE_PARSE;

create_value_type_failed:
    switch(type)
    {
    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        TSDef_DestroyFunctionCall(data);

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        TSDef_DestroyVariableReference(data);

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        TSDef_DestroyExp(data);

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        free(data);

        break;

    case TSDEF_EXP_VALUE_TYPE_BOOL:
    case TSDEF_EXP_VALUE_TYPE_INT:
    case TSDEF_EXP_VALUE_TYPE_REAL:
        break;
    }

lexer_allocation_failed:
    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_FunctionCallList (
                                   struct tsdef_function_call*       function_call,
                                   struct tsdef_function_call_list*  existing_list,
                                   struct tsdef_parser_state*        state,
                                   struct tsdef_function_call_list** created_list
                                  )
{
    int error;

    error = TSDef_ConstructFunctionCallList(function_call, existing_list, created_list);
    if(error != TSDEF_ERROR_NONE)
        goto construct_list_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

construct_list_failed:
    if(existing_list != NULL)
        TSDef_DestroyFunctionCallList(existing_list);

    TSDef_DestroyFunctionCall(function_call);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_FunctionCall (
                               char*                        name,
                               struct tsdef_exp_list*       exp_list,
                               struct tsdef_parser_state*   state,
                               struct tsdef_function_call** defined_function_call
                              )
{
    struct tsdef_function_call* function_call;
    int                         error;

    if(name == NULL)
        goto lexer_failed_allocation;

    error = TSDef_DefineFunctionCall(name, exp_list, &function_call);

    free(name);

    if(error != TSDEF_ERROR_NONE)
        goto define_function_call_failed;

    *defined_function_call = function_call;

    return TSDEF_UTIL_CONTINUE_PARSE;

define_function_call_failed:
    TSDef_DestroyExpList(exp_list);
lexer_failed_allocation:
    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_VariableList (
                               struct tsdef_variable_reference* variable,
                               struct tsdef_variable_list*      existing_list,
                               struct tsdef_parser_state*       state,
                               struct tsdef_variable_list**     created_list
                              )
{
    int error;

    error = TSDef_ConstructVariableList(variable, existing_list, created_list);
    if(error != TSDEF_ERROR_NONE)
        goto construct_list_failed;

    return TSDEF_UTIL_CONTINUE_PARSE;

construct_list_failed:
    if(existing_list != NULL)
        TSDef_DestroyVariableList(existing_list);

    TSDef_DestroyVariableReference(variable);

    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

int TSDef_Parser_ReferenceVariable (
                                    char*                             name,
                                    struct tsdef_parser_state*        state,
                                    struct tsdef_variable_reference** variable_reference
                                   )
{
    struct tsdef_variable_reference* reference;
    int                              error;

    if(name == NULL)
        goto lexer_failed_allocation;

    error = TSDef_ReferenceVariable(name, &reference);

    free(name);

    if(error != TSDEF_ERROR_NONE)
        goto reference_variable_failed;

    *variable_reference = reference;

    return TSDEF_UTIL_CONTINUE_PARSE;

reference_variable_failed:
lexer_failed_allocation:
    error = HandleError(TSDEF_DEF_ERROR_INTERNAL, state->current_line_number, state);

    return error;
}

