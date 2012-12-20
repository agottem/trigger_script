/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "expeval.h"
#include "expvalue.h"
#include "expop.h"

#include <tsint/error.h>
#include <tsint/exception.h>
#include <tsint/value.h>

#include <stdlib.h>


static int PartialPrimaryExpEvaluation (
                                        tsint_extract_exp_value_if,
                                        tsint_exp_primary_op_if,
                                        unsigned int,
                                        struct tsdef_primary_exp_node**,
                                        unsigned int,
                                        struct tsint_unit_state*,
                                        union tsint_value*
                                       );

static int PartialLogicalExpEvaluation (
                                        struct tsdef_logical_exp_node**,
                                        unsigned int,
                                        struct tsint_unit_state*,
                                        union tsint_value*
                                       );


static int PartialPrimaryExpEvaluation (
                                        tsint_extract_exp_value_if      extract_func,
                                        tsint_exp_primary_op_if         op_func,
                                        unsigned int                    primitive_type,
                                        struct tsdef_primary_exp_node** processed_node,
                                        unsigned int                    precedence_min,
                                        struct tsint_unit_state*        state,
                                        union tsint_value*              result
                                       )
{
    union tsint_value              left_value;
    union tsint_value              right_value;
    struct tsdef_primary_exp_node* current_node;
    unsigned int                   current_precedence;
    int                            exception;

    current_node = *processed_node;

    exception = extract_func(current_node->exp_value_type, state, &left_value);
    if(exception != TSINT_EXCEPTION_NONE)
        goto extract_left_failed;

    if(current_node->op == TSDEF_PRIMARY_EXP_OP_VALUE)
    {
        exception = op_func(TSDEF_PRIMARY_EXP_OP_VALUE, left_value, NULL, result);
        if(exception != TSINT_EXCEPTION_NONE)
            goto value_op_failed;

        TSInt_DestroyValue(left_value, primitive_type);

        *processed_node = NULL;

        return TSINT_EXCEPTION_NONE;
    }

    do
    {
        union tsint_value              op_result;
        struct tsdef_primary_exp_node* next_node;
        unsigned int                   next_precedence;

        current_precedence = tsdef_primary_exp_op_precedence[current_node->op];
        next_node          = current_node->remaining_exp;

        if(current_precedence < precedence_min)
            break;

        next_precedence = tsdef_primary_exp_op_precedence[next_node->op];

        if(next_precedence > current_precedence)
        {
            exception = PartialPrimaryExpEvaluation(
                                                    extract_func,
                                                    op_func,
                                                    primitive_type,
                                                    &next_node,
                                                    current_precedence+1,
                                                    state,
                                                    &right_value
                                                   );
            if(exception != TSINT_EXCEPTION_NONE)
                goto extract_right_failed;
        }
        else if(
                next_node->op == TSDEF_PRIMARY_EXP_OP_POW &&
                current_node->op == TSDEF_PRIMARY_EXP_OP_POW
               )
        {
            exception = PartialPrimaryExpEvaluation(
                                                    extract_func,
                                                    op_func,
                                                    primitive_type,
                                                    &next_node,
                                                    current_precedence,
                                                    state,
                                                    &right_value
                                                   );
            if(exception != TSINT_EXCEPTION_NONE)
                goto extract_right_failed;
        }
        else
        {
            exception = extract_func(next_node->exp_value_type, state, &right_value);
            if(exception != TSINT_EXCEPTION_NONE)
                goto extract_right_failed;
        }

        exception = op_func(current_node->op, left_value, &right_value, &op_result);
        if(exception != TSINT_EXCEPTION_NONE)
            goto left_right_op_failed;

        TSInt_DestroyValue(left_value, primitive_type);
        TSInt_DestroyValue(right_value, primitive_type);

        left_value = op_result;

        current_node = next_node;
    }while(current_node->remaining_exp != NULL);

    *processed_node = current_node;
    *result         = left_value;

    return TSINT_EXCEPTION_NONE;

left_right_op_failed:
    TSInt_DestroyValue(right_value, primitive_type);
extract_right_failed:
value_op_failed:
    TSInt_DestroyValue(left_value, primitive_type);

extract_left_failed:
    return exception;
}

