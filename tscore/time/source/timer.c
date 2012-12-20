/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <time/timer.h>

#include <tsffi/error.h>
#include <ffilib/thread.h>
#include <ffilib/module.h>
#include <ffilib/idhash.h>
#include <ffilib/error.h>

#include <windows.h>
#include <time.h>


#define TIMER_FLAG_RUNNING        0x01
#define TIMER_FLAG_FINISHED       0x02
#define TIMER_FLAG_TRIGGERED      0x04
#define TIMER_FLAG_TRIGGERED_WAIT 0x08
#define TIMER_FLAG_PERIODIC       0x10


struct timer_data
{
    struct tsffi_invocation_data* action_data;

    unsigned int flags;
    unsigned int timer_id;
};


static VOID CALLBACK TimerEvent (HWND, UINT, UINT_PTR, DWORD);


FFILIB_DECLARE_STATIC_ID_HASH(timer_id_hash);


static VOID CALLBACK TimerEvent (
                                 HWND     window_handle,
                                 UINT     message,
                                 UINT_PTR timer_id,
                                 DWORD    time_ellapsed
                                )
{
    struct timer_data* data;

    data = FFILib_GetIDData(timer_id, &timer_id_hash);

    if(data->flags&TIMER_FLAG_RUNNING)
        data->flags |= TIMER_FLAG_TRIGGERED_WAIT;
    else
    {
        struct tsffi_invocation_data* action_data;

        data->flags |= TIMER_FLAG_TRIGGERED;

        action_data = data->action_data;

        action_data->execif->signal_action(data->action_data->execif_data);
    }

    if(!(data->flags&TIMER_FLAG_PERIODIC))
    {
        KillTimer(NULL, timer_id);
        FFILib_RemoveID(data->timer_id, &timer_id_hash);
    }
}


int Time_Action_Timer (
                       struct tsffi_invocation_data* action_data,
                       unsigned int                  request,
                       void*                         group_data,
                       union tsffi_value*            input,
                       unsigned int*                 state,
                       void**                        user_action_data
                      )
{
    struct timer_data* data;
    int                error;
    int                delay;
    int                result;

    error = FFILib_SynchronousFFIAction(
                                        &Time_Action_Timer,
                                        action_data,
                                        request,
                                        group_data,
                                        input,
                                        state,
                                        user_action_data,
                                        &ffilib_control_thread,
                                        &result
                                       );
    if(error == FFILIB_ERROR_NONE)
        return result;
    else if(error != FFILIB_ERROR_THREAD_SWITCH)
        return TSFFI_ERROR_EXCEPTION;

    switch(request)
    {
    case TSFFI_INIT_ACTION:
        data = malloc(sizeof(struct timer_data));
        if(data == NULL)
            return TSFFI_ERROR_EXCEPTION;

        data->action_data = action_data;

        delay = (int)input->real_data;
        if(delay < 0)
        {
            data->flags = TIMER_FLAG_FINISHED;

            action_data->execif->signal_action(action_data->execif_data);
        }
        else
        {
            data->flags = 0;

            data->timer_id = SetTimer(
                                      NULL,
                                      0,
                                      (unsigned int)delay,
                                      &TimerEvent
                                     );
            if(data->timer_id == 0)
                goto set_timer_failed;

            error = FFILib_AddID(data->timer_id, data, &timer_id_hash);
            if(error != FFILIB_ERROR_NONE)
                goto add_id_failed;
        }

        *user_action_data = data;

        break;

    case TSFFI_RUNNING_ACTION:
        data = (struct timer_data*)*user_action_data;

        data->flags |= TIMER_FLAG_RUNNING;

        break;

    case TSFFI_UPDATE_ACTION:
        data = (struct timer_data*)*user_action_data;

        data->flags ^= TIMER_FLAG_RUNNING;

        if(data->flags&TIMER_FLAG_TRIGGERED)
        {
            struct tsffi_invocation_data* action_data;

            data->flags ^= TIMER_FLAG_TRIGGERED;
            data->flags |= TIMER_FLAG_FINISHED;

            action_data = data->action_data;

            action_data->execif->signal_action(data->action_data->execif_data);
        }
        else if(data->flags&TIMER_FLAG_TRIGGERED_WAIT)
        {
            struct tsffi_invocation_data* action_data;

            data->flags ^= TIMER_FLAG_TRIGGERED_WAIT;
            data->flags |= TIMER_FLAG_TRIGGERED;

            action_data = data->action_data;

            action_data->execif->signal_action(data->action_data->execif_data);
        }

        break;

    case TSFFI_QUERY_ACTION:
        data = (struct timer_data*)*user_action_data;

        if(data->flags&TIMER_FLAG_FINISHED)
            *state = TSFFI_ACTION_STATE_FINISHED;
        else if(data->flags&TIMER_FLAG_TRIGGERED)
            *state = TSFFI_ACTION_STATE_TRIGGERED;
        else
            *state = TSFFI_ACTION_STATE_PENDING;

        break;

    case TSFFI_STOP_ACTION:
        data = (struct timer_data*)*user_action_data;

        if(!(data->flags&(TIMER_FLAG_FINISHED|TIMER_FLAG_TRIGGERED|TIMER_FLAG_TRIGGERED_WAIT)))
        {
            KillTimer(NULL, data->timer_id);
            FFILib_RemoveID(data->timer_id, &timer_id_hash);
        }

        free(data);

        break;
    }

   return TSFFI_ERROR_NONE;

add_id_failed:
    KillTimer(NULL, data->timer_id);
set_timer_failed:
    free(data);

    return TSFFI_ERROR_EXCEPTION;
}

