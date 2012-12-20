/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsdef/resolve.h>
#include <tsdef/error.h>
#include <tsdef/deferror.h>
#include <tsdef/arguments.h>
#include <tsffi/register.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define ABORT_RESOLVE    -1
#define ABORT_STATEMENT   0
#define CONTINUE_RESOLVE  1

#define SEVERITY_WARNING 0
#define SEVERITY_ERROR   1

#define MAX_OPERATOR_VALUE 7

#define ACTIONABLE_FUNCTION 0
#define INVOCABLE_FUNCTION  1

#define MAX_INT_DIGITS 32


struct resolve_state
{
    struct tsdef_unit*      current_unit;
    struct tsdef_block*     current_block;
    struct tsdef_statement* current_statement;
    unsigned int            current_location;

    struct tsdef_module* module;

    tsdef_module_object_lookup module_object_lookup;
    void*                      lookup_data;

    struct tsdef_def_error_list* error_list;
    unsigned int                 error_count;
    unsigned int                 warning_count;
};


static void HandleError (
                         int,
                         int,
                         struct resolve_state*,
                         struct tsdef_def_error_info*
                        );

static int DecideExpValueTypePrimitive (struct resolve_state*, struct tsdef_block*, struct tsdef_exp_value_type*);

static int DecideFunctionCallPrimitive     (struct resolve_state*, struct tsdef_block*, struct tsdef_function_call*, unsigned int);
static int DecideFunctionCallListPrimitive (struct resolve_state*, struct tsdef_block*, struct tsdef_function_call_list*, unsigned int);
static int DecidePrimaryExpPrimitive       (struct resolve_state*, struct tsdef_block*, struct tsdef_primary_exp*);
static int DecideComparisonExpPrimitive    (struct resolve_state*, struct tsdef_block*, struct tsdef_comparison_exp*);
static int DecideLogicalExpPrimitive       (struct resolve_state*, struct tsdef_block*, struct tsdef_logical_exp*);
static int DecideExpPrimitive              (struct resolve_state*, struct tsdef_block*, struct tsdef_exp*);
static int DecideExpListPrimitives         (struct resolve_state*, struct tsdef_block*, struct tsdef_exp_list*);

static int PerformAssignment (struct resolve_state*, struct tsdef_block*, struct tsdef_assignment*);

static int ProcessFunctionCall (struct resolve_state*, struct tsdef_function_call*);
static int ProcessAssignment   (struct resolve_state*, struct tsdef_assignment*);
static int ProcessIfStatement  (struct resolve_state*, struct tsdef_if_statement*);
static int ProcessLoop         (struct resolve_state*, struct tsdef_loop*);
static int ProcessStatements   (struct resolve_state*);
static int ProcessActions      (struct resolve_state*);

static int ResolveInputTypes (
                              struct tsdef_unit*,
                              struct tsdef_argument_types*,
                              unsigned int,
                              struct resolve_state*,
                              struct tsdef_module_object**
                             );
static int ResolveOutputType (struct tsdef_unit*, struct resolve_state*);


static void HandleError (
                         int                          def_error,
                         int                          severity,
                         struct resolve_state*        state,
                         struct tsdef_def_error_info* info
                        )
{
    unsigned int flags;

    if(severity == SEVERITY_WARNING)
    {
        state->warning_count++;

        flags |= TSDEF_DEF_ERROR_FLAG_WARNING;
    }
    else
    {
        state->error_count++;

        flags = 0;
    }

    if(state->error_list != NULL)
    {
        int error;

        error = TSDef_AppendDefErrorList(
                                         def_error,
                                         state->current_unit->name,
                                         state->current_location,
                                         flags,
                                         info,
                                         state->error_list
                                        );
        if(error != TSDEF_ERROR_NONE)
            goto append_error_list_failed;
    }

    return;

append_error_list_failed:
    if(info != NULL)
    {
        switch(def_error)
        {
        case TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES:
            free(info->data.incompatible_types.convert_to_name);

            break;

        case TSDEF_DEF_ERROR_UNDEFINED_VARIABLE:
            free(info->data.undefined_variable.name);

            break;

        case TSDEF_DEF_ERROR_WRONG_ARGUMENT_COUNT:
            free(info->data.wrong_argument_count.name);

            break;

        case TSDEF_DEF_ERROR_VARIABLE_REDEFINITION:
            free(info->data.variable_redefinition.name);

            break;

        case TSDEF_DEF_ERROR_UNDEFINED_FUNCTION:
            free(info->data.undefined_function.name);

            break;

        case TSDEF_DEF_ERROR_FUNCTION_REDEFINITION:
            free(info->data.function_redefinition.name);

            break;

        case TSDEF_DEF_ERROR_FUNCTION_NOT_ACTIONABLE:
            free(info->data.function_not_actionable.name);

            break;

        case TSDEF_DEF_ERROR_FUNCTION_NOT_INVOCABLE:
            free(info->data.function_not_invocable.name);

            break;
        }
    }
}

