/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <ffilib/thread.h>
#include <ffilib/error.h>

#include <windows.h>
#include <intrin.h>


#define FFI_CONTROL_MESSAGE_FFI_ACTION     (WM_USER+0)
#define FFI_CONTROL_MESSAGE_FFI_FUNCTION   (WM_USER+1)
#define FFI_CONTROL_MESSAGE_ASYNC_FUNCTION (WM_USER+2)
#define FFI_CONTROL_MESSAGE_SYNC_FUNCTION  (WM_USER+3)

#define POST_MESSAGE_RETRY_DELAY 100

#define FLAG_MESSAGE_PROCESSED 0x01


struct ffi_action_message_data
{
    LONG flags;

    tsffi_action_controller controller;

    struct tsffi_invocation_data* invocation_data;
    unsigned int                  request;
    void*                         group_data;
    union tsffi_value*            input;
    unsigned int*                 state;
    void**                        user_action_data;

    int* result;
};

struct ffi_function_message_data
{
    LONG flags;

    tsffi_function function;

    struct tsffi_invocation_data* invocation_data;
    void*                         group_data;
    union tsffi_value*            output;
    union tsffi_value*            input;

    int* result;
};

struct thread_function_message_data
{
    LONG flags;

    void* user_data;
    int*  result;
};


static DWORD WINAPI FFIControlThread (LPVOID);


static HINSTANCE thread_instance;


static LRESULT CALLBACK ThreadMessageProc (
                                           HWND   window_handle,
                                           UINT   message_id,
                                           WPARAM wparam,
                                           LPARAM lparam
                                          )
{
    struct ffilib_thread_data*           thread_data;
    struct ffi_action_message_data*      ffi_action_message;
    struct ffi_function_message_data*    ffi_function_message;
    struct thread_function_message_data* function_message;
    HANDLE                               message_processed_event;
    ffilib_thread_function               thread_function;

    thread_data = (struct ffilib_thread_data*)GetWindowLongPtr(window_handle, GWLP_USERDATA);
    if(thread_data == NULL)
        return DefWindowProc(window_handle, message_id, wparam, lparam);

    message_processed_event = thread_data->message_processed_event;

    ResetEvent(message_processed_event);

    switch(message_id)
    {
    case FFI_CONTROL_MESSAGE_FFI_ACTION:
        ffi_action_message = (struct ffi_action_message_data*)wparam;

        *ffi_action_message->result = ffi_action_message->controller(
                                                                     ffi_action_message->invocation_data,
                                                                     ffi_action_message->request,
                                                                     ffi_action_message->group_data,
                                                                     ffi_action_message->input,
                                                                     ffi_action_message->state,
                                                                     ffi_action_message->user_action_data
                                                                    );

        _InterlockedOr(&ffi_action_message->flags, FLAG_MESSAGE_PROCESSED);

        break;

    case FFI_CONTROL_MESSAGE_FFI_FUNCTION:
        ffi_function_message = (struct ffi_function_message_data*)wparam;

        *ffi_function_message->result = ffi_function_message->function(
                                                                       ffi_function_message->invocation_data,
                                                                       ffi_function_message->group_data,
                                                                       ffi_function_message->output,
                                                                       ffi_function_message->input
                                                                      );

        _InterlockedOr(&ffi_function_message->flags, FLAG_MESSAGE_PROCESSED);

        break;

    case FFI_CONTROL_MESSAGE_ASYNC_FUNCTION:
        thread_function = (ffilib_thread_function)wparam;

        thread_function((void*)lparam);

        break;

    case FFI_CONTROL_MESSAGE_SYNC_FUNCTION:
        thread_function  = (ffilib_thread_function)wparam;
        function_message = (struct thread_function_message_data*)lparam;

        *function_message->result = thread_function(function_message->user_data);

        _InterlockedOr(&function_message->flags, FLAG_MESSAGE_PROCESSED);

        break;

    default:
        return DefWindowProc(window_handle, message_id, wparam, lparam);
    }

    SetEvent(message_processed_event);

    return 0;
}

