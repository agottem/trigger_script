/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <notify/pipe.h>
#include <notify/init.h>
#include <notify/error.h>

#include <tsffi/error.h>
#include <ffilib/thread.h>
#include <ffilib/module.h>
#include <ffilib/idhash.h>
#include <ffilib/error.h>

#include <stdio.h>
#include <string.h>


#define MAX_EXCEPTION_TEXT_LENGTH 1024

#define PIPE_POLL_TIMER_PERIOD 100


static int   ReadLine        (HANDLE, char**, unsigned int*);
static char* ExtractArgument (char*, unsigned int, int, struct tsffi_invocation_data*);

static int PerformPipeListen (
                              struct notify_pipe_data*,
                              struct tsffi_invocation_data*,
                              unsigned int,
                              unsigned int*
                             );

static VOID CALLBACK PipePollTimerProc (HWND, UINT, UINT, DWORD);

static int PipePollTimer    (void*);
static int StartPipePolling (void*);
static int StopPipePolling  (void*);


FFILIB_DECLARE_STATIC_ID_HASH(timer_id_hash);


static int ReadLine (HANDLE file, char** new_line, unsigned int* arguments)
{
    char*        line;
    unsigned int argument_count;
    int          bytes_read;

    bytes_read = 0;

    while(1)
    {
        char*        scan_line;
        unsigned int non_whitespace_count;
        DWORD        read_size;
        BOOL         error;
        char         read_character;
        char         current_character;

        do
        {
            error = ReadFile(file, &read_character, 1, &read_size, NULL);

            bytes_read += read_size;

            if(error == 0 || read_size == 0)
                goto no_eol_found;
        }while(read_character != '\n');

        SetFilePointer(file, -bytes_read, NULL, FILE_CURRENT);

        line = malloc(bytes_read);
        if(line == NULL)
            return NOTIFY_ERROR_MEMORY;

        error = ReadFile(file, line, bytes_read, &read_size, NULL);

        bytes_read--;
        if(error == 0 || line[bytes_read] != '\n')
            goto read_to_eol_failed;

        line[bytes_read] = 0;

        bytes_read--;
        if(line[bytes_read] == '\r')
            line[bytes_read] = 0;

        scan_line = line;

        argument_count       = 0;
        non_whitespace_count = 0;
        do
        {
            current_character = *scan_line;

            switch(current_character)
            {
            case 0:
            case '\t':
            case '\r':
            case ' ':
                if(non_whitespace_count != 0)
                {
                    *scan_line = 0;

                    while(current_character != 0)
                    {
                        scan_line++;

                        current_character = *scan_line;
                        if(
                           current_character != ' ' &&
                           current_character != '\t' &&
                           current_character != '\r'
                          )
                        {
                            break;
                        }
                    }

                    argument_count++;
                    non_whitespace_count = 0;
                }
                else
                    scan_line++;

                break;

            default:
                non_whitespace_count++;
                scan_line++;

                break;
            }
        }while(current_character != 0);

        if(argument_count == 0)
            free(line);
        else
            break;
    }

    *new_line  = line;
    *arguments = argument_count;

    return NOTIFY_ERROR_NONE;

read_to_eol_failed:
    free(line);
no_eol_found:
    SetFilePointer(file, -bytes_read, NULL, FILE_CURRENT);

    *new_line = NULL;

    return NOTIFY_ERROR_NONE;
}

static char* ExtractArgument (
                              char*                         line,
                              unsigned int                  line_argument_count,
                              int                           argument,
                              struct tsffi_invocation_data* invocation_data
                             )
{
    char*  duplicated_argument;
    size_t alloc_size;

    if(line == NULL || argument < 0 || (unsigned int)argument >= line_argument_count)
        line = "";
    else
    {
        char current_character;

        while(argument--)
            line += strlen(line)+1;

        while(1)
        {
            current_character = *line;

            if(
               current_character != ' ' &&
               current_character != '\t' &&
               current_character != '\r'
              )
            {
                break;
            }

            line++;
        }
    }

    alloc_size = strlen(line)+1;

    duplicated_argument = invocation_data->execif->allocate_memory(
                                                                   invocation_data->execif_data,
                                                                   alloc_size
                                                                  );
    if(duplicated_argument == NULL)
        return NULL;

    strcpy(duplicated_argument, line);

    return duplicated_argument;
}

