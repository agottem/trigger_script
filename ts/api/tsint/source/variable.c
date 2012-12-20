/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsint/variable.h>
#include <tsint/exception.h>

#include "expeval.h"


struct tsint_variable* TSInt_LookupVariableAddress (
                                                    struct tsdef_variable*   variable_def,
                                                    struct tsint_unit_state* state
                                                   )
{
    struct tsint_execution_stack* stack;
    struct tsint_variable*        variable_data;
    unsigned int                  depth;

    depth = variable_def->block->depth;

    stack         = &state->execution_stack[depth];
    variable_data = &stack->variables[variable_def->index];

    return variable_data;
}