static DWORD WINAPI FFIControlThread (LPVOID user_data)
{
    struct ffilib_thread_data* thread_data;
    HANDLE                     message_processed_event;
    HWND                       message_window;
    BOOL                       quit_thread;

    thread_data             = user_data;
    message_processed_event = thread_data->message_processed_event;

    message_window = CreateWindowEx(
                                    0,
                                    "FFILibThreadWindow",
                                    "",
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    HWND_MESSAGE,
                                    NULL,
                                    thread_instance,
                                    NULL
                                   );

    thread_data->message_window = message_window;

    SetWindowLongPtr(message_window, GWLP_USERDATA, (LONG_PTR)thread_data);
    SetEvent(message_processed_event);

    if(message_window == NULL)
        goto finish_thread;

    do
    {
        MSG message_data;

        quit_thread = GetMessage(&message_data, NULL, 0, 0);

        TranslateMessage(&message_data);
        DispatchMessage(&message_data);
    }while(quit_thread != 0);

finish_thread:
    return 0;
}


int FFILib_InitializeThread (HINSTANCE instance)
{
    WNDCLASSEX window_class;
    ATOM       error;

    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = 0;
    window_class.lpfnWndProc   = &ThreadMessageProc;
    window_class.cbClsExtra    = 0;
    window_class.cbWndExtra    = 0;
    window_class.hInstance     = instance;
    window_class.hIcon         = NULL;
    window_class.hCursor       = NULL;
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName  = NULL;
    window_class.lpszClassName = "FFILibThreadWindow";
    window_class.hIconSm       = NULL;

    error = RegisterClassEx(&window_class);
    if(error == 0)
        return FFILIB_ERROR_SYSTEM_CALL;

    thread_instance = instance;

    return FFILIB_ERROR_NONE;
}

void FFILib_ShutdownThread (void)
{
    UnregisterClass("FFILibThreadWindow", thread_instance);
}

int FFILib_StartThread (struct ffilib_thread_data* thread_data)
{
    HANDLE message_processed_event;

    message_processed_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(message_processed_event == NULL)
        goto create_message_processed_event_failed;

    thread_data->message_processed_event = message_processed_event;

    thread_data->thread_handle = CreateThread(
                                              NULL,
                                              0,
                                              &FFIControlThread,
                                              thread_data,
                                              0,
                                              &thread_data->thread_id
                                             );
    if(thread_data->thread_handle == NULL)
        goto create_thread_failed;

    WaitForSingleObject(message_processed_event, INFINITE);

    if(thread_data->message_window == NULL)
        goto create_window_failed;

    return FFILIB_ERROR_NONE;

create_window_failed:
    CloseHandle(thread_data->thread_handle);
create_thread_failed:
    CloseHandle(thread_data->message_processed_event);

create_message_processed_event_failed:
    return FFILIB_ERROR_SYSTEM_CALL;
}

void FFILib_StopThread  (struct ffilib_thread_data* thread_data)
{
    HANDLE thread_handle;
    BOOL   error;

    while(1)
    {
        error = PostThreadMessage(thread_data->thread_id, WM_QUIT, 0, 0);
        if(error == 0)
            Sleep(POST_MESSAGE_RETRY_DELAY);
        else
            break;
    }

    thread_handle = thread_data->thread_handle;

    WaitForSingleObject(thread_handle, INFINITE);

    CloseHandle(thread_handle);
    CloseHandle(thread_data->message_processed_event);
}

int FFILib_SynchronousFFIAction (
                                 tsffi_action_controller       action_controller,
                                 struct tsffi_invocation_data* invocation_data,
                                 unsigned int                  request,
                                 void*                         group_data,
                                 union tsffi_value*            input,
                                 unsigned int*                 state,
                                 void**                        user_action_data,
                                 struct ffilib_thread_data*    thread_data,
                                 int*                          action_result
                                )
{
    struct ffi_action_message_data ffi_action_message;
    HANDLE                             message_processed_event;
    HWND                               message_window;

    if(GetCurrentThreadId() == thread_data->thread_id)
        return FFILIB_ERROR_THREAD_SWITCH;

    ffi_action_message.flags            = 0;
    ffi_action_message.controller       = action_controller;
    ffi_action_message.invocation_data  = invocation_data;
    ffi_action_message.request          = request;
    ffi_action_message.group_data       = group_data;
    ffi_action_message.input            = input;
    ffi_action_message.state            = state;
    ffi_action_message.user_action_data = user_action_data;
    ffi_action_message.result           = action_result;

    message_window = thread_data->message_window;

    while(1)
    {
        BOOL error;

        error = PostMessage(
                            message_window,
                            FFI_CONTROL_MESSAGE_FFI_ACTION,
                            (WPARAM)&ffi_action_message,
                            0
                           );
        if(error == 0)
            Sleep(POST_MESSAGE_RETRY_DELAY);
        else
            break;
    }

    message_processed_event = thread_data->message_processed_event;

    do
    {
        DWORD wait_result;

        wait_result = WaitForSingleObject(message_processed_event, INFINITE);
        if(wait_result != WAIT_OBJECT_0)
            return FFILIB_ERROR_SYSTEM_CALL;
    }while((InterlockedCompareExchange(&ffi_action_message.flags, 0, 0)&FLAG_MESSAGE_PROCESSED) == 0);

    return FFILIB_ERROR_NONE;
}