static int PerformPipeListen (
                              struct notify_pipe_data*      pipe_data,
                              struct tsffi_invocation_data* action_data,
                              unsigned int                  request,
                              unsigned int*                 state
                             )
{
    char  text[MAX_EXCEPTION_TEXT_LENGTH];
    char* line;
    int   error;

    switch(request)
    {
    case TSFFI_INIT_ACTION:
        error = ReadLine(pipe_data->file_handle, &line, &pipe_data->line_argument_count);
        if(error != NOTIFY_ERROR_NONE)
            goto error_reading_file;

        pipe_data->current_line = line;
        pipe_data->action_data  = action_data;

        if(line != NULL)
            action_data->execif->signal_action(action_data->execif_data);

        break;

    case TSFFI_RUNNING_ACTION:
        break;

    case TSFFI_UPDATE_ACTION:
        if(pipe_data->current_line != NULL)
        {
            free(pipe_data->current_line);

            pipe_data->current_line = NULL;
        }

        error = ReadLine(pipe_data->file_handle, &line, &pipe_data->line_argument_count);
        if(error != NOTIFY_ERROR_NONE)
            goto error_reading_file;

        pipe_data->current_line = line;

        if(line != NULL)
            action_data->execif->signal_action(action_data->execif_data);

        break;

    case TSFFI_QUERY_ACTION:
        if(pipe_data->current_line != NULL)
            *state = TSFFI_ACTION_STATE_TRIGGERED;
        else
            *state = TSFFI_ACTION_STATE_PENDING;

        break;

    case TSFFI_STOP_ACTION:
        break;
    }

   return TSFFI_ERROR_NONE;

error_reading_file:
    _snprintf(
              text,
              MAX_EXCEPTION_TEXT_LENGTH,
              "function=%s line=%d: The file could not be read",
              action_data->unit_name,
              action_data->unit_location
             );

    text[MAX_EXCEPTION_TEXT_LENGTH-1] = 0;

    action_data->execif->set_exception_text(action_data->execif_data, text);

    return TSFFI_ERROR_EXCEPTION;
}

static VOID CALLBACK PipePollTimerProc (
                                        HWND  window,
                                        UINT  message,
                                        UINT  timer_id,
                                        DWORD time
                                       )
{
    int result;
    int error;

    error = FFILib_SynchronousFunction(&PipePollTimer, &timer_id, &ffilib_control_thread, &result);
    if(error == FFILIB_ERROR_THREAD_SWITCH)
        PipePollTimer(&timer_id);
}

static int PipePollTimer (void* user_data)
{
    struct notify_module_data* module_data;
    struct notify_pipe_data*   open_pipes;
    UINT*                      timer_id;

    timer_id    = user_data;
    module_data = FFILib_GetIDData(*timer_id, &timer_id_hash);

    if(module_data == NULL)
        return NOTIFY_ERROR_NONE;

    for(
        open_pipes = module_data->open_pipes;
        open_pipes != NULL;
        open_pipes = open_pipes->next_pipe
       )
    {
        char* line;
        int   error;

        if(open_pipes->current_line != NULL)
            continue;

        error = ReadLine(open_pipes->file_handle, &line, &open_pipes->line_argument_count);
        if(error != NOTIFY_ERROR_NONE)
            continue;

        open_pipes->current_line = line;

        if(line != NULL)
        {
            struct tsffi_invocation_data* action_data;

            action_data = open_pipes->action_data;

            action_data->execif->signal_action(action_data->execif_data);
        }
    }

    return NOTIFY_ERROR_NONE;
}