static int DecideExpValueTypePrimitive (
                                        struct resolve_state*        state,
                                        struct tsdef_block*          block,
                                        struct tsdef_exp_value_type* exp_value_type
                                       )
{
    struct tsdef_variable*           variable;
    struct tsdef_variable_reference* reference;
    int                              error;

    switch(exp_value_type->type)
    {
    case TSDEF_EXP_VALUE_TYPE_EXP:
        error = DecideExpPrimitive(state, block, exp_value_type->data.exp);
        if(error != CONTINUE_RESOLVE)
            return error;

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        reference = exp_value_type->data.variable;
        variable  = TSDef_LookupVariable(reference->name, block);
        if(variable == NULL)
        {
            struct tsdef_def_error_info info;

            if(state->error_list != NULL)
            {
                info.data.undefined_variable.name = strdup(reference->name);
                if(info.data.undefined_variable.name == NULL)
                    return TSDEF_ERROR_MEMORY;
            }

            HandleError(
                        TSDEF_DEF_ERROR_UNDEFINED_VARIABLE,
                        SEVERITY_ERROR,
                        state,
                        &info
                       );

            return ABORT_STATEMENT;
        }

        reference->variable = variable;

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        error = DecideFunctionCallPrimitive(
                                            state,
                                            block,
                                            exp_value_type->data.function_call,
                                            INVOCABLE_FUNCTION
                                           );
        if(error != CONTINUE_RESOLVE)
            return error;

        break;

    case TSDEF_EXP_VALUE_TYPE_BOOL:
    case TSDEF_EXP_VALUE_TYPE_INT:
    case TSDEF_EXP_VALUE_TYPE_REAL:
    case TSDEF_EXP_VALUE_TYPE_STRING:
        break;
    }

    return CONTINUE_RESOLVE;
}

