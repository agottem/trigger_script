/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_VALUE_H_
#define _TSINT_VALUE_H_


#include <tsdef/def.h>


union tsint_value
{
    tsdef_bool   bool_data;
    tsdef_int    int_data;
    tsdef_real   real_data;
    tsdef_string string_data;
};


extern void TSInt_DestroyValue (union tsint_value, unsigned int);

extern int TSInt_ConvertValue (union tsint_value, unsigned int, unsigned int, union tsint_value*);


#endif