static int StartPipePolling (void* user_data)
{
    struct notify_module_data* module_data;
    unsigned int               timer_id;
    int                        error;

    module_data = user_data;

    timer_id = SetTimer(NULL, 0, PIPE_POLL_TIMER_PERIOD, &PipePollTimerProc);
    if(timer_id == 0)
        goto set_timer_failed;

    error = FFILib_AddID(timer_id, module_data, &timer_id_hash);
    if(error != FFILIB_ERROR_NONE)
        goto add_id_failed;

    module_data->pipe_poll_timer_id = timer_id;

    return TSFFI_ERROR_NONE;

add_id_failed:
    KillTimer(NULL, timer_id);

set_timer_failed:
    return TSFFI_ERROR_EXCEPTION;
}

static int StopPipePolling (void* user_data)
{
    struct notify_module_data* module_data;
    unsigned int               timer_id;

    module_data = user_data;
    timer_id    = module_data->pipe_poll_timer_id;

    KillTimer(NULL, timer_id);
    FFILib_RemoveID(timer_id, &timer_id_hash);

    module_data->pipe_poll_timer_id = 0;

    return TSFFI_ERROR_NONE;
}


int Notify_StartPipePolling (struct notify_module_data* module_data)
{
    int result;

    FFILib_SynchronousFunction(&StartPipePolling, module_data, &ffilib_control_thread, &result);

    return result;

}

int Notify_StopPipePolling (struct notify_module_data* module_data)
{
    int result;

    FFILib_SynchronousFunction(&StartPipePolling, module_data, &ffilib_control_thread, &result);

    return result;
}

void Notify_DestroyPipes (struct notify_module_data* module_data)
{
    struct notify_pipe_data* pipes;

    if(module_data->pipe_poll_timer_id != 0)
        StopPipePolling(module_data);

    pipes = module_data->open_pipes;

    while(pipes != NULL)
    {
        struct notify_pipe_data* free_pipe;

        free_pipe = pipes;
        pipes     = pipes->next_pipe;

        if(free_pipe->current_line != NULL)
            free(free_pipe->current_line);

        CloseHandle(free_pipe->file_handle);
        free(free_pipe);
    }
}

int Notify_Pipe (
                 struct tsffi_invocation_data* invocation_data,
                 void*                         group_data,
                 union tsffi_value*            output,
                 union tsffi_value*            input
                )
{
    struct notify_module_data* module_data;
    struct notify_pipe_data*   pipe_data;
    HANDLE                     file_handle;
    unsigned int               pipe_id;
    unsigned int               unit_invocation_id;
    int                        error;
    int                        result;

    error = FFILib_SynchronousFFIFunction(
                                          &Notify_Pipe,
                                          invocation_data,
                                          group_data,
                                          output,
                                          input,
                                          &ffilib_control_thread,
                                          &result
                                         );
    if(error == FFILIB_ERROR_NONE)
        return result;
    else if(error != FFILIB_ERROR_THREAD_SWITCH)
        return TSFFI_ERROR_EXCEPTION;

    pipe_data = malloc(sizeof(struct notify_pipe_data));
    if(pipe_data == NULL)
        goto allocate_definition_failed;

    file_handle = CreateFile(
                             input->string_data,
                             GENERIC_READ,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             NULL,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                            );
    if(file_handle == INVALID_HANDLE_VALUE)
    {
        char text[MAX_EXCEPTION_TEXT_LENGTH];

        _snprintf(
                  text,
                  MAX_EXCEPTION_TEXT_LENGTH,
                  "function=%s line=%d: The specified file could not be opened",
                  invocation_data->unit_name,
                  invocation_data->unit_location
                 );

        text[MAX_EXCEPTION_TEXT_LENGTH-1] = 0;

        invocation_data->execif->set_exception_text(invocation_data->execif_data, text);

        goto open_file_failed;
    }

    module_data = group_data;

    pipe_id = module_data->next_pipe_id;

    pipe_data->pipe_id             = pipe_id;
    pipe_data->file_handle         = file_handle;
    pipe_data->current_line        = NULL;
    pipe_data->line_argument_count = 0;

    unit_invocation_id = invocation_data->unit_invocation_id;

    error = FFILib_AddID(unit_invocation_id, pipe_data, &module_data->unit_hash);
    if(error != FFILIB_ERROR_NONE)
        goto add_unit_invocation_id_failed;

    error = FFILib_AddID(pipe_id, pipe_data, &module_data->pipe_hash);
    if(error != FFILIB_ERROR_NONE)
        goto add_pipe_id_failed;

    module_data->next_pipe_id++;

    pipe_data->next_pipe    = module_data->open_pipes;
    module_data->open_pipes = pipe_data;

    output->int_data = pipe_id;

    return TSFFI_ERROR_NONE;

add_pipe_id_failed:
    FFILib_RemoveID(unit_invocation_id, &module_data->pipe_hash);
add_unit_invocation_id_failed:
    CloseHandle(file_handle);
open_file_failed:
    free(pipe_data);

allocate_definition_failed:
    return TSFFI_ERROR_EXCEPTION;
}