int FFILib_SynchronousFFIFunction (
                                   tsffi_function                function,
                                   struct tsffi_invocation_data* invocation_data,
                                   void*                         group_data,
                                   union tsffi_value*            output,
                                   union tsffi_value*            input,
                                   struct ffilib_thread_data*    thread_data,
                                   int*                          function_result
                                  )
{
    struct ffi_function_message_data ffi_function_message;
    HANDLE                           message_processed_event;
    HWND                             message_window;

    if(GetCurrentThreadId() == thread_data->thread_id)
        return FFILIB_ERROR_THREAD_SWITCH;

    ffi_function_message.flags           = 0;
    ffi_function_message.function        = function;
    ffi_function_message.invocation_data = invocation_data;
    ffi_function_message.group_data      = group_data;
    ffi_function_message.output          = output;
    ffi_function_message.input           = input;
    ffi_function_message.result          = function_result;

    message_window = thread_data->message_window;

    while(1)
    {
        BOOL error;

        error = PostMessage(
                            message_window,
                            FFI_CONTROL_MESSAGE_FFI_FUNCTION,
                            (WPARAM)&ffi_function_message,
                            0
                           );
        if(error == 0)
            Sleep(POST_MESSAGE_RETRY_DELAY);
        else
            break;
    }

    message_processed_event = thread_data->message_processed_event;

    do
    {
        DWORD wait_result;

        wait_result = WaitForSingleObject(message_processed_event, INFINITE);
        if(wait_result != WAIT_OBJECT_0)
            return FFILIB_ERROR_SYSTEM_CALL;
    }while((InterlockedCompareExchange(&ffi_function_message.flags, 0, 0)&FLAG_MESSAGE_PROCESSED) == 0);

    return FFILIB_ERROR_NONE;
}

void FFILib_AsynchronousFunction (
                                  ffilib_thread_function     function,
                                  void*                      user_data,
                                  struct ffilib_thread_data* thread_data
                                 )
{
    HWND message_window;

    message_window = thread_data->message_window;

    while(1)
    {
        BOOL error;

        error = PostMessage(
                            message_window,
                            FFI_CONTROL_MESSAGE_ASYNC_FUNCTION,
                            (WPARAM)function,
                            (LPARAM)user_data
                           );
        if(error == 0)
            Sleep(POST_MESSAGE_RETRY_DELAY);
        else
            break;
    }
}

int FFILib_SynchronousFunction (
                                ffilib_thread_function     function,
                                void*                      user_data,
                                struct ffilib_thread_data* thread_data,
                                int*                       result
                               )
{
    struct thread_function_message_data function_message;
    HANDLE                              message_processed_event;
    HWND                                message_window;

    if(GetCurrentThreadId() == thread_data->thread_id)
        return FFILIB_ERROR_THREAD_SWITCH;

    function_message.flags     = 0;
    function_message.user_data = user_data;
    function_message.result    = result;

    message_window = thread_data->message_window;

    while(1)
    {
        BOOL error;

        error = PostMessage(
                            message_window,
                            FFI_CONTROL_MESSAGE_SYNC_FUNCTION,
                            (WPARAM)function,
                            (LPARAM)&function_message
                           );
        if(error == 0)
            Sleep(POST_MESSAGE_RETRY_DELAY);
        else
            break;
    }

    message_processed_event = thread_data->message_processed_event;

    do
    {
        DWORD wait_result;

        wait_result = WaitForSingleObject(message_processed_event, INFINITE);
        if(wait_result != WAIT_OBJECT_0)
            return FFILIB_ERROR_SYSTEM_CALL;
    }while((InterlockedCompareExchange(&function_message.flags, 0, 0)&FLAG_MESSAGE_PROCESSED) == 0);

    return FFILIB_ERROR_NONE;
}

