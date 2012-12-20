/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsdef/def.h>
#include <tsdef/ffi.h>
#include <tsdef/module.h>
#include <tsdef/error.h>

#include <stdlib.h>
#include <string.h>


static void InitializeBlock (
                             struct tsdef_block*,
                             struct tsdef_statement*,
                             struct tsdef_block*
                            );
static int  CloneBlock      (
                             struct tsdef_block*,
                             struct tsdef_statement*,
                             struct tsdef_block*,
                             struct tsdef_block*
                            );
static void DestroyBlock    (struct tsdef_block*);

static void RebaseStatementBlockDepth (struct tsdef_block*);

static int CloneAction (struct tsdef_action*, struct tsdef_unit*);


unsigned int tsdef_primary_exp_op_precedence[] = {
                                                  0, /* op value */
                                                  1, /* op add */
                                                  1, /* op sub */
                                                  2, /* op mul */
                                                  2, /* op div */
                                                  2, /* op mod */
                                                  3  /* op pow */
                                                 };

unsigned int tsdef_logical_exp_op_precedence[] = {
                                                  0, /* value */
                                                  1, /* or value */
                                                  2  /* and value */
                                                 };

unsigned int tsdef_primitive_type_rank[] = {
                                            0, /* void */
                                            1, /* delayed */
                                            2, /* bool */
                                            3, /* int */
                                            4, /* real */
                                            5  /* string */
                                           };


static void InitializeBlock (
                             struct tsdef_block*     parent_block,
                             struct tsdef_statement* parent_statement,
                             struct tsdef_block*     block
                            )
{
    if(parent_block == NULL)
        block->depth = 0;
    else
        block->depth = parent_block->depth+1;

    block->variables        = NULL;
    block->variable_count   = 0;
    block->statements       = NULL;
    block->statement_count  = 0;
    block->parent_block     = parent_block;
    block->parent_statement = parent_statement;
}

static int CloneBlock (
                       struct tsdef_block*     original_block,
                       struct tsdef_statement* parent_statement,
                       struct tsdef_block*     parent_block,
                       struct tsdef_block*     cloned_block
                      )
{
    struct tsdef_statement* scan_statements;
    void*                   data;

    InitializeBlock(parent_block, parent_statement, cloned_block);

    for(
        scan_statements = original_block->statements;
        scan_statements != NULL;
        scan_statements = scan_statements->next_statement
       )
    {
        int error;

        switch(scan_statements->type)
        {
        case TSDEF_STATEMENT_TYPE_FUNCTION_CALL:
            data = TSDef_CloneFunctionCall(scan_statements->data.function_call);
            if(data == NULL)
                goto clone_function_call_failed;

            break;

        case TSDEF_STATEMENT_TYPE_ASSIGNMENT:
            data = TSDef_CloneAssignment(scan_statements->data.assignment);
            if(data == NULL)
                goto clone_assignment_failed;

            break;

        case TSDEF_STATEMENT_TYPE_CONTINUE:
        case TSDEF_STATEMENT_TYPE_BREAK:
        case TSDEF_STATEMENT_TYPE_FINISH:
            data = NULL;

            break;

        case TSDEF_STATEMENT_TYPE_IF_STATEMENT:
            data = TSDef_CloneIfStatement(scan_statements->data.if_statement);
            if(data == NULL)
                goto clone_if_statement_failed;

            break;

        case TSDEF_STATEMENT_TYPE_LOOP:
            data = TSDef_CloneLoop(scan_statements->data.loop);
            if(data == NULL)
                goto clone_loop_failed;

            break;
        }

        error = TSDef_AppendStatement(
                                      scan_statements->type,
                                      data,
                                      scan_statements->location,
                                      cloned_block,
                                      NULL
                                     );
        if(error != TSDEF_ERROR_NONE)
            goto clone_statement_failed;
    }

    return TSDEF_ERROR_NONE;

clone_statement_failed:
    switch(scan_statements->type)
    {
    case TSDEF_STATEMENT_TYPE_FUNCTION_CALL:
        TSDef_DestroyFunctionCall(data);

        break;

    case TSDEF_STATEMENT_TYPE_ASSIGNMENT:
        TSDef_DestroyAssignment(data);

        break;

    case TSDEF_STATEMENT_TYPE_CONTINUE:
    case TSDEF_STATEMENT_TYPE_BREAK:
    case TSDEF_STATEMENT_TYPE_FINISH:
        break;

    case TSDEF_STATEMENT_TYPE_IF_STATEMENT:
        TSDef_DestroyIfStatement(data);

        break;

    case TSDEF_STATEMENT_TYPE_LOOP:
        TSDef_DestroyLoop(data);

        break;
    }

clone_loop_failed:
clone_if_statement_failed:
clone_assignment_failed:
clone_function_call_failed:
    DestroyBlock(cloned_block);

    return TSDEF_ERROR_MEMORY;
}

static void DestroyBlock (struct tsdef_block* block)
{
    struct tsdef_statement* statement;
    struct tsdef_variable*  variable;

    statement = block->statements;
    while(statement != NULL)
    {
        struct tsdef_statement* free_statement;

        switch(statement->type)
        {
        case TSDEF_STATEMENT_TYPE_FUNCTION_CALL:
            TSDef_DestroyFunctionCall(statement->data.function_call);

            break;

        case TSDEF_STATEMENT_TYPE_ASSIGNMENT:
            TSDef_DestroyAssignment(statement->data.assignment);

            break;

        case TSDEF_STATEMENT_TYPE_IF_STATEMENT:
            TSDef_DestroyIfStatement(statement->data.if_statement);

            break;

        case TSDEF_STATEMENT_TYPE_LOOP:
            TSDef_DestroyLoop(statement->data.loop);

            break;

        case TSDEF_STATEMENT_TYPE_CONTINUE:
        case TSDEF_STATEMENT_TYPE_BREAK:
        case TSDEF_STATEMENT_TYPE_FINISH:
            break;
        }

        free_statement = statement;
        statement      = statement->next_statement;

        free(free_statement);
    }

    variable = block->variables;
    while(variable != NULL)
    {
        struct tsdef_variable* free_variable;

        free_variable = variable;
        variable      = variable->next_variable;

        free(free_variable->name);
        free(free_variable);
    }
}

static void RebaseStatementBlockDepth (struct tsdef_block* block)
{
    struct tsdef_statement* scan_statements;

    block->depth = block->parent_block->depth+1;

    for(
        scan_statements = block->statements;
        scan_statements != NULL;
        scan_statements = scan_statements->next_statement
       )
    {
        switch(scan_statements->type)
        {
        case TSDEF_STATEMENT_TYPE_FUNCTION_CALL:
        case TSDEF_STATEMENT_TYPE_ASSIGNMENT:
        case TSDEF_STATEMENT_TYPE_CONTINUE:
        case TSDEF_STATEMENT_TYPE_BREAK:
        case TSDEF_STATEMENT_TYPE_FINISH:
            break;

        case TSDEF_STATEMENT_TYPE_IF_STATEMENT:
            RebaseStatementBlockDepth(&scan_statements->data.if_statement->block);

            break;

        case TSDEF_STATEMENT_TYPE_LOOP:
            RebaseStatementBlockDepth(&scan_statements->data.loop->block);

            break;
        }
    }
}

static int CloneAction (struct tsdef_action* action, struct tsdef_unit* unit)
{
    struct tsdef_action* cloned_action;
    int                  error;

    cloned_action = malloc(sizeof(struct tsdef_action));
    if(cloned_action == NULL)
        goto allocate_action_failed;

    cloned_action->trigger_list = TSDef_CloneFunctionCallList(action->trigger_list);
    if(cloned_action->trigger_list == NULL)
        goto clone_trigger_list_failed;

    cloned_action->location = action->location;

    error = CloneBlock(&action->block, NULL, &unit->global_block, &cloned_action->block);
    if(error != TSDEF_ERROR_NONE)
        goto clone_block_failed;

    cloned_action->next_action = unit->actions;
    unit->actions              = cloned_action;

    unit->action_count++;

    return TSDEF_ERROR_NONE;

clone_block_failed:
    TSDef_DestroyFunctionCallList(cloned_action->trigger_list);
clone_trigger_list_failed:
    free(cloned_action);

allocate_action_failed:
    return TSDEF_ERROR_MEMORY;
}


