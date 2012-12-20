/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_FFI_H_
#define _TSINT_FFI_H_


#include <tsdef/def.h>
#include <tsint/value.h>
#include <tsffi/function.h>
#include <tsint/module.h>


extern int TSInt_DefTypeToFFIType (union tsint_value, unsigned int, union tsffi_value*);
extern int TSInt_FFITypeToDefType (union tsffi_value, unsigned int, union tsint_value*);

extern void TSInt_DestroyFFIArgument (union tsffi_value, unsigned int);

extern int  TSInt_ExpListToFFIArguments (
                                         struct tsdef_exp_list*,
                                         unsigned int*,
                                         struct tsint_unit_state*,
                                         union tsffi_value**
                                        );
extern void TSInt_DestroyFFIArguments   (
                                         unsigned int*,
                                         unsigned int,
                                         union tsffi_value*
                                        );


#endif

