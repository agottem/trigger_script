/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "sync.h"

#include <tsint/error.h>
#include <tsint/exception.h>


int TSInt_InitializeSyncData (
                              struct tsint_module_sync_data*    sync_data,
                              struct tsint_module_abort_signal* abort_signal
                             )
{
    sync_data->action_signal = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(sync_data->action_signal == NULL)
        return TSINT_ERROR_SYSTEM_CALL;

    InitializeCriticalSection(&sync_data->action_state_sync);

    sync_data->abort_signal = abort_signal;

    return TSINT_ERROR_NONE;
}

void TSInt_DestroySyncData (struct tsint_module_sync_data* sync_data)
{
    DeleteCriticalSection(&sync_data->action_state_sync);
    CloseHandle(sync_data->action_signal);
}

void TSInt_LockActionStateSync (struct tsint_module_sync_data* sync_data)
{
    EnterCriticalSection(&sync_data->action_state_sync);
}

void TSInt_UnlockActionStateSync (struct tsint_module_sync_data* sync_data)
{
    LeaveCriticalSection(&sync_data->action_state_sync);
}

int TSInt_ListenForAction (struct tsint_module_sync_data* sync_data)
{
    if(sync_data->abort_signal != NULL)
    {
        HANDLE events[2];
        DWORD  signaled_event;

        events[0] = sync_data->action_signal;
        events[1] = sync_data->abort_signal->abort_signal;

        signaled_event = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if(signaled_event == 1)
            return TSINT_EXCEPTION_HALT;
    }
    else
        WaitForSingleObject(sync_data->action_signal, INFINITE);

    return TSINT_EXCEPTION_NONE;
}

void TSInt_SignalAction (struct tsint_module_sync_data* sync_data)
{
    SetEvent(sync_data->action_signal);
}

void TSInt_ClearSignal (struct tsint_module_sync_data* sync_data)
{
    ResetEvent(sync_data->action_signal);
}

int TSInt_InitializeAbortSignal (struct tsint_module_abort_signal* signal_data)
{
    signal_data->abort_signal = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(signal_data->abort_signal == NULL)
        return TSINT_ERROR_SYSTEM_CALL;

    return TSINT_ERROR_NONE;
}

void TSInt_DestroyAbortSignal (struct tsint_module_abort_signal* signal_data)
{
    CloseHandle(signal_data->abort_signal);
}

void TSInt_SetAbortSignal (struct tsint_module_abort_signal* signal_data)
{
    SetEvent(signal_data->abort_signal);
}

void TSInt_ClearAbortSignal (struct tsint_module_abort_signal* signal_data)
{
    ResetEvent(signal_data->abort_signal);
}

int TSInt_TestAbortSignal (struct tsint_module_sync_data* sync_data)
{
    if(sync_data->abort_signal != NULL)
    {
        DWORD state;

        state = WaitForSingleObject(sync_data->abort_signal->abort_signal, 0);
        if(state == WAIT_OBJECT_0)
            return TSINT_EXCEPTION_HALT;
    }

    return TSINT_EXCEPTION_NONE;
}