struct tsdef_variable* TSDef_LookupVariable (char* name, struct tsdef_block* block)
{
    struct tsdef_block* scan_scope;

    for(
        scan_scope = block;
        scan_scope != NULL;
        scan_scope = scan_scope->parent_block
       )
    {
        struct tsdef_variable* scan_variables;

        for(
            scan_variables = scan_scope->variables;
            scan_variables != NULL;
            scan_variables = scan_variables->next_variable
           )
        {
            if(strcmp(scan_variables->name, name) == 0)
                return scan_variables;
        }
    }

    return NULL;
}

int TSDef_DeclareVariable (
                           char*                   name,
                           struct tsdef_block*     block,
                           struct tsdef_variable** declared_variable
                          )
{
    struct tsdef_variable* variable;
    char*                  duplicated_name;

    variable = malloc(sizeof(struct tsdef_variable));
    if(variable == NULL)
        goto alloc_variable_failed;

    duplicated_name = strdup(name);
    if(duplicated_name == NULL)
        goto duplicate_name_failed;

    variable->name           = duplicated_name;
    variable->next_variable  = block->variables;
    variable->primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;

    variable->block = block;
    variable->index = block->variable_count;

    block->variables = variable;
    block->variable_count++;

    if(declared_variable != NULL)
        *declared_variable = variable;

    return TSDEF_ERROR_NONE;

duplicate_name_failed:
    free(variable);

alloc_variable_failed:
    return TSDEF_ERROR_MEMORY;
}

int TSDef_ReferenceVariable (char* name, struct tsdef_variable_reference** referenced_variable)
{
    struct tsdef_variable_reference* reference;
    char*                            duplicated_name;

    reference = malloc(sizeof(struct tsdef_variable_reference));
    if(reference == NULL)
        goto alloc_reference_failed;

    duplicated_name = strdup(name);
    if(duplicated_name == NULL)
        goto duplicate_name_failed;

    reference->name     = duplicated_name;
    reference->variable = NULL;

    *referenced_variable = reference;

    return TSDEF_ERROR_NONE;

duplicate_name_failed:
    free(reference);

alloc_reference_failed:
    return TSDEF_ERROR_MEMORY;
}

struct tsdef_variable_reference* TSDef_CloneVariableReference (
                                                               struct tsdef_variable_reference* reference
                                                              )
{
    struct tsdef_variable_reference* clone;

    clone = malloc(sizeof(struct tsdef_variable_reference));
    if(clone == NULL)
        goto allocate_reference_failed;

    clone->name = strdup(reference->name);
    if(clone->name == NULL)
        goto duplicate_name_failed;

    clone->variable = NULL;

    return clone;

duplicate_name_failed:
    free(clone);

allocate_reference_failed:
    return NULL;
}

void TSDef_DestroyVariableReference (struct tsdef_variable_reference* reference)
{
    free(reference->name);
    free(reference);
}

int TSDef_ConstructVariableList (
                                 struct tsdef_variable_reference* variable,
                                 struct tsdef_variable_list*      remaining_list,
                                 struct tsdef_variable_list**     constructed_variable_list
                                )
{
    struct tsdef_variable_list*      variable_list;
    struct tsdef_variable_list_node* node;

    if(remaining_list == NULL)
    {
        variable_list = malloc(sizeof(struct tsdef_variable_list));
        if(variable_list == NULL)
            return TSDEF_ERROR_MEMORY;

        node                = &variable_list->end;
        node->next_variable = NULL;

        variable_list->count = 1;
    }
    else
    {
        variable_list = remaining_list;

        node = malloc(sizeof(struct tsdef_variable_list_node));
        if(node == NULL)
            return TSDEF_ERROR_MEMORY;

        node->next_variable = variable_list->start;

        variable_list->count++;
    }

    node->variable       = variable;
    variable_list->start = node;

    *constructed_variable_list = variable_list;

    return TSDEF_ERROR_NONE;
}

struct tsdef_variable_list* TSDef_CloneVariableList (struct tsdef_variable_list* variable_list)
{
    struct tsdef_variable_list_node* stop_node;
    struct tsdef_variable_list_node* cloned_node;
    struct tsdef_variable_list_node* node;
    struct tsdef_variable_list*      clone;

    clone = malloc(sizeof(struct tsdef_variable_list));
    if(clone == NULL)
        goto allocate_variable_list_failed;

    stop_node = &variable_list->end;
    if(variable_list->start == stop_node)
        clone->start = &clone->end;
    else
    {
        cloned_node = malloc(sizeof(struct tsdef_variable_list_node));
        if(cloned_node == NULL)
            goto allocate_initial_node_failed;

        clone->start = cloned_node;

        node = variable_list->start;
        while(1)
        {
            cloned_node->variable = TSDef_CloneVariableReference(node->variable);
            if(cloned_node->variable == NULL)
                goto clone_variable_failed;

            node = node->next_variable;
            if(node == stop_node)
                break;

            cloned_node->next_variable = malloc(sizeof(struct tsdef_variable_list_node));
            if(cloned_node->next_variable == NULL)
                goto allocate_node_failed;

            cloned_node = cloned_node->next_variable;
        }

        cloned_node->next_variable = &clone->end;
    }

    cloned_node = &clone->end;

    cloned_node->variable = TSDef_CloneVariableReference(variable_list->end.variable);
    if(cloned_node->variable == NULL)
        goto clone_variable_failed;

    cloned_node->next_variable = NULL;

    clone->count = variable_list->count;

    return clone;

allocate_node_failed:
    TSDef_DestroyVariableReference(cloned_node->variable);

clone_variable_failed:
    node = clone->start;
    while(node != cloned_node)
    {
        struct tsdef_variable_list_node* free_node;

        TSDef_DestroyVariableReference(node->variable);

        free_node = node;
        node      = node->next_variable;

        free(free_node);
    }

    if(cloned_node != &clone->end)
        free(cloned_node);

allocate_initial_node_failed:
    free(clone);

allocate_variable_list_failed:
    return NULL;
}

void TSDef_DestroyVariableList (struct tsdef_variable_list* variable_list)
{
    struct tsdef_variable_list_node* node;
    struct tsdef_variable_list_node* stop_node;

    node      = variable_list->start;
    stop_node = &variable_list->end;

    while(node != stop_node)
    {
        struct tsdef_variable_list_node* free_node;

        TSDef_DestroyVariableReference(node->variable);

        free_node = node;
        node      = free_node->next_variable;

        free(free_node);
    }

    TSDef_DestroyVariableReference(node->variable);

    free(variable_list);
}

int TSDef_DefineFunctionCall (
                              char*                        name,
                              struct tsdef_exp_list*       exp_list,
                              struct tsdef_function_call** defined_function_call
                             )
{
    struct tsdef_function_call* function_call;
    char*                       duplicated_name;

    function_call = malloc(sizeof(struct tsdef_function_call));
    if(function_call == NULL)
        goto alloc_function_call_failed;

    duplicated_name = strdup(name);
    if(duplicated_name == NULL)
        goto duplicate_name_failed;

    function_call->name          = duplicated_name;
    function_call->module_object = NULL;
    function_call->arguments     = exp_list;

    *defined_function_call = function_call;

    return TSDEF_ERROR_NONE;

duplicate_name_failed:
    free(function_call);

alloc_function_call_failed:
    return TSDEF_ERROR_MEMORY;
}

struct tsdef_function_call* TSDef_CloneFunctionCall (struct tsdef_function_call* function_call)
{
    struct tsdef_function_call* clone;

    clone = malloc(sizeof(struct tsdef_function_call));
    if(clone == NULL)
        goto allocate_function_call_failed;

    clone->name = strdup(function_call->name);
    if(clone->name == NULL)
        goto duplicate_name_failed;

    clone->module_object = NULL;

    if(function_call->arguments != NULL)
    {
        clone->arguments = TSDef_CloneExpList(function_call->arguments);
        if(clone->arguments == NULL)
            goto duplicate_arguments_failed;
    }
    else
        clone->arguments = NULL;

