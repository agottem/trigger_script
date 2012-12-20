/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsdef/arguments.h>
#include <tsdef/ffi.h>
#include <tsdef/error.h>

#include <stdlib.h>
#include <malloc.h>


void TSDef_InitializeArgumentTypes (struct tsdef_argument_types* argument_types)
{
    argument_types->count = 0;
    argument_types->types = NULL;
}

int TSDef_ArgumentTypesFromExpList (
                                    struct tsdef_exp_list*       exp_list,
                                    struct tsdef_argument_types* argument_types
                                   )
{
    if(exp_list == NULL)
    {
        argument_types->types = NULL;
        argument_types->count = 0;
    }
    else
    {
        unsigned int*               type_list;
        struct tsdef_exp_list_node* node;
        size_t                      alloc_size;

        alloc_size = sizeof(unsigned int)*exp_list->count;

        type_list = malloc(alloc_size);
        if(type_list == NULL)
            return TSDEF_ERROR_MEMORY;

        argument_types->types = type_list;

        for(node = exp_list->start; node != NULL; node = node->next_exp)
        {
            *type_list = TSDef_ExpPrimitiveType(node->exp);

            type_list++;
        }

        argument_types->count = exp_list->count;
    }

    return TSDEF_ERROR_NONE;
}

int TSDef_ArgumentTypesFromInput (
                                  struct tsdef_input*          input,
                                  struct tsdef_argument_types* argument_types
                                 )
{
    if(input == NULL)
    {
        argument_types->count = 0;
        argument_types->types = NULL;
    }
    else
    {
        struct tsdef_variable_list*      variable_list;
        struct tsdef_variable_list_node* variable_node;
        unsigned int*                    type_list;
        size_t                           alloc_size;

        variable_list = input->input_variables;

        alloc_size = sizeof(unsigned int)*variable_list->count;

        type_list = malloc(alloc_size);
        if(type_list == NULL)
            return TSDEF_ERROR_MEMORY;

        argument_types->types = type_list;

        for(
            variable_node = input->input_variables->start;
            variable_node != NULL;
            variable_node = variable_node->next_variable
           )
        {
            struct tsdef_variable_reference* reference;
            struct tsdef_variable*           variable;

            reference = variable_node->variable;
            variable  = reference->variable;

            if(variable == NULL)
                goto bad_type_in_input_list;
            if(variable->primitive_type == TSDEF_PRIMITIVE_TYPE_DELAYED)
                goto bad_type_in_input_list;

            *type_list = variable->primitive_type;

            type_list++;
        }

        argument_types->count = variable_list->count;
    }

    return TSDEF_ERROR_NONE;

bad_type_in_input_list:
    free(argument_types->types);

    return TSDEF_ERROR_INCOMPLETE_DEF;
}

void TSDef_DestroyArgumentTypes (struct tsdef_argument_types* argument_types)
{
    if(argument_types->types != NULL)
        free(argument_types->types);
}

int TSDef_ArgumentCountMatchInput (
                                   struct tsdef_input*          input,
                                   struct tsdef_argument_types* argument_types
                                  )
{
    if(
       input == NULL && argument_types->count != 0 ||
       input != NULL && input->input_variables->count != argument_types->count
      )
    {
        return TSDEF_ARGUMENT_COUNT_MISMATCH;
    }

    return TSDEF_ARGUMENT_MATCH;
}

int TSDef_ArgumentTypesMatchInput (
                                   struct tsdef_input*          input,
                                   struct tsdef_argument_types* argument_types
                                  )
{
    int match;

    match = TSDef_ArgumentCountMatchInput(input, argument_types);
    if(match != TSDEF_ARGUMENT_MATCH)
        return match;

    if(input != NULL)
    {
        struct tsdef_variable_list_node* node;
        unsigned int*                    type;

        type = argument_types->types;

        for(node = input->input_variables->start; node != NULL; node = node->next_variable)
        {
            struct tsdef_variable* variable;

            variable = node->variable->variable;

            if(variable == NULL || variable->primitive_type != *type)
                return TSDEF_ARGUMENT_TYPE_MISMATCH;

            type++;
        }
    }

    return TSDEF_ARGUMENT_MATCH;
}

extern int TSDef_ArgumentTypesMatchFFI (
                                        struct tsffi_function_definition* ffi,
                                        struct tsdef_argument_types*      argument_types,
                                        struct tsdef_ffi_argument_match*  match
                                       )
{
    unsigned int* type;
    unsigned int* ffi_type;
    unsigned int  summed_delta;
    unsigned int  index;
    unsigned int argument_count;

    if(ffi->argument_count != argument_types->count)
        return TSDEF_ARGUMENT_COUNT_MISMATCH;

    match->allow_conversion = TSDEF_PRIMITIVE_CONVERSION_ALLOWED;

    argument_count = ffi->argument_count;
    type           = argument_types->types;
    ffi_type       = ffi->argument_types;

    summed_delta = 0;

    for(index = 0; index < argument_count; index++)
    {
        unsigned int from_type;
        unsigned int to_type;
        int          allow_conversion;

        from_type = *type;
        to_type   = TSDef_TranslateFFIType(*ffi_type);

        allow_conversion = TSDef_AllowPrimitiveConversion(from_type, to_type);
        if(allow_conversion == TSDEF_PRIMITIVE_CONVERSION_DISALLOWED)
        {
            match->allow_conversion = TSDEF_PRIMITIVE_CONVERSION_DISALLOWED;
            match->argument_index   = index;
            match->from_type        = from_type;
            match->to_type          = to_type;
        }

        if(to_type > from_type)
            summed_delta += to_type-from_type;
        else
            summed_delta += from_type-to_type;

        type++;
        ffi_type++;
    }

    match->delta = summed_delta;

    if(match->allow_conversion == TSDEF_PRIMITIVE_CONVERSION_DISALLOWED)
        return TSDEF_ARGUMENT_TYPE_MISMATCH;

    return TSDEF_ARGUMENT_MATCH;
}