int Notify_ReadPipe (
                     struct tsffi_invocation_data* invocation_data,
                     void*                         group_data,
                     union tsffi_value*            output,
                     union tsffi_value*            input
                    )
{
    struct notify_module_data* module_data;
    struct notify_pipe_data*   pipe_data;
    char*                      argument;
    int                        error;
    int                        result;

    error = FFILib_SynchronousFFIFunction(
                                          &Notify_ReadPipe,
                                          invocation_data,
                                          group_data,
                                          output,
                                          input,
                                          &ffilib_control_thread,
                                          &result
                                         );
    if(error == FFILIB_ERROR_NONE)
        return result;
    else if(error != FFILIB_ERROR_THREAD_SWITCH)
        return TSFFI_ERROR_EXCEPTION;

    module_data = group_data;
    pipe_data   = FFILib_GetIDData(
                                   invocation_data->unit_invocation_id,
                                   &module_data->unit_hash
                                  );
    if(pipe_data == NULL)
    {
        char text[MAX_EXCEPTION_TEXT_LENGTH];

        _snprintf(
                  text,
                  MAX_EXCEPTION_TEXT_LENGTH,
                  "function=%s line=%d: Attempting to read from pipe which hasn't been created using 'pipe()'",
                  invocation_data->unit_name,
                  invocation_data->unit_location
                 );

        text[MAX_EXCEPTION_TEXT_LENGTH-1] = 0;

        invocation_data->execif->set_exception_text(invocation_data->execif_data, text);

        return TSFFI_ERROR_EXCEPTION;
    }

    argument = ExtractArgument(
                               pipe_data->current_line,
                               pipe_data->line_argument_count,
                               input->int_data,
                               invocation_data
                              );
    if(argument == NULL)
        return TSFFI_ERROR_EXCEPTION;

    output->string_data = (tsffi_string)argument;

    return TSFFI_ERROR_NONE;
}