    return clone;

duplicate_arguments_failed:
    free(clone->name);
duplicate_name_failed:
    free(function_call);

allocate_function_call_failed:
    return NULL;
}

void TSDef_DestroyFunctionCall (struct tsdef_function_call* function_call)
{
    free(function_call->name);

    if(function_call->arguments != NULL)
        TSDef_DestroyExpList(function_call->arguments);

    free(function_call);
}

int TSDef_ConstructFunctionCallList (
                                     struct tsdef_function_call*       function_call,
                                     struct tsdef_function_call_list*  remaining_list,
                                     struct tsdef_function_call_list** constructed_function_call_list
                                    )
{
    struct tsdef_function_call_list*      function_call_list;
    struct tsdef_function_call_list_node* node;

    if(remaining_list == NULL)
    {
        function_call_list = malloc(sizeof(struct tsdef_function_call_list));
        if(function_call_list == NULL)
            return TSDEF_ERROR_MEMORY;

        node                     = &function_call_list->end;
        node->next_function_call = NULL;

        function_call_list->count = 1;
    }
    else
    {
        function_call_list = remaining_list;

        node = malloc(sizeof(struct tsdef_function_call_list_node));
        if(node == NULL)
            return TSDEF_ERROR_MEMORY;

        node->next_function_call = function_call_list->start;

        function_call_list->count++;
    }

    node->function_call       = function_call;
    function_call_list->start = node;

    *constructed_function_call_list = function_call_list;

    return TSDEF_ERROR_NONE;
}

struct tsdef_function_call_list* TSDef_CloneFunctionCallList (struct tsdef_function_call_list* function_call_list)
{
    struct tsdef_function_call_list_node* stop_node;
    struct tsdef_function_call_list_node* cloned_node;
    struct tsdef_function_call_list_node* node;
    struct tsdef_function_call_list*      clone;

    clone = malloc(sizeof(struct tsdef_function_call_list));
    if(clone == NULL)
        goto allocate_function_call_list_failed;

    stop_node = &function_call_list->end;
    if(function_call_list->start == stop_node)
        clone->start = &clone->end;
    else
    {
        cloned_node = malloc(sizeof(struct tsdef_function_call_list_node));
        if(cloned_node == NULL)
            goto allocate_initial_node_failed;

        clone->start = cloned_node;

        node = function_call_list->start;
        while(1)
        {
            cloned_node->function_call = TSDef_CloneFunctionCall(node->function_call);
            if(cloned_node->function_call == NULL)
                goto clone_function_call_failed;

            node = node->next_function_call;
            if(node == stop_node)
                break;

            cloned_node->next_function_call = malloc(sizeof(struct tsdef_function_call_list_node));
            if(cloned_node->next_function_call == NULL)
                goto allocate_node_failed;

            cloned_node = cloned_node->next_function_call;
        }

        cloned_node->next_function_call = &clone->end;
    }

    cloned_node = &clone->end;

    cloned_node->function_call = TSDef_CloneFunctionCall(function_call_list->end.function_call);
    if(cloned_node->function_call == NULL)
        goto clone_function_call_failed;

    cloned_node->next_function_call = NULL;

    clone->count = function_call_list->count;

    return clone;

allocate_node_failed:
    TSDef_DestroyFunctionCall(cloned_node->function_call);

clone_function_call_failed:
    node = clone->start;
    while(node != cloned_node)
    {
        struct tsdef_function_call_list_node* free_node;

        TSDef_DestroyFunctionCall(node->function_call);

        free_node = node;
        node      = node->next_function_call;

        free(free_node);
    }

    if(cloned_node != &clone->end)
        free(cloned_node);

allocate_initial_node_failed:
    free(clone);

allocate_function_call_list_failed:
    return NULL;
}

void TSDef_DestroyFunctionCallList (struct tsdef_function_call_list* function_call_list)
{
    struct tsdef_function_call_list_node* node;
    struct tsdef_function_call_list_node* stop_node;

    node      = function_call_list->start;
    stop_node = &function_call_list->end;

    while(node != stop_node)
    {
        struct tsdef_function_call_list_node* free_node;

        TSDef_DestroyFunctionCall(node->function_call);

        free_node = node;
        node      = free_node->next_function_call;

        free(free_node);
    }

    TSDef_DestroyFunctionCall(node->function_call);

    free(function_call_list);
}

int TSDef_AllowPrimitiveConversion (unsigned int from_type, unsigned int to_type)
{
    unsigned int to_rank;
    unsigned int from_rank;

    if(
       from_type == TSDEF_PRIMITIVE_TYPE_VOID || from_type == TSDEF_PRIMITIVE_TYPE_DELAYED ||
       to_type == TSDEF_PRIMITIVE_TYPE_VOID || to_type == TSDEF_PRIMITIVE_TYPE_DELAYED
      )
    {
        return TSDEF_PRIMITIVE_CONVERSION_DISALLOWED;
    }

    from_rank = tsdef_primitive_type_rank[from_type];
    to_rank   = tsdef_primitive_type_rank[to_type];

    if(from_type != TSDEF_PRIMITIVE_TYPE_STRING && to_rank < from_rank)
        return TSDEF_PRIMITIVE_CONVERSION_DISALLOWED;

    return TSDEF_PRIMITIVE_CONVERSION_ALLOWED;
}

unsigned int TSDef_SelectPrimitivePromotion (unsigned int type1, unsigned int type2)
{
    unsigned int type1_rank;
    unsigned int type2_rank;

    type1_rank = tsdef_primitive_type_rank[type1];
    type2_rank = tsdef_primitive_type_rank[type2];

    if(type1_rank >= type2_rank)
        return type1;

    return type2;
}

int TSDef_OpAllowed (unsigned int left_type, unsigned int op, unsigned int right_type)
{
    if(left_type == TSDEF_PRIMITIVE_TYPE_VOID)
    {
        if(
           op == TSDEF_PRIMARY_EXP_OP_SUB &&
           (right_type == TSDEF_PRIMITIVE_TYPE_INT || right_type == TSDEF_PRIMITIVE_TYPE_REAL)
          )
        {
            return TSDEF_OP_ALLOWED;
        }

        if(op == TSDEF_PRIMARY_EXP_OP_VALUE)
            return TSDEF_OP_ALLOWED;

        return TSDEF_OP_DISALLOWED;
    }

    if(right_type == TSDEF_PRIMITIVE_TYPE_VOID)
        return TSDEF_OP_DISALLOWED;

    if(
       (left_type == TSDEF_PRIMITIVE_TYPE_STRING || right_type == TSDEF_PRIMITIVE_TYPE_STRING) &&
       op != TSDEF_PRIMARY_EXP_OP_ADD
      )
    {
        return TSDEF_OP_DISALLOWED;
    }

    return TSDEF_OP_ALLOWED;
}

int TSDef_SteppablePrimitive (unsigned int primitive_type)
{
    int steppable;

    switch(primitive_type)
    {
    case TSDEF_PRIMITIVE_TYPE_BOOL:
    case TSDEF_PRIMITIVE_TYPE_INT:
    case TSDEF_PRIMITIVE_TYPE_REAL:
        steppable = TSDEF_OP_ALLOWED;

        break;

    case TSDEF_PRIMITIVE_TYPE_VOID:
    case TSDEF_PRIMITIVE_TYPE_DELAYED:
    case TSDEF_PRIMITIVE_TYPE_STRING:
        steppable = TSDEF_OP_DISALLOWED;

        break;
    }

    return steppable;
}

int TSDef_CreateExpValueType (
                              unsigned int                  type,
                              void*                         data,
                              struct tsdef_exp_value_type** created_type
                             )
{
    struct tsdef_exp_value_type* exp_value_type;

    exp_value_type = malloc(sizeof(struct tsdef_exp_value_type));
    if(exp_value_type == NULL)
        return TSDEF_ERROR_MEMORY;

    exp_value_type->type = type;

    switch(type)
    {
    case TSDEF_EXP_VALUE_TYPE_BOOL:
        exp_value_type->data.bool_constant = *(tsdef_bool*)data;

        break;

    case TSDEF_EXP_VALUE_TYPE_INT:
        exp_value_type->data.int_constant = *(tsdef_int*)data;

        break;

    case TSDEF_EXP_VALUE_TYPE_REAL:
        exp_value_type->data.real_constant = *(tsdef_real*)data;

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        exp_value_type->data.string_constant = data;

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        exp_value_type->data.function_call = data;

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        exp_value_type->data.variable = data;

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        exp_value_type->data.exp = data;

        break;
    }

    *created_type = exp_value_type;

    return TSDEF_ERROR_NONE;
}