int Time_Action_PTimer (
                        struct tsffi_invocation_data* action_data,
                        unsigned int                  request,
                        void*                         group_data,
                        union tsffi_value*            input,
                        unsigned int*                 state,
                        void**                        user_action_data
                       )
{
    struct timer_data* data;
    int                error;
    int                delay;
    int                result;

    error = FFILib_SynchronousFFIAction(
                                        &Time_Action_PTimer,
                                        action_data,
                                        request,
                                        group_data,
                                        input,
                                        state,
                                        user_action_data,
                                        &ffilib_control_thread,
                                        &result
                                       );
    if(error == FFILIB_ERROR_NONE)
        return result;
    else if(error != FFILIB_ERROR_THREAD_SWITCH)
        return TSFFI_ERROR_EXCEPTION;

    switch(request)
    {
    case TSFFI_INIT_ACTION:
        data = malloc(sizeof(struct timer_data));
        if(data == NULL)
            return TSFFI_ERROR_EXCEPTION;

        data->action_data = action_data;

        delay = (int)input->real_data;
        if(delay < 0)
        {
            data->flags = TIMER_FLAG_FINISHED;

            action_data->execif->signal_action(action_data->execif_data);
        }
        else
        {
            data->flags = TIMER_FLAG_PERIODIC;

            data->timer_id = SetTimer(
                                      NULL,
                                      0,
                                      (unsigned int)delay,
                                      &TimerEvent
                                     );
            if(data->timer_id == 0)
                goto set_timer_failed;

            error = FFILib_AddID(data->timer_id, data, &timer_id_hash);
            if(error != FFILIB_ERROR_NONE)
                goto add_id_failed;
        }

        *user_action_data = data;

        break;

    case TSFFI_RUNNING_ACTION:
        data = (struct timer_data*)*user_action_data;

        data->flags |= TIMER_FLAG_RUNNING;

        break;

    case TSFFI_UPDATE_ACTION:
        data = (struct timer_data*)*user_action_data;

        data->flags ^= TIMER_FLAG_RUNNING;

        if(data->flags&TIMER_FLAG_TRIGGERED)
        {
            struct tsffi_invocation_data* action_data;

            data->flags ^= TIMER_FLAG_TRIGGERED;

            action_data = data->action_data;

            action_data->execif->signal_action(data->action_data->execif_data);
        }

        if(data->flags&TIMER_FLAG_TRIGGERED_WAIT)
        {
            struct tsffi_invocation_data* action_data;

            data->flags ^= TIMER_FLAG_TRIGGERED_WAIT;
            data->flags |= TIMER_FLAG_TRIGGERED;

            action_data = data->action_data;

            action_data->execif->signal_action(data->action_data->execif_data);
        }

        break;

    case TSFFI_QUERY_ACTION:
        data = (struct timer_data*)*user_action_data;

        if(data->flags&TIMER_FLAG_FINISHED)
            *state = TSFFI_ACTION_STATE_FINISHED;
        else if(data->flags&TIMER_FLAG_TRIGGERED)
            *state = TSFFI_ACTION_STATE_TRIGGERED;
        else
            *state = TSFFI_ACTION_STATE_PENDING;

        break;

    case TSFFI_STOP_ACTION:
        data = (struct timer_data*)*user_action_data;

        if(!(data->flags&TIMER_FLAG_FINISHED))
        {
            KillTimer(NULL, data->timer_id);
            FFILib_RemoveID(data->timer_id, &timer_id_hash);
        }

        free(data);

        break;
    }

   return TSFFI_ERROR_NONE;

add_id_failed:
    KillTimer(NULL, data->timer_id);
set_timer_failed:
    free(data);

    return TSFFI_ERROR_EXCEPTION;
}