static int DecideFunctionCallPrimitive (
                                        struct resolve_state*       state,
                                        struct tsdef_block*         block,
                                        struct tsdef_function_call* function_call,
                                        unsigned int                type
                                       )
{
    struct tsdef_argument_types          argument_types;
    struct tsdef_module_object_type_info type_info;
    struct tsdef_exp_list*               arguments;
    struct tsdef_module*                 module;
    struct tsdef_module_object*          module_object;
    struct tsdef_unit*                   typed_unit;
    int                                  error;
    int                                  return_error;

    arguments = function_call->arguments;
    if(arguments != NULL)
    {
        error = DecideExpListPrimitives(state, block, arguments);
        if(error != CONTINUE_RESOLVE)
            return error;
    }

    module = state->module;
    error  = TSDef_ArgumentTypesFromExpList(arguments, &argument_types);
    if(error != TSDEF_ERROR_NONE)
    {
        HandleError(TSDEF_DEF_ERROR_INTERNAL, SEVERITY_ERROR, state, NULL);

        return ABORT_RESOLVE;
    }

    error = TSDef_LookupModuleObject(
                                     function_call->name,
                                     &argument_types,
                                     module,
                                     &module_object,
                                     &type_info
                                    );
    if(error == TSDEF_ARGUMENT_TYPE_MISMATCH)
    {
        struct tsdef_def_error_info info;

        if(state->error_list != NULL)
        {
            char*  argument_name;
            size_t alloc_size;

            alloc_size = strlen(module_object->type.ffi.function_definition->name);
            alloc_size += sizeof("argument")+sizeof("(...), ")+MAX_INT_DIGITS+1;

            argument_name = malloc(alloc_size);
            if(argument_name == NULL)
                return ABORT_RESOLVE;

            sprintf(
                    argument_name,
                    "%s(...), argument%d",
                    module_object->type.ffi.function_definition->name,
                    type_info.argument_index
                   );

            info.data.incompatible_types.convert_to_name = argument_name;

            info.data.incompatible_types.to_type   = type_info.to_type;
            info.data.incompatible_types.from_type = type_info.from_type;
        }

        HandleError(TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES, SEVERITY_WARNING, state, &info);
    }
    else if(error != TSDEF_ERROR_NONE)
    {
        if(error == TSDEF_ERROR_MODULE_OBJECT_NOT_FOUND)
        {
            error = state->module_object_lookup(
                                                function_call->name,
                                                &argument_types,
                                                state->lookup_data,
                                                &module_object
                                               );
        }

        if(error != TSDEF_ERROR_NONE)
        {
            TSDef_DestroyArgumentTypes(&argument_types);

            if(error == TSDEF_ERROR_MODULE_OBJECT_NOT_FOUND)
            {
                struct tsdef_def_error_info info;

                if(state->error_list != NULL)
                {
                    info.data.undefined_function.name = strdup(function_call->name);
                    if(info.data.undefined_function.name == NULL)
                        return ABORT_RESOLVE;
                }

                HandleError(TSDEF_DEF_ERROR_UNDEFINED_FUNCTION, SEVERITY_ERROR, state, &info);

                return ABORT_STATEMENT;
            }
            else if(error == TSDEF_ERROR_MODULE_OBJECT_ARGUMENT_COUNT)
            {
                struct tsdef_def_error_info info;

                if(state->error_list != NULL)
                {
                    info.data.wrong_argument_count.name = strdup(function_call->name);
                    if(info.data.wrong_argument_count.name == NULL)
                        return ABORT_RESOLVE;

                    info.data.wrong_argument_count.passed_count   = argument_types.count;

                    if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT)
                        info.data.wrong_argument_count.required_count = module_object->type.unit->input->input_variables->count;
                    else
                        info.data.wrong_argument_count.required_count = module_object->type.ffi.function_definition->argument_count;
                }

                HandleError(
                            TSDEF_DEF_ERROR_WRONG_ARGUMENT_COUNT,
                            SEVERITY_ERROR,
                            state,
                            &info
                           );

                return ABORT_STATEMENT;
            }

            HandleError(TSDEF_DEF_ERROR_INTERNAL, SEVERITY_ERROR, state, NULL);

            return ABORT_RESOLVE;
        }

        if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT)
            TSDef_AddUnitModuleObject(module_object, module);
    }

    return_error = CONTINUE_RESOLVE;

    if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT)
    {
        if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_TYPED_UNIT)
            function_call->module_object = module_object;
        else
        {
            struct tsdef_module_object* typed_module_object;

            return_error = ABORT_RESOLVE;

            typed_unit = malloc(sizeof(struct tsdef_unit));
            if(typed_unit == NULL)
                goto allocated_typed_unit_failed;

            error = TSDef_CloneUnit(module_object->type.unit, typed_unit);
            if(error != TSDEF_ERROR_NONE)
            {
                HandleError(TSDEF_DEF_ERROR_INTERNAL, SEVERITY_ERROR, state, NULL);

                goto clone_unit_failed;
            }

            return_error = ResolveInputTypes(
                                             typed_unit,
                                             &argument_types,
                                             TSDEF_MODULE_OBJECT_FLAG_FREE_UNIT,
                                             state,
                                             &typed_module_object
                                            );
            if(return_error != CONTINUE_RESOLVE)
                goto resolve_input_types_failed;

            return_error = ResolveOutputType(typed_unit, state);

            function_call->module_object = typed_module_object;
        }

        if(type == ACTIONABLE_FUNCTION)
        {
            struct tsdef_def_error_info info;

            if(state->error_list != NULL)
            {
                info.data.function_not_actionable.name = strdup(module_object->type.unit->name);
                if(info.data.function_not_actionable.name == NULL)
                    return ABORT_RESOLVE;
            }

            HandleError(TSDEF_DEF_ERROR_FUNCTION_NOT_ACTIONABLE, SEVERITY_ERROR, state, &info);
        }
    }
    else
    {
        if(type == INVOCABLE_FUNCTION && module_object->type.ffi.function_definition->function == NULL)
        {
            struct tsdef_def_error_info info;

            if(state->error_list != NULL)
            {
                info.data.function_not_invocable.name = strdup(module_object->type.ffi.function_definition->name);
                if(info.data.function_not_invocable.name == NULL)
                    return ABORT_RESOLVE;
            }

            HandleError(TSDEF_DEF_ERROR_FUNCTION_NOT_INVOCABLE, SEVERITY_ERROR, state, &info);
        }
        else if(type == ACTIONABLE_FUNCTION && module_object->type.ffi.function_definition->action_controller == NULL)
        {
            struct tsdef_def_error_info info;

            if(state->error_list != NULL)
            {
                info.data.function_not_actionable.name = strdup(module_object->type.ffi.function_definition->name);
                if(info.data.function_not_actionable.name == NULL)
                    return ABORT_RESOLVE;
            }

            HandleError(TSDEF_DEF_ERROR_FUNCTION_NOT_ACTIONABLE, SEVERITY_ERROR, state, &info);
        }

        function_call->module_object = module_object;

        TSDef_ReferenceFFI(module_object, state->module);
    }

    TSDef_DestroyArgumentTypes(&argument_types);

    return return_error;

resolve_input_types_failed:
    TSDef_DestroyUnit(typed_unit);
clone_unit_failed:
    free(typed_unit);
