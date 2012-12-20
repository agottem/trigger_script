/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TIME_TIME_H_
#define _TIME_TIME_H_


#include <tsffi/function.h>


extern int Time_Time  (
                       struct tsffi_invocation_data*,
                       void*,
                       union tsffi_value*,
                       union tsffi_value*
                      );
extern int Time_Delay (
                       struct tsffi_invocation_data*,
                       void*,
                       union tsffi_value*,
                       union tsffi_value*
                      );


#endif

