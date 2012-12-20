/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "expvalue.h"
#include "expeval.h"
#include "unit.h"
#include "ffi.h"

#include <tsdef/module.h>
#include <tsdef/ffi.h>
#include <tsffi/function.h>
#include <tsffi/register.h>
#include <tsffi/error.h>
#include <tsint/error.h>
#include <tsint/exception.h>
#include <tsint/variable.h>
#include <tsint/value.h>

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>


#define MAX_CONVERSION_STRING_LENGTH (3+DBL_MANT_DIG-DBL_MIN_EXP)


static int ExecuteFunction (
                            struct tsdef_function_call*,
                            struct tsint_unit_state*,
                            unsigned int,
                            union tsint_value*
                           );


static int ExecuteFunction (
                            struct tsdef_function_call* function_call,
                            struct tsint_unit_state*    unit_state,
                            unsigned int                primitive_type,
                            union tsint_value*          result
                           )
{
    union  tsint_value          function_output;
    struct tsdef_module_object* module_object;
    struct tsint_module_state*  module_state;
    unsigned int                output_type;
    int                         exception;

    module_object = function_call->module_object;

    module_state = unit_state->module_state;

    if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT)
    {
        struct tsdef_unit* unit;
        union tsint_value* arguments;
        int                mode;
        int                original_mode;

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
                                     &function_output,
                                     &mode,
                                     module_state
                                    );
        if(original_mode != mode)
            unit_state->mode = mode;

        TSInt_DestroyValueArray(unit->input, arguments);

        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        output_type = unit->output->output_variable_assignment->lvalue->variable->primitive_type;
    }
    else
    {
        struct tsffi_invocation_data      invocation_data;
        union tsffi_value                 ffi_output;
        struct tsffi_function_definition* ffi_function;
        union tsffi_value*                ffi_arguments;
        struct tsdef_module_ffi_group*    ffi_group;

        ffi_function = module_object->type.ffi.function_definition;
        ffi_group    = module_object->type.ffi.group;

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

        output_type = ffi_function->output_type;

        exception = TSInt_FFITypeToDefType(ffi_output, output_type, &function_output);

        TSInt_DestroyFFIArgument(ffi_output, output_type);

        if(exception != TSFFI_ERROR_NONE)
            return TSINT_EXCEPTION_FFI;

        output_type = TSDef_TranslateFFIType(output_type);
    }

    exception = TSInt_ConvertValue(
                                   function_output,
                                   output_type,
                                   primitive_type,
                                   result
                                  );

    TSInt_DestroyValue(function_output, output_type);

    if(exception != TSINT_ERROR_NONE)
        return TSINT_EXCEPTION_OUT_OF_MEMORY;

    return TSINT_EXCEPTION_NONE;
}