allocated_typed_unit_failed:
    TSDef_DestroyArgumentTypes(&argument_types);

    return return_error;
}

static int DecideFunctionCallListPrimitive (
                                            struct resolve_state*            state,
                                            struct tsdef_block*              block,
                                            struct tsdef_function_call_list* function_call_list,
                                            unsigned int                     type
                                           )
{
    struct tsdef_function_call_list_node* node;
    int                                   error;

    for(node = function_call_list->start; node != NULL; node = node->next_function_call)
    {
        error = DecideFunctionCallPrimitive(state, block, node->function_call, type);
        if(error == ABORT_RESOLVE)
            return ABORT_RESOLVE;
    }

    return CONTINUE_RESOLVE;
}

static int DecidePrimaryExpPrimitive (
                                      struct resolve_state*     state,
                                      struct tsdef_block*       block,
                                      struct tsdef_primary_exp* exp
                                     )
{
    struct tsdef_def_error_info    info;
    struct tsdef_primary_exp_node* node;
    unsigned int                   promoted_type;
    unsigned int                   op;
    unsigned int                   used_ops;
    unsigned int                   flag_index;
    int                            op_allowed;
    int                            error;

    node = exp->start;
    op   = node->op;

    error = DecideExpValueTypePrimitive(state, block, node->exp_value_type);
    if(error != CONTINUE_RESOLVE)
        return error;

    promoted_type = TSDef_ExpValuePrimitiveType(node->exp_value_type);
    if(promoted_type == TSDEF_PRIMITIVE_TYPE_DELAYED)
    {
        HandleError(TSDEF_DEF_ERROR_USING_DELAYED_TYPE, SEVERITY_ERROR, state, NULL);

        return ABORT_STATEMENT;
    }
    else if(promoted_type == TSDEF_PRIMITIVE_TYPE_VOID)
    {
        HandleError(TSDEF_DEF_ERROR_USING_VOID_TYPE, SEVERITY_ERROR, state, NULL);

        return ABORT_STATEMENT;
    }

    used_ops = 0;
    for(node = node->remaining_exp; node != NULL; node = node->remaining_exp)
    {
        unsigned int primitive_type;

        error = DecideExpValueTypePrimitive(state, block, node->exp_value_type);
        if(error != CONTINUE_RESOLVE)
            return error;

        primitive_type = TSDef_ExpValuePrimitiveType(node->exp_value_type);
        if(primitive_type == TSDEF_PRIMITIVE_TYPE_DELAYED)
        {
            HandleError(TSDEF_DEF_ERROR_USING_DELAYED_TYPE, SEVERITY_ERROR, state, NULL);

            return ABORT_STATEMENT;
        }
        else if(primitive_type == TSDEF_PRIMITIVE_TYPE_VOID)
        {
            HandleError(TSDEF_DEF_ERROR_USING_VOID_TYPE, SEVERITY_ERROR, state, NULL);

            return ABORT_STATEMENT;
        }

        used_ops     |= 1<<op;
        promoted_type = TSDef_SelectPrimitivePromotion(promoted_type, primitive_type);

        op = node->op;
    }

    for(flag_index = 0; flag_index < MAX_OPERATOR_VALUE && used_ops != 0; flag_index++)
    {
        unsigned int flag;

        op   = flag_index;
        flag = 1<<op;

        if(used_ops&flag)
        {
            op_allowed = TSDef_OpAllowed(promoted_type, op, promoted_type);
            if(op_allowed == TSDEF_OP_DISALLOWED)
                goto invalid_use_of_operator;

            used_ops ^= flag;
        }
    }

    op_allowed = TSDef_OpAllowed(
                                 TSDEF_PRIMITIVE_TYPE_VOID,
                                 TSDEF_PRIMARY_EXP_OP_VALUE,
                                 promoted_type
                                );
    if(op_allowed == TSDEF_OP_DISALLOWED)
    {
        op = TSDEF_PRIMARY_EXP_OP_VALUE;

        goto invalid_use_of_operator;
    }

    if(exp->flags&TSDEF_PRIMARY_EXP_FLAG_NEGATE)
    {
        op         = TSDEF_PRIMARY_EXP_OP_SUB;
        op_allowed = TSDef_OpAllowed(TSDEF_PRIMITIVE_TYPE_VOID, op, promoted_type);
        if(op_allowed == TSDEF_OP_DISALLOWED)
            goto invalid_use_of_operator;
    }

    exp->effective_primitive_type = promoted_type;

    return CONTINUE_RESOLVE;

invalid_use_of_operator:
    info.data.invalid_use_of_operator.operator = op;
    info.data.invalid_use_of_operator.type     = promoted_type;

    HandleError(
                TSDEF_DEF_ERROR_INVALID_USE_OF_OPERATOR,
                SEVERITY_ERROR,
                state,
                &info
               );

    return ABORT_STATEMENT;
}

