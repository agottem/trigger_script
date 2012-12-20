/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _MATH_RANDOM_H_
#define _MATH_RANDOM_H_


#include <tsffi/function.h>


extern int Math_UniformRandom  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Math_GaussianRandom (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);


#endif