unsigned int TSDef_ExpValuePrimitiveType  (struct tsdef_exp_value_type* exp_value_type)
{
    struct tsdef_variable*      variable;
    struct tsdef_function_call* function_call;
    struct tsdef_module_object* module_object;
    unsigned int                primitive_type;

    switch(exp_value_type->type)
    {
    case TSDEF_EXP_VALUE_TYPE_BOOL:
        primitive_type = TSDEF_PRIMITIVE_TYPE_BOOL;

        break;

    case TSDEF_EXP_VALUE_TYPE_INT:
        primitive_type = TSDEF_PRIMITIVE_TYPE_INT;

        break;

    case TSDEF_EXP_VALUE_TYPE_REAL:
        primitive_type = TSDEF_PRIMITIVE_TYPE_REAL;

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        primitive_type = TSDEF_PRIMITIVE_TYPE_STRING;

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        function_call = exp_value_type->data.function_call;
        module_object = function_call->module_object;

        if(module_object == NULL)
            primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;
        else
         {
            if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT)
            {
                struct tsdef_unit*   unit;
                struct tsdef_output* output;

                unit   = module_object->type.unit;
                output = unit->output;

                if(output == NULL)
                    primitive_type = TSDEF_PRIMITIVE_TYPE_VOID;
                else
                {
                    struct tsdef_variable_reference* reference;

                    reference = output->output_variable_assignment->lvalue;
                    if(reference->variable == NULL)
                        primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;
                    else
                        primitive_type = reference->variable->primitive_type;
                }
            }
            else
            {
                unsigned int ffi_output_type;

                ffi_output_type = module_object->type.ffi.function_definition->output_type;
                primitive_type  = TSDef_TranslateFFIType(ffi_output_type);
            }
        }

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        variable = exp_value_type->data.variable->variable;
        if(variable == NULL)
            primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;
        else
            primitive_type = variable->primitive_type;

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        primitive_type = TSDef_ExpPrimitiveType(exp_value_type->data.exp);

        break;
    }

    return primitive_type;
}

struct tsdef_exp_value_type* TSDef_CloneExpValueType  (struct tsdef_exp_value_type* exp_value_type)
{
    struct tsdef_exp_value_type* clone;

    clone = malloc(sizeof(struct tsdef_exp_value_type));
    if(clone == NULL)
        goto allocate_value_type_failed;

    clone->type = exp_value_type->type;

    switch(exp_value_type->type)
    {
    case TSDEF_EXP_VALUE_TYPE_BOOL:
        clone->data.bool_constant = exp_value_type->data.bool_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_INT:
        clone->data.int_constant = exp_value_type->data.int_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_REAL:
        clone->data.real_constant = exp_value_type->data.real_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        clone->data.string_constant = strdup(exp_value_type->data.string_constant);
        if(clone->data.string_constant == NULL)
            goto duplicate_string_failed;

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        clone->data.function_call = TSDef_CloneFunctionCall(exp_value_type->data.function_call);
        if(clone->data.function_call == NULL)
            goto clone_type_data_failed;

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        clone->data.variable = TSDef_CloneVariableReference(exp_value_type->data.variable);
        if(clone->data.variable == NULL)
            goto clone_type_data_failed;

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        clone->data.exp = TSDef_CloneExp(exp_value_type->data.exp);
        if(clone->data.exp == NULL)
            goto clone_type_data_failed;

        break;
    }

    return clone;

duplicate_string_failed:
clone_type_data_failed:
    free(clone);

allocate_value_type_failed:
    return NULL;
}

void TSDef_DestroyExpValueType (struct tsdef_exp_value_type* exp_value_type)
{
    switch(exp_value_type->type)
    {

    case TSDEF_EXP_VALUE_TYPE_BOOL:
    case TSDEF_EXP_VALUE_TYPE_INT:
    case TSDEF_EXP_VALUE_TYPE_REAL:
        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        free(exp_value_type->data.string_constant);

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        TSDef_DestroyFunctionCall(exp_value_type->data.function_call);

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        TSDef_DestroyVariableReference(exp_value_type->data.variable);

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        TSDef_DestroyExp(exp_value_type->data.exp);

        break;
    }

    free(exp_value_type);
}

int TSDef_ConstructPrimaryExp (
                               unsigned int                 op,
                               struct tsdef_exp_value_type* exp_value_type,
                               struct tsdef_primary_exp*    remaining_exp,
                               struct tsdef_primary_exp**   constructed_exp
                              )
{
    struct tsdef_primary_exp*      exp;
    struct tsdef_primary_exp_node* node;

    if(remaining_exp == NULL)
    {
        exp = malloc(sizeof(struct tsdef_primary_exp));
        if(exp == NULL)
            return TSDEF_ERROR_MEMORY;

        node                = &exp->end;
        node->remaining_exp = NULL;

        exp->effective_primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;
        exp->flags                    = 0;
    }
    else
    {
        exp = remaining_exp;

        node = malloc(sizeof(struct tsdef_primary_exp_node));
        if(node == NULL)
            return TSDEF_ERROR_MEMORY;

        node->remaining_exp = exp->start;
    }

    node->op             = op;
    node->exp_value_type = exp_value_type;

    exp->start = node;

    *constructed_exp = exp;

    return TSDEF_ERROR_NONE;
}

int TSDef_SetPrimaryExpFlag (unsigned int flags, struct tsdef_primary_exp* exp)
{
    exp->flags |= flags;

    return TSDEF_ERROR_NONE;
}

struct tsdef_primary_exp* TSDef_ClonePrimaryExp (struct tsdef_primary_exp* primary_exp)
{
    struct tsdef_primary_exp_node* stop_node;
    struct tsdef_primary_exp_node* cloned_node;
    struct tsdef_primary_exp_node* node;
    struct tsdef_primary_exp*      clone;

    clone = malloc(sizeof(struct tsdef_primary_exp));
    if(clone == NULL)
        goto allocate_primary_exp_failed;

    stop_node = &primary_exp->end;
    if(primary_exp->start == stop_node)
        clone->start = &clone->end;
    else
    {
        cloned_node = malloc(sizeof(struct tsdef_primary_exp_node));
        if(cloned_node == NULL)
            goto allocate_initial_node_failed;

        clone->start = cloned_node;

        node = primary_exp->start;
        while(1)
        {
            cloned_node->op             = node->op;
            cloned_node->exp_value_type = TSDef_CloneExpValueType(node->exp_value_type);
            if(cloned_node->exp_value_type == NULL)
                goto clone_value_type_failed;

            node = node->remaining_exp;
            if(node == stop_node)
                break;

            cloned_node->remaining_exp = malloc(sizeof(struct tsdef_primary_exp_node));
            if(cloned_node->remaining_exp == NULL)
                goto allocate_node_failed;

            cloned_node = cloned_node->remaining_exp;
        }

        cloned_node->remaining_exp = &clone->end;
    }

    cloned_node = &clone->end;

    cloned_node->op             = primary_exp->end.op;
    cloned_node->exp_value_type = TSDef_CloneExpValueType(primary_exp->end.exp_value_type);
    if(cloned_node->exp_value_type == NULL)
        goto clone_value_type_failed;

    cloned_node->remaining_exp = NULL;

    clone->flags                    = primary_exp->flags;
    clone->effective_primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;

    return clone;

allocate_node_failed:
    TSDef_DestroyExpValueType(cloned_node->exp_value_type);

clone_value_type_failed:
    node = clone->start;
    while(node != cloned_node)
    {
        struct tsdef_primary_exp_node* free_node;

        TSDef_DestroyExpValueType(node->exp_value_type);

        free_node = node;
        node      = node->remaining_exp;

        free(free_node);
    }

    if(cloned_node != &clone->end)
        free(cloned_node);

allocate_initial_node_failed:
    free(clone);

allocate_primary_exp_failed:
    return NULL;
}