static int DecideComparisonExpPrimitive (
                                         struct resolve_state*        state,
                                         struct tsdef_block*          block,
                                         struct tsdef_comparison_exp* exp
                                        )
{
    struct tsdef_comparison_exp_node* node;
    struct tsdef_comparison_exp_node* left_node;
    struct tsdef_primary_exp*         left_exp;
    struct tsdef_primary_exp*         right_exp;
    unsigned int                      left_type;
    unsigned int                      right_type;
    int                               error;

    node      = exp->start;
    left_node = node;

    left_exp  = node->left_exp;

    error = DecidePrimaryExpPrimitive(state, block, left_exp);
    if(error != CONTINUE_RESOLVE)
        return error;

    left_type = left_exp->effective_primitive_type;

    for(node = node->remaining_exp; node != NULL; node = node->remaining_exp)
    {
        right_exp = node->left_exp;

        error = DecidePrimaryExpPrimitive(state, block, right_exp);
        if(error != CONTINUE_RESOLVE)
            return error;

        right_type = right_exp->effective_primitive_type;

        left_node->primitive_type = TSDef_SelectPrimitivePromotion(left_type, right_type);

        left_type = right_type;
        left_node = node;
    }

    right_exp = left_node->right_exp;

    error = DecidePrimaryExpPrimitive(state, block, right_exp);
    if(error != CONTINUE_RESOLVE)
        return error;

    right_type = right_exp->effective_primitive_type;

    left_node->primitive_type = TSDef_SelectPrimitivePromotion(left_type, right_type);

    return CONTINUE_RESOLVE;
}

static int DecideLogicalExpPrimitive (
                                      struct resolve_state*     state,
                                      struct tsdef_block*       block,
                                      struct tsdef_logical_exp* exp
                                     )
{
    struct tsdef_logical_exp_node* node;

    for(node = exp->start; node != NULL; node = node->remaining_exp)
    {
        if(node->left_exp != NULL)
        {
            int error;

            error = DecideExpPrimitive(state, block, node->left_exp);
            if(error != CONTINUE_RESOLVE)
                return error;
        }

        if(node->right_exp != NULL)
        {
            int error;

            error = DecideExpPrimitive(state, block, node->right_exp);
            if(error != CONTINUE_RESOLVE)
                return error;
        }
    }

    return CONTINUE_RESOLVE;
}

static int DecideExpPrimitive (
                               struct resolve_state* state,
                               struct tsdef_block*   block,
                               struct tsdef_exp*     exp
                              )
{
    int error;

    switch(exp->type)
    {
    case TSDEF_EXP_TYPE_PRIMARY:
        error = DecidePrimaryExpPrimitive(state, block, exp->data.primary_exp);

        break;

    case TSDEF_EXP_TYPE_COMPARISON:
        error = DecideComparisonExpPrimitive(state, block, exp->data.comparison_exp);

        break;

    case TSDEF_EXP_TYPE_LOGICAL:
        error = DecideLogicalExpPrimitive(state, block, exp->data.logical_exp);

        break;
    }

    return error;
}

static int DecideExpListPrimitives (
                                    struct resolve_state*  state,
                                    struct tsdef_block*    block,
                                    struct tsdef_exp_list* exp_list
                                   )
{
    struct tsdef_exp_list_node* node;
    int                         error;

    for(node = exp_list->start; node != NULL; node = node->next_exp)
    {
        error = DecideExpPrimitive(state, block, node->exp);
        if(error != CONTINUE_RESOLVE)
            return error;
    }

    return CONTINUE_RESOLVE;
}

