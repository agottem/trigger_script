/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "controller.h"
#include "main.h"
#include "context.h"

#include <tsdef/def.h>
#include <tsint/variable.h>
#include <tsint/exception.h>

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>


#define LINES_OF_CONTEXT 5
#define PADDING_SIZE     5


static void PrintVariables (struct tsint_unit_state*);


static void PrintVariables (struct tsint_unit_state* state)
{
    struct tsdef_block* current_scope;

    for(
        current_scope = state->current_block;
        current_scope != NULL;
        current_scope = current_scope->parent_block
       )
    {
        struct tsdef_variable* variables;

        for(
            variables = current_scope->variables;
            variables != NULL;
            variables = variables->next_variable
           )
        {
            struct tsint_variable* variable_data;

            variable_data = TSInt_LookupVariableAddress(variables, state);

            if(!(variable_data->flags&TSINT_VARIABLE_FLAG_INITIALIZED))
                continue;

            switch(variables->primitive_type)
            {
            case TSDEF_PRIMITIVE_TYPE_BOOL:
                printf(
                       "bool %s = %s\n",
                       variables->name,
                       variable_data->value.bool_data ? "true" : "false"
                      );

                break;

            case TSDEF_PRIMITIVE_TYPE_INT:
                printf("integer %s = %d\n", variables->name, variable_data->value.int_data);

                break;

            case TSDEF_PRIMITIVE_TYPE_REAL:
                printf("real %s = %f\n", variables->name, variable_data->value.real_data);

                break;

            case TSDEF_PRIMITIVE_TYPE_STRING:
                printf("string %s = %s\n", variables->name, variable_data->value.string_data);

                break;
            }
        }
    }
}


unsigned int TSI_Controller (struct tsint_unit_state* state, void* user_data)
{
    char*        unit_name;
    unsigned int location;
    int          pressed_char;

    unit_name = state->unit->name;

    if(strcmp(unit_name, "_module_main") == 0 && state->exception == TSINT_EXCEPTION_NONE)
    {
        if(tsi_flags&TSI_FLAG_DEBUG)
            return TSINT_CONTROL_STEP_INTO;

        return TSINT_CONTROL_RUN;
    }

    location = state->current_location;

    printf("Function: %s Line: %d\n", unit_name, location);

    TSI_DisplayContext(unit_name, location, LINES_OF_CONTEXT, PADDING_SIZE);

    if(state->exception != TSINT_EXCEPTION_NONE)
    {
        printf(
               "\n"
               "EXCEPTION: "
              );

        switch(state->exception)
        {
        case TSINT_EXCEPTION_OUT_OF_MEMORY:
            printf("Out of memory");

            break;

        case TSINT_EXCEPTION_DIVIDE_BY_ZERO:
            printf("Divide by zero");

            break;

        case TSINT_EXCEPTION_FFI:
            printf("Failure in ffi plugin");

            break;

        case TSINT_EXCEPTION_HALT:
            printf("Instructed to halt");

            break;
        }

        printf("\n");
    }

    while(1)
    {
        printf(
               "\n"
               "Possible commands: (n)ext, (s)tep into, (r)un, (h)alt, (v)ariables\n"
               "Command> "
              );

        do
        {
            pressed_char = getch();
            pressed_char = tolower(pressed_char);

            if(isalpha(pressed_char))
                putchar(pressed_char);
        }while(
               pressed_char != 'n' &&
               pressed_char != 's' &&
               pressed_char != 'r' &&
               pressed_char != 'h' &&
               pressed_char != 'v'
              );

        printf("\n\n");

        if(pressed_char != 'v')
            break;

        PrintVariables(state);
    }

    switch(pressed_char)
    {
    case 'n':
        return TSINT_CONTROL_STEP;

    case 's':
        return TSINT_CONTROL_STEP_INTO;

    case 'r':
        return TSINT_CONTROL_RUN;

    case 'h':
        return TSINT_CONTROL_HALT;
    }

    return TSINT_CONTROL_HALT;
}