void TSDef_DestroyPrimaryExp (struct tsdef_primary_exp* exp)
{
    struct tsdef_primary_exp_node* node;
    struct tsdef_primary_exp_node* stop_node;

    node      = exp->start;
    stop_node = &exp->end;

    while(node != stop_node)
    {
        struct tsdef_primary_exp_node* free_node;

        TSDef_DestroyExpValueType(node->exp_value_type);

        free_node = node;
        node      = node->remaining_exp;

        free(free_node);
    }

    TSDef_DestroyExpValueType(node->exp_value_type);

    free(exp);
}

int TSDef_ConstructComparisonExp (
                                  unsigned int                  op,
                                  struct tsdef_primary_exp*     left_exp,
                                  struct tsdef_primary_exp*     right_exp,
                                  struct tsdef_comparison_exp*  remaining_exp,
                                  struct tsdef_comparison_exp** constructed_exp
                                 )
{
    struct tsdef_comparison_exp*      exp;
    struct tsdef_comparison_exp_node* node;

    if(remaining_exp == NULL)
    {
        exp = malloc(sizeof(struct tsdef_comparison_exp));
        if(exp == NULL)
            return TSDEF_ERROR_MEMORY;

        node                = &exp->end;
        node->remaining_exp = NULL;
    }
    else
    {
        exp = remaining_exp;

        node = malloc(sizeof(struct tsdef_comparison_exp_node));
        if(node == NULL)
            return TSDEF_ERROR_MEMORY;

        node->remaining_exp = exp->start;
    }

    node->op             = op;
    node->primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;
    node->left_exp       = left_exp;
    node->right_exp      = right_exp;

    exp->start = node;

    *constructed_exp = exp;

    return TSDEF_ERROR_NONE;
}

struct tsdef_comparison_exp* TSDef_CloneComparisonExp (struct tsdef_comparison_exp* comparison_exp)
{
    struct tsdef_comparison_exp_node* stop_node;
    struct tsdef_comparison_exp_node* cloned_node;
    struct tsdef_comparison_exp_node* node;
    struct tsdef_comparison_exp*      clone;

    clone = malloc(sizeof(struct tsdef_comparison_exp));
    if(clone == NULL)
        goto allocate_comparison_exp_failed;

    stop_node = &comparison_exp->end;
    if(comparison_exp->start == stop_node)
        clone->start = &clone->end;
    else
    {
        cloned_node = malloc(sizeof(struct tsdef_comparison_exp_node));
        if(cloned_node == NULL)
            goto allocate_initial_node_failed;

        clone->start = cloned_node;

        node = comparison_exp->start;
        while(1)
        {
            cloned_node->op             = node->op;
            cloned_node->primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;
            cloned_node->left_exp       = TSDef_ClonePrimaryExp(node->left_exp);
            if(cloned_node->left_exp== NULL)
                goto clone_primary_exp_failed;

            cloned_node->right_exp = NULL;

            node = node->remaining_exp;
            if(node == stop_node)
                break;

            cloned_node->remaining_exp = malloc(sizeof(struct tsdef_comparison_exp_node));
            if(cloned_node->remaining_exp == NULL)
                goto allocate_node_failed;

            cloned_node = cloned_node->remaining_exp;
        }

        cloned_node->remaining_exp = &clone->end;
    }

    cloned_node = &clone->end;

    cloned_node->op             = comparison_exp->end.op;
    cloned_node->primitive_type = TSDEF_PRIMITIVE_TYPE_DELAYED;
    cloned_node->left_exp       = TSDef_ClonePrimaryExp(comparison_exp->end.left_exp);
    if(cloned_node->left_exp == NULL)
        goto clone_primary_exp_failed;

    cloned_node->right_exp = TSDef_ClonePrimaryExp(comparison_exp->end.right_exp);
    if(cloned_node->right_exp == NULL)
        goto clone_right_exp_failed;

    cloned_node->remaining_exp = NULL;

    return clone;

clone_right_exp_failed:
allocate_node_failed:
    TSDef_DestroyPrimaryExp(cloned_node->left_exp);

clone_primary_exp_failed:
    node = clone->start;
    while(node != cloned_node)
    {
        struct tsdef_comparison_exp_node* free_node;

        TSDef_DestroyPrimaryExp(node->left_exp);

        free_node = node;
        node      = node->remaining_exp;

        free(free_node);
    }

    if(cloned_node != &clone->end)
        free(cloned_node);

allocate_initial_node_failed:
    free(clone);

allocate_comparison_exp_failed:
    return NULL;
}

void TSDef_DestroyComparisonExp  (struct tsdef_comparison_exp* exp)
{
    struct tsdef_comparison_exp_node* node;
    struct tsdef_comparison_exp_node* stop_node;

    node      = exp->start;
    stop_node = &exp->end;

    while(node != stop_node)
    {
        struct tsdef_comparison_exp_node* free_node;

        TSDef_DestroyPrimaryExp(node->left_exp);

        free_node = node;
        node      = node->remaining_exp;

        free(free_node);
    }

    TSDef_DestroyPrimaryExp(node->left_exp);
    TSDef_DestroyPrimaryExp(node->right_exp);

    free(exp);
}

int TSDef_ConstructLogicalExp (
                               unsigned int               op,
                               struct tsdef_exp*          left_exp,
                               unsigned int               left_exp_flags,
                               struct tsdef_exp*          right_exp,
                               unsigned int               right_exp_flags,
                               struct tsdef_logical_exp*  remaining_exp,
                               struct tsdef_logical_exp** constructed_exp
                              )
{
    struct tsdef_logical_exp*      exp;
    struct tsdef_logical_exp_node* node;

    if(remaining_exp == NULL)
    {
        exp = malloc(sizeof(struct tsdef_logical_exp));
        if(exp == NULL)
            return TSDEF_ERROR_MEMORY;

        node       = &exp->end;
        exp->start = node;

        node->op              = op;
        node->left_exp        = left_exp;
        node->left_exp_flags  = left_exp_flags;
        node->right_exp       = right_exp;
        node->right_exp_flags = right_exp_flags;
        node->remaining_exp   = NULL;
    }
    else
    {
        exp = remaining_exp;

        if(exp->start->op == TSDEF_LOGICAL_EXP_OP_VALUE)
        {
            node = exp->start;

            node->op              = op;
            node->right_exp       = node->left_exp;
            node->right_exp_flags = node->left_exp_flags;
            node->left_exp        = left_exp;
            node->left_exp_flags  = left_exp_flags;
        }
        else
        {
            node = malloc(sizeof(struct tsdef_logical_exp_node));
            if(node == NULL)
                return TSDEF_ERROR_MEMORY;

            node->op              = op;
            node->left_exp        = left_exp;
            node->left_exp_flags  = left_exp_flags;
            node->right_exp       = right_exp;
            node->right_exp_flags = right_exp_flags;
            node->remaining_exp   = exp->start;

            exp->start = node;
        }
    }

    *constructed_exp = exp;

    return TSDEF_ERROR_NONE;
}

struct tsdef_logical_exp* TSDef_CloneLogicalExp (struct tsdef_logical_exp* logical_exp)
{
    struct tsdef_logical_exp_node* stop_node;
    struct tsdef_logical_exp_node* cloned_node;
    struct tsdef_logical_exp_node* node;
    struct tsdef_logical_exp*      clone;

    clone = malloc(sizeof(struct tsdef_logical_exp));
    if(clone == NULL)
        goto allocate_logical_exp_failed;

    stop_node = &logical_exp->end;
    if(logical_exp->start == stop_node)
        clone->start = &clone->end;
    else
    {
        cloned_node = malloc(sizeof(struct tsdef_logical_exp_node));
        if(cloned_node == NULL)
            goto allocate_initial_node_failed;

        clone->start = cloned_node;

        node = logical_exp->start;
        while(1)
        {
            cloned_node->op             = node->op;
            cloned_node->left_exp       = TSDef_CloneExp(node->left_exp);
            if(cloned_node->left_exp== NULL)
                goto clone_exp_failed;

            cloned_node->left_exp_flags  = node->left_exp_flags;
            cloned_node->right_exp       = NULL;
            cloned_node->right_exp_flags = 0;

            node = node->remaining_exp;
            if(node == stop_node)
                break;

            cloned_node->remaining_exp = malloc(sizeof(struct tsdef_logical_exp_node));
            if(cloned_node->remaining_exp == NULL)
                goto allocate_node_failed;

            cloned_node = cloned_node->remaining_exp;
        }

        cloned_node->remaining_exp = &clone->end;
    }