static int PerformAssignment (
                              struct resolve_state*    state,
                              struct tsdef_block*      block,
                              struct tsdef_assignment* assignment
                             )
{
    struct tsdef_variable_reference* reference;
    struct tsdef_variable*           variable;
    struct tsdef_exp*                exp;
    unsigned int                     exp_primitive_type;
    unsigned int                     variable_type;
    int                              error;

    reference = assignment->lvalue;
    exp       = assignment->rvalue;

    error = DecideExpPrimitive(state, block, exp);
    if(error != CONTINUE_RESOLVE)
        return error;

    variable = TSDef_LookupVariable(reference->name, block);
    if(variable == NULL)
    {
        error = TSDef_DeclareVariable(
                                      reference->name,
                                      block,
                                      &variable
                                     );
        if(error != TSDEF_ERROR_NONE)
        {
            HandleError(TSDEF_DEF_ERROR_INTERNAL, SEVERITY_ERROR, state, NULL);

            return ABORT_RESOLVE;
        }
    }

    reference->variable = variable;

    exp_primitive_type = TSDef_ExpPrimitiveType(exp);
    variable_type      = variable->primitive_type;

    if(variable_type != TSDEF_PRIMITIVE_TYPE_DELAYED)
    {
        int allow_conversion;

        allow_conversion = TSDef_AllowPrimitiveConversion(
                                                          exp_primitive_type,
                                                          variable->primitive_type
                                                         );
        if(allow_conversion == TSDEF_PRIMITIVE_CONVERSION_DISALLOWED)
        {
            struct tsdef_def_error_info info;

            if(state->error_list != NULL)
            {
                info.data.incompatible_types.convert_to_name = strdup(variable->name);
                if(info.data.incompatible_types.convert_to_name == NULL)
                    return ABORT_RESOLVE;

                info.data.incompatible_types.to_type   = variable_type;
                info.data.incompatible_types.from_type = exp_primitive_type;
            }

            HandleError(TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES, SEVERITY_WARNING, state, &info);
        }
    }
    else
        variable->primitive_type = exp_primitive_type;

    return CONTINUE_RESOLVE;
}

static int ProcessFunctionCall (
                                struct resolve_state*       state,
                                struct tsdef_function_call* function_call
                               )
{
    int error;

    error = DecideFunctionCallPrimitive(
                                        state,
                                        state->current_block,
                                        function_call,
                                        INVOCABLE_FUNCTION
                                       );
    if(error == ABORT_RESOLVE)
        return ABORT_RESOLVE;

    state->current_statement = state->current_statement->next_statement;

    return CONTINUE_RESOLVE;
}

static int ProcessAssignment (struct resolve_state* state, struct tsdef_assignment* assignment)
{
    int error;

    error = PerformAssignment(state, state->current_block, assignment);
    if(error == ABORT_RESOLVE)
        return ABORT_RESOLVE;

    state->current_statement = state->current_statement->next_statement;

    return CONTINUE_RESOLVE;
}

static int ProcessIfStatement (struct resolve_state* state, struct tsdef_if_statement* if_statement)
{
    if(if_statement->exp != NULL)
    {
        int error;

        error = DecideExpPrimitive(state, state->current_block, if_statement->exp);
        if(error == ABORT_RESOLVE)
            return ABORT_RESOLVE;
    }

    state->current_statement = if_statement->block.statements;
    state->current_block     = &if_statement->block;

    return CONTINUE_RESOLVE;
}

static int ProcessLoop (struct resolve_state* state, struct tsdef_loop* loop)
{
    struct tsdef_assignment* assignment;
    struct tsdef_exp*        to_exp;
    struct tsdef_variable*   variable;
    unsigned int             to_exp_type;
    unsigned int             variable_type;
    int                      allow_conversion;
    int                      steppable;
    int                      error;

    switch(loop->type)
    {
    case TSDEF_LOOP_TYPE_FOR:
        assignment = loop->data.for_loop.assignment;

        if(assignment != NULL)
        {
            error = PerformAssignment(state, &loop->block, assignment);
            if(error != CONTINUE_RESOLVE)
                goto loop_error;

            variable = loop->data.for_loop.variable->variable;
        }
        else
        {
            struct tsdef_variable_reference* reference;

            reference = loop->data.for_loop.variable;
            variable  = TSDef_LookupVariable(reference->name, state->current_block);
            if(variable == NULL)
            {
                struct tsdef_def_error_info info;

                if(state->error_list != NULL)
                {
                    info.data.undefined_variable.name = strdup(reference->name);
                    if(info.data.undefined_variable.name == NULL)
                        return ABORT_RESOLVE;
                }

                HandleError(
                            TSDEF_DEF_ERROR_UNDEFINED_VARIABLE,
                            SEVERITY_ERROR,
                            state,
                            &info
                           );

                goto abort_statement_error;
            }

            reference->variable = variable;
        }

        to_exp = loop->data.for_loop.to_exp;

        error = DecideExpPrimitive(state, state->current_block, to_exp);
        if(error != CONTINUE_RESOLVE)
            goto loop_error;

        to_exp_type   = TSDef_ExpPrimitiveType(to_exp);
        variable_type = variable->primitive_type;

        allow_conversion = TSDef_AllowPrimitiveConversion(to_exp_type, variable_type);
        if(allow_conversion == TSDEF_PRIMITIVE_CONVERSION_DISALLOWED)
        {
            struct tsdef_def_error_info info;

            if(state->error_list != NULL)
            {
                info.data.incompatible_types.convert_to_name = strdup(loop->data.for_loop.variable->name);
                if(info.data.incompatible_types.convert_to_name == NULL)
                    return ABORT_RESOLVE;

                info.data.incompatible_types.to_type   = variable_type;
                info.data.incompatible_types.from_type = to_exp_type;
            }

            HandleError(TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES, SEVERITY_WARNING, state, &info);
        }

        steppable = TSDef_SteppablePrimitive(variable_type);
        if(steppable == TSDEF_OP_DISALLOWED)
        {
            struct tsdef_def_error_info info;

            if(state->error_list != NULL)
                info.data.type_not_steppable.type = variable_type;

            HandleError(TSDEF_DEF_ERROR_TYPE_NOT_STEPPABLE, SEVERITY_WARNING, state, &info);

            error = ABORT_STATEMENT;

            goto loop_error;
        }

        break;

    case TSDEF_LOOP_TYPE_WHILE:
        error = DecideExpPrimitive(state, state->current_block, loop->data.while_loop.exp);
        if(error != CONTINUE_RESOLVE)
            goto loop_error;

        break;
    }

    state->current_statement = loop->block.statements;
    state->current_block     = &loop->block;

    return CONTINUE_RESOLVE;

abort_statement_error:
    error = ABORT_STATEMENT;

loop_error:
    state->current_statement = loop->block.statements;
    state->current_block     = &loop->block;

    return error;
}