int Notify_HReadPipe (
                      struct tsffi_invocation_data* invocation_data,
                      void*                         group_data,
                      union tsffi_value*            output,
                      union tsffi_value*            input
                     )
{
    struct notify_module_data* module_data;
    struct notify_pipe_data*   pipe_data;
    char*                      argument;
    int                        error;
    int                        result;

    error = FFILib_SynchronousFFIFunction(
                                          &Notify_HReadPipe,
                                          invocation_data,
                                          group_data,
                                          output,
                                          input,
                                          &ffilib_control_thread,
                                          &result
                                         );
    if(error == FFILIB_ERROR_NONE)
        return result;
    else if(error != FFILIB_ERROR_THREAD_SWITCH)
        return TSFFI_ERROR_EXCEPTION;

    module_data = group_data;
    pipe_data   = FFILib_GetIDData(
                                   input[0].int_data,
                                   &module_data->pipe_hash
                                  );
    if(pipe_data == NULL)
    {
        char text[MAX_EXCEPTION_TEXT_LENGTH];

        _snprintf(
                  text,
                  MAX_EXCEPTION_TEXT_LENGTH,
                  "function=%s line=%d: Attempting to read from pipe using an invalid handle, retrieve a handle by calling 'pipe()'",
                  invocation_data->unit_name,
                  invocation_data->unit_location
                 );

        text[MAX_EXCEPTION_TEXT_LENGTH-1] = 0;

        invocation_data->execif->set_exception_text(invocation_data->execif_data, text);

        return TSFFI_ERROR_EXCEPTION;
    }

    argument = ExtractArgument(
                               pipe_data->current_line,
                               pipe_data->line_argument_count,
                               input[1].int_data,
                               invocation_data
                              );
    if(argument == NULL)
        return TSFFI_ERROR_EXCEPTION;

    output->string_data = (tsffi_string)argument;

    return TSFFI_ERROR_NONE;
}

int Notify_Action_ListenPipe (
                              struct tsffi_invocation_data* action_data,
                              unsigned int                  request,
                              void*                         group_data,
                              union tsffi_value*            input,
                              unsigned int*                 state,
                              void**                        user_action_data
                             )
{
    struct notify_module_data* module_data;
    struct notify_pipe_data*   pipe_data;
    int                        error;
    int                        result;

    error = FFILib_SynchronousFFIAction(
                                        &Notify_Action_ListenPipe,
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

    module_data = group_data;

    if(request == TSFFI_INIT_ACTION)
    {
        pipe_data = FFILib_GetIDData(
                                     action_data->unit_invocation_id,
                                     &module_data->unit_hash
                                    );

        *user_action_data = pipe_data;
    }
    else
        pipe_data = *user_action_data;

    if(pipe_data == NULL)
    {
        char text[MAX_EXCEPTION_TEXT_LENGTH];

        _snprintf(
                  text,
                  MAX_EXCEPTION_TEXT_LENGTH,
                  "function=%s line=%d: Attempting to listen on a pipe which hasn't been created using 'pipe()'",
                  action_data->unit_name,
                  action_data->unit_location
                 );

        text[MAX_EXCEPTION_TEXT_LENGTH-1] = 0;

        action_data->execif->set_exception_text(action_data->execif_data, text);

        return TSFFI_ERROR_EXCEPTION;
    }

    error = PerformPipeListen(pipe_data, action_data, request, state);

    return error;
}

int Notify_Action_HListenPipe (
                               struct tsffi_invocation_data* action_data,
                               unsigned int                  request,
                               void*                         group_data,
                               union tsffi_value*            input,
                               unsigned int*                 state,
                               void**                        user_action_data
                              )
{
    struct notify_module_data* module_data;
    struct notify_pipe_data*   pipe_data;
    int                        error;
    int                        result;

    error = FFILib_SynchronousFFIAction(
                                        &Notify_Action_HListenPipe,
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

    module_data = group_data;

    if(request == TSFFI_INIT_ACTION)
    {
        pipe_data = FFILib_GetIDData(
                                     input->int_data,
                                     &module_data->pipe_hash
                                    );

        *user_action_data = pipe_data;
    }
    else
        pipe_data = *user_action_data;

    if(pipe_data == NULL)
    {
        char text[MAX_EXCEPTION_TEXT_LENGTH];

        _snprintf(
                  text,
                  MAX_EXCEPTION_TEXT_LENGTH,
                  "function=%s line=%d: Invalid handle used, retrieve a handle by calling 'pipe()'",
                  action_data->unit_name,
                  action_data->unit_location
                 );

        text[MAX_EXCEPTION_TEXT_LENGTH-1] = 0;

        action_data->execif->set_exception_text(action_data->execif_data, text);

        return TSFFI_ERROR_EXCEPTION;
    }

    error = PerformPipeListen(pipe_data, action_data, request, state);

    return error;
}

