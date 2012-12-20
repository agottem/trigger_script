/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_VARIABLE_H_
#define _TSINT_VARIABLE_H_


#include <tsdef/def.h>
#include <tsint/value.h>
#include <tsint/module.h>


#define TSINT_VARIABLE_FLAG_INITIALIZED 0x01


struct tsint_variable
{
    union tsint_value value;
    unsigned int      flags;
};


extern struct tsint_variable* TSInt_LookupVariableAddress (
                                                           struct tsdef_variable*,
                                                           struct tsint_unit_state*
                                                          );


#endif