static int ProcessStatements (struct resolve_state* state)
{
    int error;

    while(state->current_statement != NULL)
    {
        struct tsdef_statement* current_statement;

        current_statement       = state->current_statement;
        state->current_location = current_statement->location;

        switch(current_statement->type)
        {
        case TSDEF_STATEMENT_TYPE_FUNCTION_CALL:
            error = ProcessFunctionCall(state, current_statement->data.function_call);

            break;

        case TSDEF_STATEMENT_TYPE_ASSIGNMENT:
            error = ProcessAssignment(state, current_statement->data.assignment);

            break;

        case TSDEF_STATEMENT_TYPE_IF_STATEMENT:
            error = ProcessIfStatement(state, current_statement->data.if_statement);

            break;

        case TSDEF_STATEMENT_TYPE_LOOP:
            error = ProcessLoop(state, current_statement->data.loop);

            break;

        case TSDEF_STATEMENT_TYPE_BREAK:
        case TSDEF_STATEMENT_TYPE_CONTINUE:
        case TSDEF_STATEMENT_TYPE_FINISH:
            error = TSDEF_ERROR_NONE;

            state->current_statement = current_statement->next_statement;

            break;
        }

        if(error == ABORT_RESOLVE)
            return ABORT_RESOLVE;

        while(state->current_statement == NULL && state->current_block != NULL)
        {
            struct tsdef_block*     current_block;
            struct tsdef_statement* parent_statement;

            current_block    = state->current_block;
            parent_statement = current_block->parent_statement;

            state->current_block = current_block->parent_block;

            if(parent_statement != NULL)
                state->current_statement = parent_statement->next_statement;
        }
    }

    return CONTINUE_RESOLVE;
}

static int ProcessActions (struct resolve_state* state)
{
    struct tsdef_action* action;
    struct tsdef_unit*   current_unit;

    current_unit = state->current_unit;

    for(action = current_unit->actions; action != NULL; action = action->next_action)
    {
        int error;

        state->current_location = action->location;

        error = DecideFunctionCallListPrimitive(
                                                state,
                                                &current_unit->global_block,
                                                action->trigger_list,
                                                ACTIONABLE_FUNCTION
                                               );
        if(error != CONTINUE_RESOLVE)
            return error;

        state->current_block     = &action->block;
        state->current_statement = action->block.statements;

        error = ProcessStatements(state);
        if(error != CONTINUE_RESOLVE)
            return error;
    }

    return CONTINUE_RESOLVE;
}