    cloned_node = &clone->end;

    cloned_node->op       = logical_exp->end.op;
    cloned_node->left_exp = TSDef_CloneExp(logical_exp->end.left_exp);
    if(cloned_node->left_exp == NULL)
        goto clone_exp_failed;

    cloned_node->left_exp_flags = logical_exp->end.left_exp_flags;

    if(logical_exp->end.right_exp != NULL)
    {
        cloned_node->right_exp = TSDef_CloneExp(logical_exp->end.right_exp);
        if(cloned_node->right_exp == NULL)
            goto clone_right_exp_failed;
    }
    else
        cloned_node->right_exp = NULL;

    cloned_node->right_exp_flags = logical_exp->end.right_exp_flags;
    cloned_node->remaining_exp   = NULL;

    return clone;

clone_right_exp_failed:
allocate_node_failed:
    TSDef_DestroyExp(cloned_node->left_exp);

clone_exp_failed:
    node = clone->start;
    while(node != cloned_node)
    {
        struct tsdef_logical_exp_node* free_node;

        TSDef_DestroyExp(node->left_exp);

        free_node = node;
        node      = node->remaining_exp;

        free(free_node);
    }

    if(cloned_node != &clone->end)
        free(cloned_node);

allocate_initial_node_failed:
    free(clone);

allocate_logical_exp_failed:
    return NULL;
}

void TSDef_DestroyLogicalExp (struct tsdef_logical_exp* exp)
{
    struct tsdef_logical_exp_node* node;
    struct tsdef_logical_exp_node* stop_node;

    node      = exp->start;
    stop_node = &exp->end;

    while(node != stop_node)
    {
        struct tsdef_logical_exp_node* free_node;

        TSDef_DestroyExp(node->left_exp);

        free_node = node;
        node      = node->remaining_exp;

        free(free_node);
    }

    TSDef_DestroyExp(node->left_exp);

    if(node->right_exp != NULL)
        TSDef_DestroyExp(node->right_exp);

    free(exp);
}

int TSDef_CreateExp (
                     unsigned int       type,
                     void*              data,
                     struct tsdef_exp** created_exp
                    )
{
    struct tsdef_exp* exp;

    exp = malloc(sizeof(struct tsdef_exp));
    if(exp == NULL)
        return TSDEF_ERROR_MEMORY;

    exp->type = type;

    switch(type)
    {
    case TSDEF_EXP_TYPE_PRIMARY:
        exp->data.primary_exp = data;

        break;

    case TSDEF_EXP_TYPE_COMPARISON:
        exp->data.comparison_exp = data;

        break;

    case TSDEF_EXP_TYPE_LOGICAL:
        exp->data.logical_exp = data;

        break;
    }

    *created_exp = exp;

    return TSDEF_ERROR_NONE;
}

unsigned int TSDef_ExpPrimitiveType (struct tsdef_exp* exp)
{
    unsigned int primitive_type;

    switch(exp->type)
    {
    case TSDEF_EXP_TYPE_PRIMARY:
        primitive_type = exp->data.primary_exp->effective_primitive_type;

        break;

    case TSDEF_EXP_TYPE_COMPARISON:
        primitive_type = TSDEF_PRIMITIVE_TYPE_BOOL;

        break;

    case TSDEF_EXP_TYPE_LOGICAL:
        primitive_type = TSDEF_PRIMITIVE_TYPE_BOOL;

        break;
    }

    return primitive_type;
}

struct tsdef_exp* TSDef_CloneExp (struct tsdef_exp* exp)
{
    struct tsdef_exp* clone;

    clone = malloc(sizeof(struct tsdef_exp));
    if(clone == NULL)
        return NULL;

    clone->type = exp->type;

    switch(exp->type)
    {
    case TSDEF_EXP_TYPE_PRIMARY:
        clone->data.primary_exp = TSDef_ClonePrimaryExp(exp->data.primary_exp);
        if(clone->data.primary_exp == NULL)
            goto clone_exp_type_failed;

        break;

    case TSDEF_EXP_TYPE_COMPARISON:
        clone->data.comparison_exp = TSDef_CloneComparisonExp(exp->data.comparison_exp);
        if(clone->data.comparison_exp == NULL)
            goto clone_exp_type_failed;

        break;

    case TSDEF_EXP_TYPE_LOGICAL:
        clone->data.logical_exp = TSDef_CloneLogicalExp(exp->data.logical_exp);
        if(clone->data.logical_exp == NULL)
            goto clone_exp_type_failed;

        break;
    }

    return clone;

clone_exp_type_failed:
    free(clone);

    return NULL;
}

void TSDef_DestroyExp (struct tsdef_exp* exp)
{
    switch(exp->type)
    {
    case TSDEF_EXP_TYPE_PRIMARY:
        TSDef_DestroyPrimaryExp(exp->data.primary_exp);

        break;

    case TSDEF_EXP_TYPE_COMPARISON:
        TSDef_DestroyComparisonExp(exp->data.comparison_exp);

        break;

    case TSDEF_EXP_TYPE_LOGICAL:
        TSDef_DestroyLogicalExp(exp->data.logical_exp);

        break;
    }

    free(exp);
}

int TSDef_ConstructExpList (
                            struct tsdef_exp*       exp,
                            struct tsdef_exp_list*  remaining_list,
                            struct tsdef_exp_list** constructed_exp_list
                           )
{
    struct tsdef_exp_list*      exp_list;
    struct tsdef_exp_list_node* node;

    if(remaining_list == NULL)
    {
        exp_list = malloc(sizeof(struct tsdef_exp_list));
        if(exp_list == NULL)
            return TSDEF_ERROR_MEMORY;

        node           = &exp_list->end;
        node->next_exp = NULL;

        exp_list->count = 1;
    }
    else
    {
        exp_list = remaining_list;

        node = malloc(sizeof(struct tsdef_exp_list_node));
        if(node == NULL)
            return TSDEF_ERROR_MEMORY;

        node->next_exp = exp_list->start;

        exp_list->count++;
    }

    node->exp       = exp;
    exp_list->start = node;

    *constructed_exp_list = exp_list;

    return TSDEF_ERROR_NONE;
}

struct tsdef_exp_list* TSDef_CloneExpList (struct tsdef_exp_list* exp_list)
{
    struct tsdef_exp_list_node* stop_node;
    struct tsdef_exp_list_node* cloned_node;
    struct tsdef_exp_list_node* node;
    struct tsdef_exp_list*      clone;

    clone = malloc(sizeof(struct tsdef_exp_list));
    if(clone == NULL)
        goto allocate_exp_list_failed;

    stop_node = &exp_list->end;
    if(exp_list->start == stop_node)
        clone->start = &clone->end;
    else
    {
        cloned_node = malloc(sizeof(struct tsdef_exp_list_node));
        if(cloned_node == NULL)
            goto allocate_initial_node_failed;

        clone->start = cloned_node;

        node = exp_list->start;
        while(1)
        {
            cloned_node->exp = TSDef_CloneExp(node->exp);
            if(cloned_node->exp == NULL)
                goto clone_exp_failed;

            node = node->next_exp;
            if(node == stop_node)
                break;

            cloned_node->next_exp = malloc(sizeof(struct tsdef_exp_list_node));
            if(cloned_node->next_exp == NULL)
                goto allocate_node_failed;

            cloned_node = cloned_node->next_exp;
        }

        cloned_node->next_exp = &clone->end;
    }

    cloned_node = &clone->end;

    cloned_node->exp = TSDef_CloneExp(exp_list->end.exp);
    if(cloned_node->exp == NULL)
        goto clone_exp_failed;

    cloned_node->next_exp = NULL;

    clone->count = exp_list->count;

