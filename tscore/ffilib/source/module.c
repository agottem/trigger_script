/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <ffilib/module.h>
#include <ffilib/thread.h>
#include <ffilib/error.h>

#include <graph/gui.h>
#include <tsffi/error.h>

#include <windows.h>


static CRITICAL_SECTION module_begin_count_sync;
static unsigned int     module_begin_count;
static HINSTANCE        dll_instance;

struct ffilib_thread_data ffilib_control_thread;
struct ffilib_thread_data ffilib_gui_thread;


void FFILib_InitializeModuleData (HINSTANCE instance)
{
    InitializeCriticalSection(&module_begin_count_sync);

    module_begin_count = 0;
    dll_instance       = instance;
}

void FFILib_ShutdownModuleData (void)
{
    DeleteCriticalSection(&module_begin_count_sync);
}

int FFILib_BeginModule (
                        struct tsffi_execif*             execif,
                        void*                            execif_data,
                        struct tsffi_registration_group* group,
                        void**                           group_data
                       )
{
    EnterCriticalSection(&module_begin_count_sync);

    if(module_begin_count == 0)
    {
        int error;

        error = Graph_InitializeGUI(dll_instance);
        if(error != FFILIB_ERROR_NONE)
            goto initialize_gui_failed;

        error = FFILib_InitializeThread(dll_instance);
        if(error != FFILIB_ERROR_NONE)
            goto initialize_thread_failed;

        error = FFILib_StartThread(&ffilib_control_thread);
        if(error != FFILIB_ERROR_NONE)
            goto start_control_thread_failed;

        error = FFILib_StartThread(&ffilib_gui_thread);
        if(error != FFILIB_ERROR_NONE)
            goto start_gui_thread_failed;
    }

    module_begin_count++;

    LeaveCriticalSection(&module_begin_count_sync);

    return TSFFI_ERROR_NONE;


start_gui_thread_failed:
    FFILib_StopThread(&ffilib_control_thread);
start_control_thread_failed:
    FFILib_ShutdownThread();
initialize_thread_failed:
    Graph_ShutdownGUI();
initialize_gui_failed:
    LeaveCriticalSection(&module_begin_count_sync);

    return TSFFI_ERROR_EXCEPTION;
}

void FFILib_EndModule (
                       struct tsffi_execif*             execif,
                       void*                            execif_data,
                       int                              error,
                       struct tsffi_registration_group* group,
                       void*                            group_data
                      )
{
    EnterCriticalSection(&module_begin_count_sync);

    module_begin_count--;

    if(module_begin_count == 0)
    {
        FFILib_StopThread(&ffilib_gui_thread);
        FFILib_StopThread(&ffilib_control_thread);
        FFILib_ShutdownThread();
        Graph_ShutdownGUI();
    }

    LeaveCriticalSection(&module_begin_count_sync);
}

