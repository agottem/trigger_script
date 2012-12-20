/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <notify/init.h>

#include <ffilib/module.h>
#include <ffilib/error.h>
#include <tsffi/error.h>


int Notify_BeginModule (
                        struct tsffi_execif*             execif,
                        void*                            execif_data,
                        struct tsffi_registration_group* group,
                        void**                           group_data
                       )
{
    struct notify_module_data* module_data;
    int                        error;

    error = FFILib_BeginModule(execif, execif_data, group, NULL);
    if(error != TSFFI_ERROR_NONE)
        goto begin_ffilib_failed;

    module_data = malloc(sizeof(struct notify_module_data));
    if(module_data == NULL)
        goto allocate_module_data_failed;

    module_data->next_pipe_id       = 1;
    module_data->pipe_poll_timer_id = 0;
    module_data->open_pipes         = NULL;

    FFILib_InitializeHash(&module_data->pipe_hash);
    FFILib_InitializeHash(&module_data->unit_hash);

    *group_data = module_data;

    return TSFFI_ERROR_NONE;

allocate_module_data_failed:
    FFILib_EndModule(execif, execif_data, TSFFI_ERROR_EXCEPTION, group, NULL);

begin_ffilib_failed:
    return TSFFI_ERROR_EXCEPTION;
}

int Notify_ModuleState (
                        struct tsffi_execif*             execif,
                        void*                            execif_data,
                        unsigned int                     state,
                        struct tsffi_registration_group* group,
                        void*                            group_data
                       )
{
    int error;

    switch(state)
    {
    case TSFFI_MODULE_SLEEPING:
        error = Notify_StartPipePolling(group_data);

        break;

    case TSFFI_MODULE_RUNNING:
        error = Notify_StopPipePolling(group_data);

        break;
    }

    return error;
}

void Notify_EndModule (
                       struct tsffi_execif*             execif,
                       void*                            execif_data,
                       int                              error,
                       struct tsffi_registration_group* group,
                       void*                            group_data
                      )
{
    struct notify_module_data* module_data;

    module_data = group_data;

    Notify_DestroyPipes(module_data);

    FFILib_DestroyHash(&module_data->unit_hash);
    FFILib_DestroyHash(&module_data->pipe_hash);
    free(module_data);

    FFILib_EndModule(execif, execif_data, TSFFI_ERROR_EXCEPTION, group, NULL);
}

