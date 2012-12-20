/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _MATH_BASIC_H_
#define _MATH_BASIC_H_


#include <tsffi/function.h>


extern int Math_MinBool (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Math_MinInt  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Math_MinReal (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Math_MaxBool (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Math_MaxInt  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Math_MaxReal (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Math_Ceil  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Math_Floor (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Math_Round (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);


#endif

