/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "assignment.h"
#include "expeval.h"

#include <tsint/variable.h>
#include <tsint/value.h>
#include <tsint/exception.h>


int TSInt_PerformAssignment (struct tsdef_assignment* assignment, struct tsint_unit_state* state)
{
    union tsint_value       value;
    struct tsint_variable*  variable_data;
    struct tsdef_variable*  variable_def;
    int                     exception;

    variable_def  = assignment->lvalue->variable;
    variable_data = TSInt_LookupVariableAddress(variable_def, state);

    exception = TSInt_EvaluateExp(
                                  variable_def->primitive_type,
                                  assignment->rvalue,
                                  state,
                                  &value
                                 );
    if(exception != TSINT_EXCEPTION_NONE)
        return exception;

    if(variable_data->flags&TSINT_VARIABLE_FLAG_INITIALIZED)
        TSInt_DestroyValue(variable_data->value, variable_def->primitive_type);

    variable_data->value  = value;
    variable_data->flags |= TSINT_VARIABLE_FLAG_INITIALIZED;

    return TSINT_EXCEPTION_NONE;
}

