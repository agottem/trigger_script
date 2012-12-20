/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSDEF_ARGUMENTS_H_
#define _TSDEF_ARGUMENTS_H_


#include <tsdef/def.h>
#include <tsffi/register.h>


#define TSDEF_ARGUMENT_MATCH           0
#define TSDEF_ARGUMENT_COUNT_MISMATCH -1
#define TSDEF_ARGUMENT_TYPE_MISMATCH  -2


struct tsdef_argument_types
{
    unsigned int* types;
    unsigned int  count;
};

struct tsdef_ffi_argument_match
{
    unsigned int delta;

    int          allow_conversion;
    unsigned int argument_index;
    unsigned int from_type;
    unsigned int to_type;
};


extern int  TSDef_ArgumentTypesFromExpList (struct tsdef_exp_list*, struct tsdef_argument_types*);
extern int  TSDef_ArgumentTypesFromInput   (struct tsdef_input*, struct tsdef_argument_types*);
extern void TSDef_DestroyArgumentTypes     (struct tsdef_argument_types*);

extern int TSDef_ArgumentCountMatchInput (struct tsdef_input*, struct tsdef_argument_types*);
extern int TSDef_ArgumentTypesMatchInput (struct tsdef_input*, struct tsdef_argument_types*);
extern int TSDef_ArgumentTypesMatchFFI   (
                                          struct tsffi_function_definition*,
                                          struct tsdef_argument_types*,
                                          struct tsdef_ffi_argument_match*
                                         );


#endif

