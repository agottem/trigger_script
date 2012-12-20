/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _NOTIFY_INIT_H_
#define _NOTIFY_INIT_H_


#include <notify/pipe.h>
#include <ffilib/idhash.h>

#include <tsffi/register.h>


struct notify_module_data
{
    unsigned int next_pipe_id;
    unsigned int pipe_poll_timer_id;

    struct ffilib_id_hash pipe_hash;
    struct ffilib_id_hash unit_hash;

    struct notify_pipe_data* open_pipes;
};


extern int  Notify_BeginModule (
                                struct tsffi_execif*,
                                void*,
                                struct tsffi_registration_group*,
                                void**
                               );
extern int  Notify_ModuleState (
                                struct tsffi_execif*,
                                void*,
                                unsigned int,
                                struct tsffi_registration_group*,
                                void*
                               );
extern void Notify_EndModule   (
                                struct tsffi_execif*,
                                void*,
                                int,
                                struct tsffi_registration_group*,
                                void*
                               );


#endif