int Time_Action_VTimer (
                        struct tsffi_invocation_data* action_data,
                        unsigned int                  request,
                        void*                         group_data,
                        union tsffi_value*            input,
                        unsigned int*                 state,
                        void**                        user_action_data
                       )
{
    struct timer_data* data;
    int                error;
    int                delay;
    int                result;

    error = FFILib_SynchronousFFIAction(
                                        &Time_Action_VTimer,
                                        action_data,
                                        request,
                                        group_data,
                                        input,
                                        state,
                                        user_action_data,
                                        &ffilib_control_thread,
                                        &result
                                       );
    if(error == FFILIB_ERROR_NONE)
        return result;
    else if(error != FFILIB_ERROR_THREAD_SWITCH)
        return TSFFI_ERROR_EXCEPTION;

    switch(request)
    {
    case TSFFI_INIT_ACTION:
        data = malloc(sizeof(struct timer_data));
        if(data == NULL)
            return TSFFI_ERROR_EXCEPTION;

        data->action_data = action_data;

        delay = (int)input->real_data;
        if(delay < 0)
        {
            data->flags = TIMER_FLAG_FINISHED;

            action_data->execif->signal_action(action_data->execif_data);
        }
        else
        {
            data->flags = 0;

            data->timer_id = SetTimer(
                                      NULL,
                                      0,
                                      (unsigned int)delay,
                                      &TimerEvent
                                     );
            if(data->timer_id == 0)
                goto set_timer_failed;

            error = FFILib_AddID(data->timer_id, data, &timer_id_hash);
            if(error != FFILIB_ERROR_NONE)
                goto add_id_failed;
        }

        *user_action_data = data;

        break;

    case TSFFI_RUNNING_ACTION:
        data = (struct timer_data*)*user_action_data;

        data->flags |= TIMER_FLAG_RUNNING;

        break;

    case TSFFI_UPDATE_ACTION:
        data = (struct timer_data*)*user_action_data;

        if(
           !(data->flags&(TIMER_FLAG_FINISHED|TIMER_FLAG_TRIGGERED|TIMER_FLAG_TRIGGERED_WAIT)) &&
           data->timer_id != 0
          )
        {
            KillTimer(NULL, data->timer_id);
            FFILib_RemoveID(data->timer_id, &timer_id_hash);
        }

        delay = (int)input->real_data;
        if(delay < 0)
        {
            data->flags = TIMER_FLAG_FINISHED;

            action_data->execif->signal_action(action_data->execif_data);
        }
        else
        {
            data->flags = 0;

            data->timer_id = SetTimer(
                                      NULL,
                                      0,
                                      (unsigned int)delay,
                                      &TimerEvent
                                     );
            if(data->timer_id == 0)
                return TSFFI_ERROR_EXCEPTION;

            error = FFILib_AddID(data->timer_id, data, &timer_id_hash);
            if(error != FFILIB_ERROR_NONE)
            {
                KillTimer(NULL, data->timer_id);

                return TSFFI_ERROR_EXCEPTION;
            }
        }

        break;

    case TSFFI_QUERY_ACTION:
        data = (struct timer_data*)*user_action_data;

        if(data->flags&TIMER_FLAG_FINISHED)
            *state = TSFFI_ACTION_STATE_FINISHED;
        else if(data->flags&TIMER_FLAG_TRIGGERED)
            *state = TSFFI_ACTION_STATE_TRIGGERED;
        else
            *state = TSFFI_ACTION_STATE_PENDING;

        break;

    case TSFFI_STOP_ACTION:
        data = (struct timer_data*)*user_action_data;

        if(
           !(data->flags&(TIMER_FLAG_FINISHED|TIMER_FLAG_TRIGGERED|TIMER_FLAG_TRIGGERED_WAIT)) &&
           data->timer_id != 0
          )
        {
            KillTimer(NULL, data->timer_id);
            FFILib_RemoveID(data->timer_id, &timer_id_hash);
        }

        free(data);

        break;
    }

   return TSFFI_ERROR_NONE;

add_id_failed:
    KillTimer(NULL, data->timer_id);
set_timer_failed:
    free(data);

    return TSFFI_ERROR_EXCEPTION;
}

