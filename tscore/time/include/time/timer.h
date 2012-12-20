/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TIME_TIMER_H_
#define _TIME_TIMER_H_


#include <tsffi/function.h>


extern int Time_Action_Timer  (struct tsffi_invocation_data*, unsigned int, void*, union tsffi_value*, unsigned int*, void**);
extern int Time_Action_PTimer (struct tsffi_invocation_data*, unsigned int, void*, union tsffi_value*, unsigned int*, void**);
extern int Time_Action_VTimer (struct tsffi_invocation_data*, unsigned int, void*, union tsffi_value*, unsigned int*, void**);


#endif