static int PartialLogicalExpEvaluation (
                                        struct tsdef_logical_exp_node** processed_node,
                                        unsigned int                    precedence_min,
                                        struct tsint_unit_state*        state,
                                        union tsint_value*              result
                                       )
{
    union tsint_value              left_value;
    struct tsdef_logical_exp_node* current_node;
    int                            exception;

    current_node = *processed_node;

    exception = TSInt_EvaluateExp(
                                  TSDEF_PRIMITIVE_TYPE_BOOL,
                                  current_node->left_exp,
                                  state,
                                  &left_value
                                 );
    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    if(current_node->left_exp_flags&TSDEF_LOGICAL_EXP_FLAG_NOT)
        left_value.bool_data ^= TSDEF_BOOL_TRUE;

    if(current_node->op == TSDEF_LOGICAL_EXP_OP_VALUE)
    {
        *processed_node = NULL;
        *result         = left_value;

        return TSINT_EXCEPTION_NONE;
    }

    do
    {
        struct tsdef_logical_exp_node* next_node;
        unsigned int                   current_precedence;

        current_precedence = tsdef_logical_exp_op_precedence[current_node->op];
        if(current_precedence < precedence_min)
            break;

        next_node = current_node->remaining_exp;

        if(
           current_node->op == TSDEF_LOGICAL_EXP_OP_OR && left_value.bool_data == TSDEF_BOOL_TRUE ||
           current_node->op == TSDEF_LOGICAL_EXP_OP_AND && left_value.bool_data == TSDEF_BOOL_FALSE
          )
        {
            unsigned int next_precedence;

            if(next_node != NULL)
            {
                do
                {
                    next_precedence = tsdef_logical_exp_op_precedence[next_node->op];
                    current_node    = next_node;
                    next_node       = current_node->remaining_exp;
                }while(next_node != NULL && next_precedence >= current_precedence);
            }

            if(next_node == NULL)
            {
                *result         = left_value;
                *processed_node = NULL;

                return TSINT_EXCEPTION_NONE;
            }

            continue;
        }

        TSInt_DestroyValue(left_value, TSDEF_PRIMITIVE_TYPE_BOOL);

        if(next_node == NULL)
        {
            exception = TSInt_EvaluateExp(
                                          TSDEF_PRIMITIVE_TYPE_BOOL,
                                          current_node->right_exp,
                                          state,
                                          &left_value
                                         );
            if(exception != TSINT_EXCEPTION_NONE)
                return exception;

            if(current_node->right_exp_flags&TSDEF_LOGICAL_EXP_FLAG_NOT)
                left_value.bool_data ^= TSDEF_BOOL_TRUE;
        }
        else
        {
            unsigned int next_precedence;

            next_precedence = tsdef_logical_exp_op_precedence[next_node->op];
            if(next_precedence > current_precedence)
            {
                exception = PartialLogicalExpEvaluation(
                                                        &next_node,
                                                        current_precedence+1,
                                                        state,
                                                        &left_value
                                                       );
                if(exception != TSINT_EXCEPTION_NONE)
                    return exception;
            }
            else
            {
                exception = TSInt_EvaluateExp(
                                              TSDEF_PRIMITIVE_TYPE_BOOL,
                                              next_node->left_exp,
                                              state,
                                              &left_value
                                             );
                if(exception != TSINT_EXCEPTION_NONE)
                    return exception;

                if(current_node->left_exp_flags&TSDEF_LOGICAL_EXP_FLAG_NOT)
                    left_value.bool_data ^= TSDEF_BOOL_TRUE;
            }
        }

        current_node = next_node;
    }while(current_node != NULL);

    *processed_node = current_node;
    *result         = left_value;

    return TSINT_EXCEPTION_NONE;
}


