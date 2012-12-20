/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _FFILIB_THREAD_H_
#define _FFILIB_THREAD_H_


#include <tsffi/function.h>

#include <windows.h>


struct ffilib_thread_data
{
    HWND   message_window;
    DWORD  thread_id;
    HANDLE thread_handle;
    HANDLE message_processed_event;
};

typedef int (*ffilib_thread_function) (void*);


extern int  FFILib_InitializeThread (HINSTANCE);
extern void FFILib_ShutdownThread   (void);

extern int  FFILib_StartThread (struct ffilib_thread_data*);
extern void FFILib_StopThread  (struct ffilib_thread_data*);

extern int FFILib_SynchronousFFIAction   (
                                          tsffi_action_controller,
                                          struct tsffi_invocation_data*,
                                          unsigned int,
                                          void*,
                                          union tsffi_value*,
                                          unsigned int*,
                                          void**,
                                          struct ffilib_thread_data*,
                                          int*
                                         );
extern int FFILib_SynchronousFFIFunction (
                                          tsffi_function,
                                          struct tsffi_invocation_data*,
                                          void*,
                                          union tsffi_value*,
                                          union tsffi_value*,
                                          struct ffilib_thread_data*,
                                          int*
                                         );

extern void FFILib_AsynchronousFunction (
                                         ffilib_thread_function,
                                         void*,
                                         struct ffilib_thread_data*
                                        );
extern int  FFILib_SynchronousFunction  (
                                         ffilib_thread_function,
                                         void*,
                                         struct ffilib_thread_data*,
                                         int*
                                        );


#endif