    return clone;

allocate_node_failed:
    TSDef_DestroyExp(cloned_node->exp);

clone_exp_failed:
    node = clone->start;
    while(node != cloned_node)
    {
        struct tsdef_exp_list_node* free_node;

        TSDef_DestroyExp(node->exp);

        free_node = node;
        node      = node->next_exp;

        free(free_node);
    }

    if(cloned_node != &clone->end)
        free(cloned_node);

allocate_initial_node_failed:
    free(clone);

allocate_exp_list_failed:
    return NULL;
}

void TSDef_DestroyExpList (struct tsdef_exp_list* exp_list)
{
    struct tsdef_exp_list_node* node;
    struct tsdef_exp_list_node* stop_node;

    node      = exp_list->start;
    stop_node = &exp_list->end;

    while(node != stop_node)
    {
        struct tsdef_exp_list_node* free_node;

        TSDef_DestroyExp(node->exp);

        free_node = node;
        node      = free_node->next_exp;

        free(free_node);
    }

    TSDef_DestroyExp(node->exp);

    free(exp_list);
}

int TSDef_DefineAssignment (
                            struct tsdef_variable_reference* variable,
                            struct tsdef_exp*                exp,
                            struct tsdef_assignment**        defined_assignment
                           )
{
    struct tsdef_assignment* assignment;

    assignment = malloc(sizeof(struct tsdef_assignment));
    if(assignment == NULL)
        return TSDEF_ERROR_MEMORY;

    assignment->lvalue = variable;
    assignment->rvalue = exp;

    *defined_assignment = assignment;

    return TSDEF_ERROR_NONE;
}

struct tsdef_assignment* TSDef_CloneAssignment (struct tsdef_assignment* assignment)
{
    struct tsdef_assignment* clone;

    clone = malloc(sizeof(struct tsdef_assignment));
    if(clone == NULL)
        goto allocate_clone_failed;

    clone->lvalue = TSDef_CloneVariableReference(assignment->lvalue);
    if(clone->lvalue == NULL)
        goto clone_lvalue_failed;

    clone->rvalue = TSDef_CloneExp(assignment->rvalue);
    if(clone->rvalue == NULL)
        goto clone_rvalue_failed;

    return clone;

clone_rvalue_failed:
    TSDef_DestroyVariableReference(clone->lvalue);
clone_lvalue_failed:
    free(clone);

allocate_clone_failed:
    return NULL;
}

void TSDef_DestroyAssignment (struct tsdef_assignment* assignment)
{
    TSDef_DestroyVariableReference(assignment->lvalue);
    TSDef_DestroyExp(assignment->rvalue);

    free(assignment);
}

int TSDef_DefineIfStatement (
                             struct tsdef_exp*           exp,
                             unsigned int                flags,
                             struct tsdef_if_statement** constructed_if
                            )
{
    struct tsdef_if_statement* if_statement;

    if_statement = malloc(sizeof(struct tsdef_if_statement));
    if(if_statement == NULL)
        return TSDEF_ERROR_MEMORY;

    if_statement->exp   = exp;
    if_statement->flags = flags;

    InitializeBlock(NULL, NULL, &if_statement->block);

    *constructed_if = if_statement;

    return TSDEF_ERROR_NONE;
}

struct tsdef_if_statement* TSDef_CloneIfStatement (struct tsdef_if_statement* if_statement)
{
    struct tsdef_if_statement* clone;
    int                        error;

    clone = malloc(sizeof(struct tsdef_if_statement));
    if(clone == NULL)
        goto allocate_clone_failed;

    if(if_statement->exp != NULL)
    {
        clone->exp = TSDef_CloneExp(if_statement->exp);
        if(clone->exp == NULL)
            goto clone_exp_failed;
    }
    else
        clone->exp = NULL;

    clone->flags = if_statement->flags;

    error = CloneBlock(&if_statement->block, NULL, NULL, &clone->block);
    if(error != TSDEF_ERROR_NONE)
        goto clone_block_failed;

    return clone;

clone_block_failed:
    TSDef_DestroyExp(clone->exp);
clone_exp_failed:
    free(clone);

allocate_clone_failed:
    return NULL;
}

void TSDef_DestroyIfStatement (struct tsdef_if_statement* if_statement)
{
    if(if_statement->exp != NULL)
        TSDef_DestroyExp(if_statement->exp);

    DestroyBlock(&if_statement->block);

    free(if_statement);
}

int TSDef_DeclareLoop (struct tsdef_loop** declared_loop)
{
    struct tsdef_loop* loop;

    loop = malloc(sizeof(struct tsdef_loop));
    if(loop == NULL)
        return TSDEF_ERROR_MEMORY;

    loop->type = 0;

    InitializeBlock(NULL, NULL, &loop->block);

    *declared_loop = loop;

    return TSDEF_ERROR_NONE;
}

void TSDef_MakeLoopWhile (
                          struct tsdef_exp*  exp,
                          struct tsdef_loop* loop
                         )
{
    loop->type = TSDEF_LOOP_TYPE_WHILE;

    loop->data.while_loop.exp = exp;
}

void TSDef_MakeLoopFor (
                        struct tsdef_variable_reference* variable,
                        struct tsdef_assignment*         assignment,
                        struct tsdef_exp*                to_exp,
                        unsigned int                     flags,
                        struct tsdef_loop*               loop
                       )
{
    struct tsdef_for_loop*           for_loop;
    struct tsdef_variable_reference* loop_variable;

    loop->type = TSDEF_LOOP_TYPE_FOR;

    for_loop = &loop->data.for_loop;

    if(variable == NULL)
        loop_variable = assignment->lvalue;
    else
        loop_variable = variable;

    for_loop->variable   = loop_variable;
    for_loop->assignment = assignment;
    for_loop->to_exp     = to_exp;
    for_loop->flags      = flags;
}

struct tsdef_loop* TSDef_CloneLoop (struct tsdef_loop* loop)
{
    struct tsdef_loop* clone;
    int                error;

    clone = malloc(sizeof(struct tsdef_loop));
    if(clone == NULL)
        goto allocate_clone_failed;

    error = CloneBlock(&loop->block, NULL, NULL, &clone->block);
    if(error != TSDEF_ERROR_NONE)
        goto clone_block_failed;

    clone->type = loop->type;

    switch(loop->type)
    {
    case TSDEF_LOOP_TYPE_FOR:
        if(loop->data.for_loop.assignment != NULL)
        {
            clone->data.for_loop.assignment = TSDef_CloneAssignment(loop->data.for_loop.assignment);
            if(clone->data.for_loop.assignment == NULL)
                goto clone_for_loop_assignment_failed;

            clone->data.for_loop.variable = clone->data.for_loop.assignment->lvalue;
        }
        else
        {
            clone->data.for_loop.assignment = NULL;
            clone->data.for_loop.variable   = TSDef_CloneVariableReference(loop->data.for_loop.variable);
            if(clone->data.for_loop.variable == NULL)
                goto clone_for_loop_reference_failed;
        }

        clone->data.for_loop.to_exp = TSDef_CloneExp(loop->data.for_loop.to_exp);
        if(clone->data.for_loop.to_exp == NULL)
            goto clone_for_loop_exp_failed;

        clone->data.for_loop.flags = loop->data.for_loop.flags;

        break;

    case TSDEF_LOOP_TYPE_WHILE:
        clone->data.while_loop.exp = TSDef_CloneExp(loop->data.while_loop.exp);
        if(clone->data.while_loop.exp == NULL)
            goto clone_while_loop_exp_failed;

        break;
    }

    return clone;

clone_for_loop_exp_failed:
    if(clone->data.for_loop.assignment != NULL)
        TSDef_DestroyAssignment(clone->data.for_loop.assignment);
    else
        TSDef_DestroyVariableReference(clone->data.for_loop.variable);

clone_while_loop_exp_failed:
clone_for_loop_reference_failed:
clone_for_loop_assignment_failed:
    DestroyBlock(&clone->block);
clone_block_failed:
    free(clone);

allocate_clone_failed:
    return NULL;
}