int TSInt_PrimaryExpEvaluation (
                                struct tsdef_primary_exp* exp,
                                struct tsint_unit_state*  state,
                                union tsint_value*        result
                               )
{
    tsint_extract_exp_value_if     extract_func;
    tsint_exp_primary_op_if        op_func;
    union tsint_value              exp_value;
    struct tsdef_primary_exp_node* node;
    unsigned int                   exp_primitive_type;
    int                            exception;

    exp_primitive_type = exp->effective_primitive_type;
    switch(exp_primitive_type)
    {
    case TSDEF_PRIMITIVE_TYPE_BOOL:
        extract_func = &TSInt_ExpValueAsBool;
        op_func      = &TSInt_BoolPrimaryExpOp;

        break;

    case TSDEF_PRIMITIVE_TYPE_INT:
        extract_func = &TSInt_ExpValueAsInt;
        op_func      = &TSInt_IntPrimaryExpOp;

        break;

    case TSDEF_PRIMITIVE_TYPE_REAL:
        extract_func = &TSInt_ExpValueAsReal;
        op_func      = &TSInt_RealPrimaryExpOp;

        break;

    case TSDEF_PRIMITIVE_TYPE_STRING:
        extract_func = &TSInt_ExpValueAsString;
        op_func      = &TSInt_StringPrimaryExpOp;

        break;
    }

    node = exp->start;

    exception = PartialPrimaryExpEvaluation(
                                            extract_func,
                                            op_func,
                                            exp_primitive_type,
                                            &node,
                                            0,
                                            state,
                                            &exp_value
                                           );
    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    if(exp->flags&TSDEF_PRIMARY_EXP_FLAG_NEGATE)
    {
        op_func(TSDEF_PRIMARY_EXP_OP_SUB, exp_value, NULL, result);

        TSInt_DestroyValue(exp_value, exp_primitive_type);
    }
    else
        *result = exp_value;

    return TSINT_EXCEPTION_NONE;
}

int TSInt_ComparisonExpEvaluation (
                                   struct tsdef_comparison_exp* exp,
                                   struct tsint_unit_state*     state,
                                   union tsint_value*           result
                                  )
{
    tsint_exp_comparison_op_if        op_func;
    union tsint_value                 left_exp_value;
    union tsint_value                 left_cmp_value;
    union tsint_value                 right_exp_value;
    union tsint_value                 right_cmp_value;
    struct tsdef_primary_exp*         left_exp;
    struct tsdef_primary_exp*         right_exp;
    struct tsdef_comparison_exp_node* node;
    unsigned int                      primitive_type;
    int                               exception;

    node = exp->start;

    primitive_type = node->primitive_type;
    switch(primitive_type)
    {
    case TSDEF_PRIMITIVE_TYPE_BOOL:
        op_func = &TSInt_BoolComparisonExpOp;

        break;

    case TSDEF_PRIMITIVE_TYPE_INT:
        op_func = &TSInt_IntComparisonExpOp;

        break;

    case TSDEF_PRIMITIVE_TYPE_REAL:
        op_func = &TSInt_RealComparisonExpOp;

        break;

    case TSDEF_PRIMITIVE_TYPE_STRING:
        op_func = &TSInt_StringComparisonExpOp;

        break;
    }

    left_exp  = node->left_exp;
    right_exp = node->right_exp;

    exception = TSInt_PrimaryExpEvaluation(left_exp, state, &left_exp_value);
    if(exception != TSINT_EXCEPTION_NONE)
        goto left_evaluation_failed;

    exception = TSInt_ConvertValue(
                                   left_exp_value,
                                   left_exp->effective_primitive_type,
                                   primitive_type,
                                   &left_cmp_value
                                  );

    TSInt_DestroyValue(left_exp_value, left_exp->effective_primitive_type);

    if(exception != TSINT_ERROR_NONE)
    {
        exception = TSINT_EXCEPTION_OUT_OF_MEMORY;

        goto left_conversion_failed;
    }

    while(right_exp == NULL)
    {
        right_exp = node->remaining_exp->left_exp;

        exception = TSInt_PrimaryExpEvaluation(right_exp, state, &right_exp_value);
        if(exception != TSINT_EXCEPTION_NONE)
            goto right_evaluation_failed;

        exception = TSInt_ConvertValue(
                                       right_exp_value,
                                       right_exp->effective_primitive_type,
                                       primitive_type,
                                       &right_cmp_value
                                      );

        TSInt_DestroyValue(right_exp_value, right_exp->effective_primitive_type);

        if(exception != TSINT_ERROR_NONE)
        {
            exception = TSINT_EXCEPTION_OUT_OF_MEMORY;

            goto right_conversion_failed;
        }

        exception = op_func(node->op, left_cmp_value, right_cmp_value, result);
        if(exception != TSINT_EXCEPTION_NONE)
            goto left_right_op_failed;

        TSInt_DestroyValue(left_cmp_value, primitive_type);

        if(result->bool_data == TSDEF_BOOL_FALSE)
        {
            TSInt_DestroyValue(right_cmp_value, primitive_type);

            return TSINT_EXCEPTION_NONE;
        }

        TSInt_DestroyValue(*result, TSDEF_PRIMITIVE_TYPE_BOOL);

        left_cmp_value = right_cmp_value;

        node      = node->remaining_exp;
        right_exp = node->right_exp;
    }

    exception = TSInt_PrimaryExpEvaluation(right_exp, state, &right_exp_value);
    if(exception != TSINT_EXCEPTION_NONE)
        goto right_evaluation_failed;

    exception = TSInt_ConvertValue(
                                   right_exp_value,
                                   right_exp->effective_primitive_type,
                                   primitive_type,
                                   &right_cmp_value
                                  );

    TSInt_DestroyValue(right_exp_value, right_exp->effective_primitive_type);

    if(exception != TSINT_ERROR_NONE)
    {
        exception = TSINT_EXCEPTION_OUT_OF_MEMORY;

        goto right_conversion_failed;
    }

    exception = op_func(node->op, left_cmp_value, right_cmp_value, result);

left_right_op_failed:
    TSInt_DestroyValue(right_cmp_value, primitive_type);
right_conversion_failed:
right_evaluation_failed:
    TSInt_DestroyValue(left_cmp_value, primitive_type);

left_conversion_failed:
left_evaluation_failed:
    return exception;
}

