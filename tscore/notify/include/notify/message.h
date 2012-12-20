/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _NOTIFY_MESSAGE_H_
#define _NOTIFY_MESSAGE_H_


#include <tsffi/function.h>


extern int Notify_Print   (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Notify_Message (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Notify_Choice  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);


#endif

