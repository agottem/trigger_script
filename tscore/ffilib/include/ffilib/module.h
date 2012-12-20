/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _FFILIB_MODULE_H_
#define _FFILIB_MODULE_H_


#include <ffilib/thread.h>

#include <tsffi/register.h>

#include <windows.h>


extern struct ffilib_thread_data ffilib_control_thread;
extern struct ffilib_thread_data ffilib_gui_thread;


extern void FFILib_InitializeModuleData (HINSTANCE);
extern void FFILib_ShutdownModuleData   (void);

extern int  FFILib_BeginModule (
                                struct tsffi_execif*,
                                void*,
                                struct tsffi_registration_group*,
                                void**
                               );
extern void FFILib_EndModule   (
                                struct tsffi_execif*,
                                void*,
                                int,
                                struct tsffi_registration_group*,
                                void*
                               );


#endif

