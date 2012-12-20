/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "ffi.h"
#include "expeval.h"

#include <tsdef/ffi.h>
#include <tsffi/register.h>
#include <tsint/error.h>
#include <tsint/exception.h>

#include <stdlib.h>
#include <string.h>


int TSInt_DefTypeToFFIType (
                            union tsint_value  value,
                            unsigned int       type,
                            union tsffi_value* ffi_value
                           )
{
    switch(type)
    {
    case TSDEF_PRIMITIVE_TYPE_BOOL:
        if(value.bool_data == TSDEF_BOOL_TRUE)
            ffi_value->bool_data = TSFFI_BOOL_TRUE;
        else
            ffi_value->bool_data = TSFFI_BOOL_FALSE;

        break;

    case TSDEF_PRIMITIVE_TYPE_INT:
        ffi_value->int_data = (tsffi_int)value.int_data;

        break;

    case TSDEF_PRIMITIVE_TYPE_REAL:
        ffi_value->real_data = (tsffi_real)value.real_data;

        break;

    case TSDEF_PRIMITIVE_TYPE_STRING:
        ffi_value->string_data = strdup(value.string_data);
        if(ffi_value->string_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

int TSInt_FFITypeToDefType (
                            union tsffi_value  value,
                            unsigned int       type,
                            union tsint_value* def_value
                           )
{
    switch(type)
    {
    case TSFFI_PRIMITIVE_TYPE_BOOL:
        if(value.bool_data == TSFFI_BOOL_TRUE)
            def_value->bool_data = TSFFI_BOOL_TRUE;
        else
            def_value->bool_data = TSFFI_BOOL_FALSE;

        break;

    case TSFFI_PRIMITIVE_TYPE_INT:
        def_value->int_data = (tsdef_int)value.int_data;

        break;

    case TSFFI_PRIMITIVE_TYPE_REAL:
        def_value->real_data = (tsdef_real)value.real_data;

        break;

    case TSFFI_PRIMITIVE_TYPE_STRING:
        def_value->string_data = strdup(value.string_data);
        if(def_value->string_data == NULL)
            return TSINT_EXCEPTION_OUT_OF_MEMORY;

        break;
    }

    return TSINT_EXCEPTION_NONE;
}

void TSInt_DestroyFFIArgument (union tsffi_value value, unsigned int type)
{
    switch(type)
    {
    case TSFFI_PRIMITIVE_TYPE_BOOL:
    case TSFFI_PRIMITIVE_TYPE_INT:
    case TSFFI_PRIMITIVE_TYPE_REAL:
        break;

    case TSFFI_PRIMITIVE_TYPE_STRING:
        free(value.string_data);

        break;
    }
}

int TSInt_ExpListToFFIArguments (
                                 struct tsdef_exp_list*   exp_list,
                                 unsigned int*            ffi_argument_types,
                                 struct tsint_unit_state* state,
                                 union tsffi_value**      created_ffi_arguments
                                )
{
    union tsffi_value*          ffi_arguments;
    union tsffi_value*          original_ffi_arguments;
    struct tsdef_exp_list_node* exp_node;
    size_t                      alloc_size;
    int                         exception;

    if(exp_list == NULL)
    {
        *created_ffi_arguments = NULL;

        return TSINT_EXCEPTION_NONE;
    }

    alloc_size = sizeof(union tsffi_value)*exp_list->count;
    ffi_arguments = malloc(alloc_size);
    if(ffi_arguments == NULL)
        return TSINT_EXCEPTION_OUT_OF_MEMORY;

    original_ffi_arguments = ffi_arguments;

    for(exp_node = exp_list->start; exp_node != NULL; exp_node = exp_node->next_exp)
    {
        union tsint_value def_value;
        unsigned int      def_type;

        def_type = TSDef_TranslateFFIType(*ffi_argument_types);

        exception = TSInt_EvaluateExp(
                                      def_type,
                                      exp_node->exp,
                                      state,
                                      &def_value
                                     );
        if(exception != TSINT_EXCEPTION_NONE)
            goto evaluate_exp_failed;

        exception = TSInt_DefTypeToFFIType(def_value, def_type, ffi_arguments);

        TSInt_DestroyValue(def_value, def_type);

        if(exception != TSINT_EXCEPTION_NONE)
            goto translate_type_failed;

        ffi_argument_types++;
        ffi_arguments++;
    }

    *created_ffi_arguments = original_ffi_arguments;

    return TSINT_EXCEPTION_NONE;

translate_type_failed:
evaluate_exp_failed:
    while(ffi_arguments != original_ffi_arguments)
    {
        ffi_arguments--;
        ffi_argument_types--;

        TSInt_DestroyFFIArgument(*ffi_arguments, *ffi_argument_types);
    }

    free(original_ffi_arguments);

    return exception;
}

void TSInt_DestroyFFIArguments (
                                unsigned int*      argument_types,
                                unsigned int       argument_count,
                                union tsffi_value* arguments
                               )
{
    unsigned int index;

    if(argument_count == 0)
        return;

    index = argument_count;
    while(index--)
        TSInt_DestroyFFIArgument(arguments[index], argument_types[index]);

    free(arguments);
}