int TSInt_LogicalExpEvaluation (
                                struct tsdef_logical_exp* exp,
                                struct tsint_unit_state*  state,
                                union tsint_value*        result
                               )
{
    struct tsdef_logical_exp_node* node;
    int                            exception;

    node = exp->start;

    exception = PartialLogicalExpEvaluation(
                                            &node,
                                            0,
                                            state,
                                            result
                                           );

    return exception;
}

int TSInt_EvaluateExp (
                       unsigned int             evaluated_type,
                       struct tsdef_exp*        exp,
                       struct tsint_unit_state* state,
                       union tsint_value*       value
                      )
{
    union tsint_value exp_value;
    unsigned int      exp_type;
    int               exception;

    switch(exp->type)
    {
    case TSDEF_EXP_TYPE_PRIMARY:
        exp_type = exp->data.primary_exp->effective_primitive_type;

        exception = TSInt_PrimaryExpEvaluation(exp->data.primary_exp, state, &exp_value);

        break;

    case TSDEF_EXP_TYPE_COMPARISON:
        exp_type = TSDEF_PRIMITIVE_TYPE_BOOL;

        exception = TSInt_ComparisonExpEvaluation(exp->data.comparison_exp, state, &exp_value);

        break;

    case TSDEF_EXP_TYPE_LOGICAL:
        exp_type = TSDEF_PRIMITIVE_TYPE_BOOL;

        exception = TSInt_LogicalExpEvaluation(exp->data.logical_exp, state, &exp_value);

        break;
    }

    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    exception = TSInt_ConvertValue(exp_value, exp_type, evaluated_type, value);

    TSInt_DestroyValue(exp_value, exp_type);

    if(exception != TSINT_ERROR_NONE)
        return TSINT_EXCEPTION_OUT_OF_MEMORY;

    return TSINT_EXCEPTION_NONE;
}