static int ResolveInputTypes (
                              struct tsdef_unit*           unit,
                              struct tsdef_argument_types* argument_types,
                              unsigned int                 module_object_flags,
                              struct resolve_state*        state,
                              struct tsdef_module_object** module_object
                             )
{
    struct tsdef_input*         input;
    struct tsdef_block*         global_block;
    struct tsdef_module_object* allocated_module_object;
    int                         match;
    int                         error;

    input = unit->input;
    if(input != NULL)
        state->current_location = input->location;

    global_block = &unit->global_block;

    match = TSDef_ArgumentCountMatchInput(input, argument_types);
    if(match != TSDEF_ARGUMENT_MATCH)
    {
        struct tsdef_def_error_info info;

        info.data.wrong_argument_count.name = strdup(unit->name);
        if(info.data.wrong_argument_count.name == NULL)
            return ABORT_RESOLVE;

        info.data.wrong_argument_count.passed_count = argument_types->count;

        if(input != NULL)
            info.data.wrong_argument_count.required_count = input->input_variables->count;
        else
            info.data.wrong_argument_count.required_count = 0;

        HandleError(
                    TSDEF_DEF_ERROR_WRONG_ARGUMENT_COUNT,
                    SEVERITY_ERROR,
                    state,
                    &info
                   );

        return ABORT_STATEMENT;
    }

    if(input != NULL)
    {
        struct tsdef_variable_list_node* node;
        unsigned int*                    type;

        type = argument_types->types;

        for(node = input->input_variables->start; node != NULL; node = node->next_variable)
        {
            struct tsdef_variable*           variable;
            struct tsdef_variable_reference* reference;
            char*                            name;

            reference = node->variable;
            name      = reference->name;
            variable  = TSDef_LookupVariable(name, global_block);
            if(variable != NULL)
            {
                struct tsdef_def_error_info info;

                info.data.variable_redefinition.name = strdup(name);
                if(info.data.variable_redefinition.name == NULL)
                    return ABORT_RESOLVE;

                HandleError(
                            TSDEF_DEF_ERROR_VARIABLE_REDEFINITION,
                            SEVERITY_ERROR,
                            state,
                            &info
                           );

                return ABORT_STATEMENT;
            }


            error = TSDef_DeclareVariable(name, global_block, &variable);
            if(error != TSDEF_ERROR_NONE)
            {
                HandleError(TSDEF_DEF_ERROR_INTERNAL, SEVERITY_ERROR, state, NULL);

                return ABORT_RESOLVE;
            }

            variable->primitive_type = *type;

            reference->variable = variable;

            type++;
        }
    }

    allocated_module_object = TSDef_AllocateUnitModuleObject(
                                                             unit,
                                                             TSDEF_MODULE_OBJECT_FLAG_TYPED_UNIT|module_object_flags
                                                            );
    if(allocated_module_object == NULL)
    {
        HandleError(TSDEF_DEF_ERROR_INTERNAL, SEVERITY_ERROR, state, NULL);

        return ABORT_RESOLVE;
    }

    TSDef_AddUnitModuleObject(allocated_module_object, state->module);

    if(module_object != NULL)
        *module_object = allocated_module_object;

    return CONTINUE_RESOLVE;
}

static int ResolveOutputType (struct tsdef_unit* unit, struct resolve_state* state)
{
    struct tsdef_output* output;
    struct tsdef_block*  global_block;

    output = unit->output;
    if(output != NULL)
        state->current_location = output->location;

    global_block = &unit->global_block;

    if(output != NULL)
    {
        int error;

        error = PerformAssignment(state, global_block, output->output_variable_assignment);
        if(error != CONTINUE_RESOLVE)
            return error;
    }

    return CONTINUE_RESOLVE;
}

int TSDef_ResolveUnit (
                       struct tsdef_unit*           unit,
                       struct tsdef_argument_types* arguments,
                       unsigned int                 unit_module_object_flags,
                       struct tsdef_module*         module,
                       tsdef_module_object_lookup   module_object_lookup,
                       void*                        lookup_data,
                       struct tsdef_def_error_list* errors
                      )
{
    struct resolve_state        state;
    struct tsdef_module_object* module_object;
    int                         error;

    state.current_unit         = unit;
    state.current_block        = NULL;
    state.current_statement    = NULL;
    state.module               = module;
    state.module_object_lookup = module_object_lookup;
    state.lookup_data          = lookup_data;
    state.error_list           = errors;
    state.error_count          = 0;
    state.warning_count        = 0;

    error = ResolveInputTypes(
                              unit,
                              arguments,
                              unit_module_object_flags,
                              &state,
                              &module_object
                             );
    if(error != CONTINUE_RESOLVE)
        return TSDEF_ERROR_RESOLVE_INITIALIZE_ERROR;

    error = ResolveOutputType(unit, &state);
    if(error == ABORT_RESOLVE)
        return TSDEF_ERROR_RESOLVE_ERROR;

    while(module_object != NULL)
    {
        state.current_unit      = module_object->type.unit;
        state.current_block     = &state.current_unit->global_block;
        state.current_statement = state.current_unit->global_block.statements;

        error = ProcessStatements(&state);
        if(error != CONTINUE_RESOLVE)
            return TSDEF_ERROR_RESOLVE_ERROR;

        error = ProcessActions(&state);
        if(error != CONTINUE_RESOLVE)
            return TSDEF_ERROR_RESOLVE_ERROR;

        if(state.error_count != 0)
            return TSDEF_ERROR_RESOLVE_ERROR;

        TSDef_MarkUnitResolved(module_object, module);

        module_object = module->unresolved_unit_objects;
    }

    if(state.warning_count != 0)
        return TSDEF_ERROR_RESOLVE_WARNING;

    return TSDEF_ERROR_NONE;
}