int TSInt_ExpValueAsBool (
                          struct tsdef_exp_value_type* exp_value_type,
                          struct tsint_unit_state*     state,
                          union tsint_value*           value
                         )
{
    struct tsdef_variable* variable_def;
    struct tsint_variable* variable_data;
    int                    exception;

    switch(exp_value_type->type)
    {
    case TSDEF_EXP_VALUE_TYPE_BOOL:
        value->bool_data = exp_value_type->data.bool_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_INT:
        if(exp_value_type->data.int_constant != 0)
            value->bool_data = TSDEF_BOOL_TRUE;
        else
            value->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_EXP_VALUE_TYPE_REAL:
        if(exp_value_type->data.real_constant != 0)
            value->bool_data = TSDEF_BOOL_TRUE;
        else
            value->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        if(strcmp(exp_value_type->data.string_constant, "true") != 0)
            value->bool_data = TSDEF_BOOL_TRUE;
        else
            value->bool_data = TSDEF_BOOL_FALSE;

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        exception = ExecuteFunction(
                                    exp_value_type->data.function_call,
                                    state,
                                    TSDEF_PRIMITIVE_TYPE_BOOL,
                                    value
                                   );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        variable_def  = exp_value_type->data.variable->variable;
        variable_data = TSInt_LookupVariableAddress(variable_def, state);

        exception = TSInt_ConvertValue(
                                       variable_data->value,
                                       variable_def->primitive_type,
                                       TSDEF_PRIMITIVE_TYPE_BOOL,
                                       value
                                      );
        if(exception != TSINT_ERROR_NONE)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        exception = TSInt_EvaluateExp(
                                      TSDEF_PRIMITIVE_TYPE_BOOL,
                                      exp_value_type->data.exp,
                                      state,
                                      value
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_ExpValueAsInt (
                         struct tsdef_exp_value_type* exp_value_type,
                         struct tsint_unit_state*     state,
                         union tsint_value*           value
                        )
{
    struct tsdef_variable* variable_def;
    struct tsint_variable* variable_data;
    int                    exception;

    switch(exp_value_type->type)
    {
    case TSDEF_EXP_VALUE_TYPE_BOOL:
        value->int_data = (tsdef_int)exp_value_type->data.bool_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_INT:
        value->int_data = exp_value_type->data.int_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_REAL:
        value->int_data = (tsdef_int)exp_value_type->data.real_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        value->int_data = (tsdef_int)atoi(exp_value_type->data.string_constant);

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        exception = ExecuteFunction(
                                    exp_value_type->data.function_call,
                                    state,
                                    TSDEF_PRIMITIVE_TYPE_INT,
                                    value
                                   );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        variable_def  = exp_value_type->data.variable->variable;
        variable_data = TSInt_LookupVariableAddress(variable_def, state);

        exception = TSInt_ConvertValue(
                                       variable_data->value,
                                       variable_def->primitive_type,
                                       TSDEF_PRIMITIVE_TYPE_INT,
                                       value
                                      );
        if(exception != TSINT_ERROR_NONE)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        exception = TSInt_EvaluateExp(
                                      TSDEF_PRIMITIVE_TYPE_INT,
                                      exp_value_type->data.exp,
                                      state,
                                      value
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_ExpValueAsReal (
                          struct tsdef_exp_value_type* exp_value_type,
                          struct tsint_unit_state*     state,
                          union tsint_value*           value
                         )
{
    struct tsdef_variable* variable_def;
    struct tsint_variable* variable_data;
    int                    exception;

    switch(exp_value_type->type)
    {
    case TSDEF_EXP_VALUE_TYPE_BOOL:
        value->real_data = (tsdef_real)exp_value_type->data.bool_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_INT:
        value->real_data = (tsdef_real)exp_value_type->data.int_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_REAL:
        value->real_data = exp_value_type->data.real_constant;

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        value->real_data = (tsdef_real)atof(exp_value_type->data.string_constant);

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        exception = ExecuteFunction(
                                    exp_value_type->data.function_call,
                                    state,
                                    TSDEF_PRIMITIVE_TYPE_REAL,
                                    value
                                   );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        variable_def  = exp_value_type->data.variable->variable;
        variable_data = TSInt_LookupVariableAddress(variable_def, state);

        exception = TSInt_ConvertValue(
                                       variable_data->value,
                                       variable_def->primitive_type,
                                       TSDEF_PRIMITIVE_TYPE_REAL,
                                       value
                                      );
        if(exception != TSINT_ERROR_NONE)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        exception = TSInt_EvaluateExp(
                                      TSDEF_PRIMITIVE_TYPE_REAL,
                                      exp_value_type->data.exp,
                                      state,
                                      value
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_ExpValueAsString (
                            struct tsdef_exp_value_type* exp_value_type,
                            struct tsint_unit_state*     state,
                            union tsint_value*           value
                           )
{
    char                   converted_string[MAX_CONVERSION_STRING_LENGTH+1];
    struct tsdef_variable* variable_def;
    struct tsint_variable* variable_data;
    int                    exception;

    switch(exp_value_type->type)
    {
    case TSDEF_EXP_VALUE_TYPE_BOOL:
        if(exp_value_type->data.bool_constant == TSDEF_BOOL_TRUE)
            value->string_data = strdup(TSDEF_BOOL_TRUE_STRING);
        else
            value->string_data = strdup(TSDEF_BOOL_FALSE_STRING);

        if(value->string_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_INT:
        sprintf(converted_string, "%d", exp_value_type->data.int_constant);

        value->string_data = strdup(converted_string);
        if(value->string_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_REAL:
        sprintf(converted_string, "%f", exp_value_type->data.real_constant);

        value->string_data = strdup(converted_string);
        if(value->string_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_STRING:
        value->string_data = strdup(exp_value_type->data.string_constant);
        if(value->string_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL:
        exception = ExecuteFunction(
                                    exp_value_type->data.function_call,
                                    state,
                                    TSDEF_PRIMITIVE_TYPE_STRING,
                                    value
                                   );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;

    case TSDEF_EXP_VALUE_TYPE_VARIABLE:
        variable_def  = exp_value_type->data.variable->variable;
        variable_data = TSInt_LookupVariableAddress(variable_def, state);

        exception = TSInt_ConvertValue(
                                       variable_data->value,
                                       variable_def->primitive_type,
                                       TSDEF_PRIMITIVE_TYPE_STRING,
                                       value
                                      );
        if(exception != TSINT_ERROR_NONE)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;

    case TSDEF_EXP_VALUE_TYPE_EXP:
        exception = TSInt_EvaluateExp(
                                      TSDEF_PRIMITIVE_TYPE_STRING,
                                      exp_value_type->data.exp,
                                      state,
                                      value
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            return exception;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