void TSDef_DestroyLoop (struct tsdef_loop* loop)
{
    switch(loop->type)
    {
    case TSDEF_LOOP_TYPE_FOR:
        if(loop->data.for_loop.assignment != NULL)
            TSDef_DestroyAssignment(loop->data.for_loop.assignment);
        else
            TSDef_DestroyVariableReference(loop->data.for_loop.variable);

        TSDef_DestroyExp(loop->data.for_loop.to_exp);

        break;

    case TSDEF_LOOP_TYPE_WHILE:
        TSDef_DestroyExp(loop->data.while_loop.exp);

        break;
    }

    DestroyBlock(&loop->block);

    free(loop);
}

int TSDef_AppendStatement (
                           unsigned int             type,
                           void*                    data,
                           unsigned int             location,
                           struct tsdef_block*      block,
                           struct tsdef_statement** appended_statement
                          )
{
    struct tsdef_statement* statement;
    struct tsdef_block*     statement_block;

    statement = malloc(sizeof(struct tsdef_statement));
    if(statement == NULL)
        return TSDEF_ERROR_MEMORY;

    statement->type = type;

    switch(type)
    {
    case TSDEF_STATEMENT_TYPE_FUNCTION_CALL:
        statement->data.function_call = data;

        statement_block = NULL;

        break;

    case TSDEF_STATEMENT_TYPE_ASSIGNMENT:
        statement->data.assignment = data;

        statement_block = NULL;

        break;

    case TSDEF_STATEMENT_TYPE_CONTINUE:
    case TSDEF_STATEMENT_TYPE_BREAK:
    case TSDEF_STATEMENT_TYPE_FINISH:
        statement_block = NULL;

        break;

    case TSDEF_STATEMENT_TYPE_IF_STATEMENT:
        statement->data.if_statement = data;
        statement_block              = &statement->data.if_statement->block;

        break;

    case TSDEF_STATEMENT_TYPE_LOOP:
        statement->data.loop = data;
        statement_block      = &statement->data.loop->block;

        break;
    }

    statement->location = location;

    if(statement_block != NULL)
    {
        statement_block->parent_block     = block;
        statement_block->parent_statement = statement;

        RebaseStatementBlockDepth(statement_block);
    }

    statement->next_statement = NULL;

    if(block->statements != NULL)
    {
        block->last_statement->next_statement = statement;
        block->last_statement                 = statement;
    }
    else
    {
        block->statements     = statement;
        block->last_statement = statement;
    }

    block->statement_count++;

    if(appended_statement != NULL)
        *appended_statement = statement;

    return TSDEF_ERROR_NONE;
}

int TSDef_AddAction (
                     struct tsdef_function_call_list* function_call_list,
                     unsigned int                     location,
                     struct tsdef_unit*               unit,
                     struct tsdef_action**            added_action
                    )
{
    struct tsdef_action* action;

    action = malloc(sizeof(struct tsdef_action));
    if(action == NULL)
        return TSDEF_ERROR_MEMORY;

    action->trigger_list = function_call_list;
    action->location     = location;

    InitializeBlock(&unit->global_block, NULL, &action->block);

    action->next_action = unit->actions;
    unit->actions       = action;

    unit->action_count++;

    if(added_action != NULL)
        *added_action = action;

    return TSDEF_ERROR_NONE;
}

int TSDef_DefineInput (
                       struct tsdef_variable_list* variable_list,
                       unsigned int                location,
                       struct tsdef_unit*          unit,
                       struct tsdef_input**        defined_input
                      )
{
    struct tsdef_input* input;

    input = malloc(sizeof(struct tsdef_input));
    if(input == NULL)
        return TSDEF_ERROR_MEMORY;

    input->input_variables = variable_list;
    input->location        = location;

    unit->input = input;

    if(defined_input != NULL)
        *defined_input = input;

    return TSDEF_ERROR_NONE;
}

int TSDef_DefineOutput (
                        struct tsdef_assignment* assignment,
                        unsigned int             location,
                        struct tsdef_unit*       unit,
                        struct tsdef_output**    defined_output
                       )
{
    struct tsdef_output* output;

    output = malloc(sizeof(struct tsdef_output));
    if(output == NULL)
        return TSDEF_ERROR_MEMORY;

    output->output_variable_assignment = assignment;
    output->location                   = location;

    unit->output = output;

    if(defined_output != NULL)
        *defined_output = output;

    return TSDEF_ERROR_NONE;
}

int TSDef_InitializeUnit (char* name, struct tsdef_unit* unit)
{
    unit->name = strdup(name);
    if(unit->name == NULL)
        return TSDEF_ERROR_MEMORY;

    unit->unit_id = 0;
    unit->input   = NULL;
    unit->output  = NULL;

    InitializeBlock(NULL, NULL, &unit->global_block);

    unit->actions      = NULL;
    unit->action_count = 0;

    return TSDEF_ERROR_NONE;
}

int TSDef_CloneUnit (struct tsdef_unit* original_unit, struct tsdef_unit* cloned_unit)
{
    struct tsdef_action*        action;
    struct tsdef_variable_list* cloned_variable_list;
    struct tsdef_assignment*    cloned_assignment;
    struct tsdef_input*         input;
    struct tsdef_output*        output;
    int                         error;

    cloned_unit->name = strdup(original_unit->name);
    if(cloned_unit->name == NULL)
        goto duplicate_name_failed;

    cloned_unit->unit_id = 0;

    input = original_unit->input;
    if(input != NULL)
    {
        cloned_variable_list = TSDef_CloneVariableList(input->input_variables);
        if(cloned_variable_list == NULL)
            goto clone_input_variables_failed;

        error = TSDef_DefineInput(cloned_variable_list, input->location, cloned_unit, NULL);
        if(error != TSDEF_ERROR_NONE)
            goto define_input_failed;
    }
    else
        cloned_unit->input = NULL;

    output = original_unit->output;
    if(output != NULL)
    {
        cloned_assignment = TSDef_CloneAssignment(output->output_variable_assignment);
        if(cloned_assignment == NULL)
            goto clone_assignment_failed;

        error = TSDef_DefineOutput(cloned_assignment, output->location, cloned_unit, NULL);
        if(error != TSDEF_ERROR_NONE)
            goto define_output_failed;
    }
    else
        cloned_unit->output = NULL;

    error = CloneBlock(&original_unit->global_block, NULL, NULL, &cloned_unit->global_block);
    if(error != TSDEF_ERROR_NONE)
        goto clone_block_failed;

    cloned_unit->actions      = NULL;
    cloned_unit->action_count = 0;
    for(action = original_unit->actions; action != NULL; action = action->next_action)
    {
        error = CloneAction(action, cloned_unit);
        if(error != TSDEF_ERROR_NONE)
            goto clone_action_failed;
    }

    return TSDEF_ERROR_NONE;

clone_action_failed:
    action = cloned_unit->actions;
    while(action != NULL)
    {
        struct tsdef_action* free_action;

        TSDef_DestroyFunctionCallList(action->trigger_list);
        DestroyBlock(&action->block);

        free_action = action;
        action      = action->next_action;

        free(free_action);
    }

clone_block_failed:
    if(output != NULL)
        free(cloned_unit->output);
define_output_failed:
    if(output != NULL)
        TSDef_DestroyAssignment(cloned_assignment);
clone_assignment_failed:
    if(input != NULL)
        free(cloned_unit->input);
define_input_failed:
    if(input != NULL)
        TSDef_DestroyVariableList(cloned_variable_list);
clone_input_variables_failed:
    free(cloned_unit->name);

duplicate_name_failed:
    return TSDEF_ERROR_MEMORY;
}

void TSDef_DestroyUnit (struct tsdef_unit* unit)
{
    struct tsdef_action* action;
    struct tsdef_input*  input;
    struct tsdef_output* output;

    free(unit->name);

    input = unit->input;
    if(input != NULL)
    {
        TSDef_DestroyVariableList(input->input_variables);
        free(input);
    }

    output = unit->output;
    if(output != NULL)
    {
        TSDef_DestroyAssignment(output->output_variable_assignment);
        free(output);
    }

    DestroyBlock(&unit->global_block);

    action = unit->actions;
    while(action != NULL)
    {
        struct tsdef_action* free_action;

        TSDef_DestroyFunctionCallList(action->trigger_list);
        DestroyBlock(&action->block);

        free_action = action;
        action      = action->next_action;

        free(free_action);
    }
}

