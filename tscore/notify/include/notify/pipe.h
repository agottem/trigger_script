/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _NOTIFY_PIPE_H_
#define _NOTIFY_PIPE_H_


#include <tsffi/function.h>

#include <windows.h>


struct notify_pipe_data
{
    unsigned int pipe_id;

    HANDLE       file_handle;
    char*        current_line;
    unsigned int line_argument_count;

    struct tsffi_invocation_data* action_data;

    struct notify_pipe_data* next_pipe;
};

struct notify_module_data;


extern int Notify_StartPipePolling (struct notify_module_data*);
extern int Notify_StopPipePolling  (struct notify_module_data*);

extern void Notify_DestroyPipes (struct notify_module_data*);

extern int Notify_Pipe (
                        struct tsffi_invocation_data*,
                        void*,
                        union tsffi_value*,
                        union tsffi_value*
                       );

extern int Notify_ReadPipe  (
                             struct tsffi_invocation_data*,
                             void*,
                             union tsffi_value*,
                             union tsffi_value*
                            );
extern int Notify_HReadPipe (
                             struct tsffi_invocation_data*,
                             void*,
                             union tsffi_value*,
                             union tsffi_value*
                            );

extern int Notify_Action_ListenPipe  (
                                      struct tsffi_invocation_data*,
                                      unsigned int,
                                      void*,
                                      union tsffi_value*,
                                      unsigned int*,
                                      void**
                                     );
extern int Notify_Action_HListenPipe (
                                      struct tsffi_invocation_data*,
                                      unsigned int,
                                      void*,
                                      union tsffi_value*,
                                      unsigned int*,
                                      void**
                                     );


#endif

