/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "run.h"
#include "text.h"
#include "gui.h"
#include "resources.h"
#include "main.h"
#include "ide.h"
#include "plugins.h"
#include "paths.h"
#include "alerts.h"
#include "error.h"

#include <tsdef/def.h>
#include <tsdef/deferror.h>
#include <tsdef/module.h>
#include <tsdef/error.h>
#include <tsint/module.h>
#include <tsint/controller.h>
#include <tsint/execif.h>
#include <tsint/exception.h>
#include <tsint/error.h>
#include <tsutil/compile.h>
#include <tsutil/path.h>
#include <tsutil/error.h>

#include <stdio.h>
#include <malloc.h>
#include <commctrl.h>


#define MAX_TIME_LENGTH 256

#define MAX_NUMBER_LENGTH 1024

#define MESSAGE_COMPILATION_COMPLETE WM_USER+1
#define MESSAGE_EXECUTION_COMPLETE   WM_USER+2
#define MESSAGE_EXECUTION_ALERT      WM_USER+3
#define MESSAGE_EXECUTION_BREAK      WM_USER+4

#define EXECUTION_FLAG_MANUAL_STOP 0x01
#define EXECUTION_FLAG_BREAK       0x02


struct run_module_data
{
    char* invocation;

    WCHAR                    current_unit[MAX_PATH];
    unsigned int             current_location;
    struct tsint_unit_state* current_unit_state;

    HANDLE       controller_event;
    unsigned int controller_mode;

    struct tsdef_module         module_def;
    struct tsdef_def_error_list module_errors;
    int                         compile_result;
    int                         execution_result;
    int                         execution_exception;

    unsigned int execution_flags;

    HANDLE module_thread;
};

struct run_data
{
    HWND invocation_input_window;
    HWND compile_and_run_button;
    HWND stop_button;
    HWND debug_checkbox;
    HWND notification_window;
};


static int  InitializeDialog (HWND);
static void DestroyDialog    (struct run_data*);
static void CloseDialog      (void);

static void ClearNotifications (struct run_data*);
static void AddNotification    (WCHAR*, void*, struct run_data*);
static void GotoNotification   (int, struct run_data*);

static void Alert            (void*, unsigned int, char*);
static void SetExceptionText (void*, char*);

static unsigned int Controller (struct tsint_unit_state*, void*);

static DWORD WINAPI CompileThread (LPVOID);
static DWORD WINAPI ExecuteThread (LPVOID);

static void ResetRunState (struct run_data*);

static void BeginCompile          (struct run_data*);
static void ProcessCompileResult  (struct run_data*);
static void ProcessExecuteResult  (struct run_data*);
static void ProcessExecutionAlert (struct tside_alert*, struct run_data*);
static void ProcessExecutionBreak (struct run_data*);


static struct run_module_data*           current_run_module;
static struct run_module_data            run_module;
static struct tsint_module_abort_signal* abort_signal;
static HANDLE                            control_mode_signal;

static struct tsffi_execif module_execif = {NULL, &Alert, &SetExceptionText, NULL, NULL};


static int InitializeDialog (HWND window)
{
    RECT             notification_area;
    LVCOLUMN         column_data;
    HWND             notification_window;
    struct run_data* data;
    HICON            icon;
    LRESULT          style;

    data = malloc(sizeof(struct run_data));
    if(data == NULL)
        goto allocate_run_data_failed;

    icon = LoadIcon(tside_application_instance, MAKEINTRESOURCE(TSIDE_I_GUI_RUN));
    if(icon == NULL)
        goto load_run_icon_failed;

    SendMessage(window, WM_SETICON, ICON_BIG, (LPARAM)icon);

    notification_window = GetDlgItem(window, TSIDE_C_RUN_NOTIFICATION_LIST);

    GetClientRect(notification_window, &notification_area);

    column_data.mask     = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
    column_data.cx       = notification_area.right/6;
    column_data.pszText  = TSIDE_GetResourceText(TSIDE_S_RUN_NOTIFICATION_TIME);
    column_data.iSubItem = 0;

    SendMessage(notification_window, LVM_INSERTCOLUMN, 0, (LPARAM)&column_data);

    column_data.cx       = notification_area.right*5/6-GetSystemMetrics(SM_CXVSCROLL);
    column_data.pszText  = TSIDE_GetResourceText(TSIDE_S_RUN_NOTIFICATION_MESSAGE);
    column_data.iSubItem = 1;

    SendMessage(notification_window, LVM_INSERTCOLUMN, 1, (LPARAM)&column_data);

    style = SendMessage(notification_window, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);

    style |= LVS_EX_FULLROWSELECT;

    SendMessage(notification_window, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, style);

    data->invocation_input_window = GetDlgItem(window, TSIDE_C_RUN_INVOCATION_INPUT);
    data->compile_and_run_button  = GetDlgItem(window, TSIDE_C_RUN_COMPILE_AND_RUN);
    data->stop_button             = GetDlgItem(window, TSIDE_C_RUN_STOP);
    data->debug_checkbox          = GetDlgItem(window, TSIDE_C_RUN_DEBUG);
    data->notification_window     = notification_window;

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    return TSIDE_ERROR_NONE;

load_run_icon_failed:
    free(data);

allocate_run_data_failed:
    return TSIDE_ERROR_RUN_ERROR;
}

static void DestroyDialog (struct run_data* data)
{
    ClearNotifications(data);

    free(data);
}

static void CloseDialog (void)
{
    NMHDR ide_notification;

    ShowWindow(tside_run_window, SW_HIDE);

    ide_notification.hwndFrom = tside_run_window;
    ide_notification.idFrom   = 0;
    ide_notification.code     = TSIDE_NOTIFY_RUN_CLOSED;

    SendMessage(tside_ide_window, WM_NOTIFY, 0, (LPARAM)&ide_notification);
}

static void ClearNotifications (struct run_data* data)
{
    HWND    notification_window;
    LRESULT item_count;

    notification_window = data->notification_window;
    item_count          = SendMessage(notification_window, LVM_GETITEMCOUNT, 0 ,0);

    while(item_count--)
    {
        LVITEM item;

        item.mask  = LVIF_PARAM;
        item.iItem = item_count;

        SendMessage(notification_window, LVM_GETITEM, item_count, (LPARAM)&item);

        if(item.lParam != 0)
            free((void*)item.lParam);
    }

    SendMessage(notification_window, LVM_DELETEALLITEMS, 0, 0);

    SendMessage(
                tside_ide_window,
                TSIDE_MESSAGE_RUN_CLEAR_ALERTS,
                0,
                0
               );
}

static void AddNotification (WCHAR* message, void* notification_data, struct run_data* data)
{
    WCHAR   formatted_time[MAX_TIME_LENGTH];
    LVITEM  item_data;
    HWND    notification_window;
    LRESULT item_count;

    GetTimeFormat(
                  LOCALE_USER_DEFAULT,
                  TIME_FORCE24HOURFORMAT,
                  NULL,
                  L"HH:mm:ss",
                  formatted_time,
                  MAX_TIME_LENGTH
                 );

    notification_window = data->notification_window;
    item_count          = SendMessage(notification_window, LVM_GETITEMCOUNT, 0, 0);

    item_data.mask     = LVIF_TEXT|LVIF_PARAM;
    item_data.iItem    = item_count;
    item_data.iSubItem = 0;
    item_data.pszText  = formatted_time;
    item_data.lParam   = (LPARAM)notification_data;

    SendMessage(notification_window, LVM_INSERTITEM, item_count, (LPARAM)&item_data);

    item_data.iSubItem = 1;
    item_data.mask     = LVIF_TEXT;
    item_data.pszText  = message;

    SendMessage(notification_window, LVM_SETITEM, item_count, (LPARAM)&item_data);
    SendMessage(notification_window, LVM_ENSUREVISIBLE, item_count, FALSE);
}

static void GotoNotification (int item, struct run_data* data)
{
    LVITEM              item_data;
    struct tside_alert* alert;

    item_data.mask  = LVIF_PARAM;
    item_data.iItem = item;

    SendMessage(data->notification_window, LVM_GETITEM, item, (LPARAM)&item_data);

    if(item_data.lParam == 0)
        return;

    alert = (struct tside_alert*)item_data.lParam;

    SendMessage(tside_ide_window, TSIDE_MESSAGE_RUN_GOTO_ALERT, 0, (LPARAM)alert);
}

static void Alert (void* execif_data, unsigned int severity, char* text)
{
    struct tside_alert* alert;
    struct run_data*    data;
    unsigned int        type;
    BOOL                winapi_error;

    data = execif_data;

    switch(severity)
    {
    default:
    case TSFFI_ALERT_MESSAGE:
        type = TSIDE_ALERT_TYPE_RUNTIME_MESSAGE;

        break;

    case TSFFI_ALERT_WARNING:
        type = TSIDE_ALERT_TYPE_RUNTIME_WARNING;

        break;

    case TSFFI_ALERT_ERROR:
        type = TSIDE_ALERT_TYPE_RUNTIME_ERROR;

        break;
    }

    alert = TSIDE_AlertFromString(text, type);

    do
    {
        winapi_error = PostMessage(tside_run_window, MESSAGE_EXECUTION_ALERT, 0, (LPARAM)alert);
    }while(winapi_error == 0);
}

static void SetExceptionText (void* execif_data, char* text)
{
    struct tside_alert* alert;
    struct run_data*    data;
    BOOL                winapi_error;

    data = execif_data;

    alert = TSIDE_AlertFromString(text, TSIDE_ALERT_TYPE_RUNTIME_ERROR);

    do
    {
        winapi_error = PostMessage(tside_run_window, MESSAGE_EXECUTION_ALERT, 0, (LPARAM)alert);
    }while(winapi_error == 0);
}

static unsigned int Controller (struct tsint_unit_state* state, void* user_data)
{
    struct run_data* data;
    char*            separators;
    char*            unit_name;
    char*            file_name;
    DWORD            winapi_error;
    int              error;

    data      = user_data;
    unit_name = state->unit->name;

    if(state->exception != TSINT_EXCEPTION_NONE)
    {
        char  assembled_exception[MAX_PATH+10+512];
        char* exception_text;

        switch(state->exception)
        {
        case TSINT_EXCEPTION_OUT_OF_MEMORY:
            exception_text = "Out of memory";

            break;

        case TSINT_EXCEPTION_DIVIDE_BY_ZERO:
            exception_text = "Divide by zero";

            break;

        case TSINT_EXCEPTION_FFI:
            exception_text = "Failure in FFI plugin";

            break;

        default:
        case TSINT_EXCEPTION_HALT:
            exception_text= "Instructed to halt";

            break;

        }

        sprintf(
                assembled_exception,
                "function=%s line=%d: %s",
                unit_name,
                state->current_location,
                exception_text
               );

        SetExceptionText(data, assembled_exception);

        run_module.execution_exception = state->exception;

        return TSINT_CONTROL_HALT;
    }

    if(run_module.controller_mode == TSINT_CONTROL_RUN)
        return TSINT_CONTROL_RUN;

    error = TSUtil_FindUnitFile(unit_name, ".ts", &tside_unit_paths, &file_name);
    if(error != TSUTIL_ERROR_NONE)
        return TSINT_CONTROL_STEP_INTO;

    for(separators = file_name; *separators != 0; separators++)
    {
        if(*separators == '/')
            *separators = '\\';
    }

    mbstowcs(run_module.current_unit, file_name, MAX_PATH);

    free(file_name);

    run_module.current_location   = state->current_location;
    run_module.current_unit_state = state;

    do
    {
        winapi_error = PostMessage(tside_run_window, MESSAGE_EXECUTION_BREAK, 0, 0);
    }while(winapi_error == 0);

    WaitForSingleObject(control_mode_signal, INFINITE);

    return run_module.controller_mode;
}

static DWORD WINAPI CompileThread (LPVOID user_data)
{
    struct run_data* data;
    BOOL             winapi_error;

    data = user_data;

    run_module.compile_result = TSUtil_CompileUnit(
                                                   run_module.invocation,
                                                   0,
                                                   &tside_unit_paths,
                                                   ".ts",
                                                   NULL,
                                                   &run_module.module_errors,
                                                   &run_module.module_def
                                                  );

    do
    {
        winapi_error = PostMessage(tside_run_window, MESSAGE_COMPILATION_COMPLETE, 0, 0);
    }while(winapi_error == 0);

    return 0;
}

static DWORD WINAPI ExecuteThread (LPVOID user_data)
{
    struct tsint_controller_data controller;
    struct tsint_execif_data     execif;
    struct run_data*             data;
    BOOL                         winapi_error;
    int                          error;

    data = user_data;

    controller.function  = Controller;
    controller.user_data = data;

    execif.execif    = &module_execif;
    execif.user_data = data;

    error = TSInt_InterpretModule(
                                  &run_module.module_def,
                                  NULL,
                                  NULL,
                                  &controller,
                                  &execif,
                                  abort_signal
                                 );

    run_module.execution_result = error;

    do
    {
        winapi_error = PostMessage(tside_run_window, MESSAGE_EXECUTION_COMPLETE, 0, 0);
    }while(winapi_error == 0);

    return 0;
}

static void ResetRunState (struct run_data* data)
{
    NMHDR ide_notification;
    HMENU system_menu;

    TSDef_DestroyDefErrorList(&run_module.module_errors);
    TSDef_DestroyModule(&run_module.module_def);
    free(run_module.invocation);

    current_run_module = NULL;

    EnableWindow(data->invocation_input_window, TRUE);
    EnableWindow(data->compile_and_run_button, TRUE);
    EnableWindow(data->debug_checkbox, TRUE);
    EnableWindow(data->stop_button, FALSE);

    system_menu = GetSystemMenu(tside_run_window, FALSE);
    if(system_menu != NULL)
        EnableMenuItem(system_menu, SC_CLOSE, MF_ENABLED);

    ide_notification.hwndFrom = tside_run_window;
    ide_notification.idFrom   = 0;
    ide_notification.code     = TSIDE_NOTIFY_RUN_FINISHED;

    SendMessage(tside_ide_window, WM_NOTIFY, 0, (LPARAM)&ide_notification);
}

static void BeginCompile (struct run_data* data)
{
    NMHDR                           ide_notification;
    struct tside_registered_plugin* plugins;
    char*                           invocation_text;
    HMENU                           system_menu;
    HANDLE                          created_thread;
    LRESULT                         check_state;
    LRESULT                         continue_with_run;
    int                             invocation_length;

    if(current_run_module != NULL)
        goto concurrent_compilation_not_supported;

    continue_with_run = SendMessage(tside_ide_window, TSIDE_MESSAGE_RUN_BEGINNING, 0, 0);
    if(continue_with_run == FALSE)
        return;

    ClearNotifications(data);

    system_menu = GetSystemMenu(tside_run_window, FALSE);
    if(system_menu != NULL)
        EnableMenuItem(system_menu, SC_CLOSE, MF_DISABLED|MF_GRAYED);

    invocation_length = GetWindowTextLength(data->invocation_input_window)+1;
    invocation_text   = malloc(invocation_length);
    if(invocation_text == NULL)
        goto allocate_invocation_text_failed;

    GetWindowTextA(data->invocation_input_window, invocation_text, invocation_length);

    run_module.invocation = invocation_text;

    check_state = SendMessage(data->debug_checkbox, BM_GETCHECK, 0, 0);
    if(check_state == BST_CHECKED)
        run_module.controller_mode = TSINT_CONTROL_STEP_INTO;
    else
        run_module.controller_mode = TSINT_CONTROL_RUN;

    TSDef_InitializeModule(&run_module.module_def);

    for(plugins = tside_available_plugins; plugins != NULL; plugins = plugins->next_plugin)
    {
        struct tsffi_registration_group* groups;
        unsigned int                     count;

        groups = plugins->groups;
        count  = plugins->count;

        while(count--)
        {
            int error;

            error = TSDef_AddFFIGroup(plugins->path, groups, &run_module.module_def);
            if(error != TSDEF_ERROR_NONE)
                goto add_ffi_group_failed;

            groups++;
        }
    }

    TSDef_InitializeDefErrorList(&run_module.module_errors);

    current_run_module = &run_module;

    created_thread = CreateThread(NULL, 0, &CompileThread, data, 0, NULL);
    if(created_thread == NULL)
        goto create_compile_thread_failed;

    run_module.module_thread = created_thread;

    EnableWindow(data->invocation_input_window, FALSE);
    EnableWindow(data->compile_and_run_button, FALSE);
    EnableWindow(data->debug_checkbox, FALSE);

    ide_notification.hwndFrom = tside_run_window;
    ide_notification.idFrom   = 0;
    ide_notification.code     = TSIDE_NOTIFY_RUN_COMPILING;

    SendMessage(tside_ide_window, WM_NOTIFY, 0, (LPARAM)&ide_notification);

    AddNotification(
                    TSIDE_GetResourceText(TSIDE_S_VALIDATING_SOURCE),
                    NULL,
                    data
                   );

    return;

create_compile_thread_failed:
    TSDef_DestroyDefErrorList(&run_module.module_errors);
add_ffi_group_failed:
    TSDef_DestroyModule(&run_module.module_def);
    free(invocation_text);
allocate_invocation_text_failed:
concurrent_compilation_not_supported:
    MessageBox(
               tside_run_window,
               TSIDE_GetResourceText(TSIDE_S_INITIALIZE_RUN_ERROR),
               TSIDE_GetResourceText(TSIDE_S_CANNOT_RUN),
               MB_ICONINFORMATION|MB_OK
              );
}

static void ProcessCompileResult (struct run_data* data)
{
    NMHDR                   ide_notification;
    struct tsdef_def_error* def_error;
    HANDLE                  created_thread;

    CloseHandle(run_module.module_thread);

    for(
        def_error = run_module.module_errors.encountered_errors;
        def_error != NULL;
        def_error = def_error->next_error
       )
    {
        struct tside_alert* alert;

        alert = TSIDE_AlertFromDefError(def_error, run_module.invocation);
        if(alert == NULL)
        {
            MessageBox(
                       tside_run_window,
                       TSIDE_GetResourceText(TSIDE_S_NOTIFICATION_NOT_DISPLAYED),
                       TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                       MB_ICONINFORMATION|MB_OK
                      );

            continue;
        }

        AddNotification(alert->alert_text_w, alert, data);

        SendMessage(tside_ide_window, TSIDE_MESSAGE_RUN_ALERT, 0, (LPARAM)alert);
    }

    if(run_module.compile_result != TSUTIL_ERROR_NONE)
    {
        if(run_module.compile_result == TSUTIL_ERROR_COMPILATION_WARNING)
        {
            int selection;

            AddNotification(
                            TSIDE_GetResourceText(TSIDE_S_VALIDATION_COMPLETE_WITH_WARNINGS),
                            NULL,
                            data
                           );

            selection = MessageBox(
                                   tside_run_window,
                                   TSIDE_GetResourceText(TSIDE_S_PROMPT_RUN_VALIDATION_WARNINGS),
                                   TSIDE_GetResourceText(TSIDE_S_VALIDATION_WARNINGS),
                                   MB_ICONINFORMATION|MB_YESNO
                                  );
            if(selection == IDNO)
                goto run_complete;
        }
        else
        {
            AddNotification(
                            TSIDE_GetResourceText(TSIDE_S_VALIDATION_COMPLETE_WITH_ERRORS),
                            NULL,
                            data
                           );

            MessageBox(
                       tside_run_window,
                       TSIDE_GetResourceText(TSIDE_S_CANNOT_RUN_VALIDATION_ERRORS),
                       TSIDE_GetResourceText(TSIDE_S_VALIDATION_ERRORS),
                       MB_ICONINFORMATION|MB_OK
                      );

            goto run_complete;
        }
    }

    AddNotification(
                    TSIDE_GetResourceText(TSIDE_S_VALIDATION_COMPLETE_EXECUTING),
                    NULL,
                    data
                   );

    TSInt_ClearAbort(abort_signal);

    run_module.execution_flags = 0;

    EnableWindow(data->stop_button, TRUE);

    created_thread = CreateThread(NULL, 0, &ExecuteThread, data, 0, NULL);
    if(created_thread == NULL)
        goto create_compile_thread_failed;

    ide_notification.hwndFrom = tside_run_window;
    ide_notification.idFrom   = 0;
    ide_notification.code     = TSIDE_NOTIFY_RUN_EXECUTING;

    SendMessage(tside_ide_window, WM_NOTIFY, 0, (LPARAM)&ide_notification);

    return;

create_compile_thread_failed:
    MessageBox(
               tside_run_window,
               TSIDE_GetResourceText(TSIDE_S_INITIALIZE_RUN_ERROR),
               TSIDE_GetResourceText(TSIDE_S_CANNOT_RUN),
               MB_ICONINFORMATION|MB_OK
              );
run_complete:
    ResetRunState(data);
}

static void ProcessExecuteResult (struct run_data* data)
{
    if(run_module.execution_flags&EXECUTION_FLAG_MANUAL_STOP)
    {
        AddNotification(
                        TSIDE_GetResourceText(TSIDE_S_EXECUTION_MANUALLY_STOPPED),
                        NULL,
                        data
                       );
    }
    else if(run_module.execution_result != TSINT_ERROR_NONE)
    {
        AddNotification(
                        TSIDE_GetResourceText(TSIDE_S_EXECUTION_COMPLETE_WITH_ERRORS),
                        NULL,
                        data
                       );

        MessageBox(
                   tside_run_window,
                   TSIDE_GetResourceText(TSIDE_S_EXECUTION_ERROR_ENCOUNTERED),
                   TSIDE_GetResourceText(TSIDE_S_EXECUTION_ERROR),
                   MB_ICONINFORMATION|MB_OK
                  );
    }
    else
    {
        AddNotification(
                        TSIDE_GetResourceText(TSIDE_S_EXECUTION_COMPLETE),
                        NULL,
                        data
                       );
    }

    ResetRunState(data);
}

static void ProcessExecutionAlert (struct tside_alert* alert, struct run_data* data)
{
    if(alert == NULL)
    {
        MessageBox(
                   tside_run_window,
                   TSIDE_GetResourceText(TSIDE_S_NOTIFICATION_NOT_DISPLAYED),
                   TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                   MB_ICONINFORMATION|MB_OK
                  );
    }
    else
    {
        AddNotification(alert->alert_text_w, alert, data);

        SendMessage(tside_ide_window, TSIDE_MESSAGE_RUN_ALERT, 0, (LPARAM)alert);
    }
}

static void ProcessExecutionBreak (struct run_data* data)
{
    run_module.execution_flags |= EXECUTION_FLAG_BREAK;

    SendMessage(
                tside_ide_window,
                TSIDE_MESSAGE_RUN_BREAK,
                run_module.current_location,
                (LPARAM)run_module.current_unit
               );
}

int TSIDE_InitializeRun (void)
{
    int error;

    current_run_module = NULL;

    error = TSInt_AllocAbortSignal(&abort_signal);
    if(error != TSINT_ERROR_NONE)
        goto allocate_abort_signal_failed;

    control_mode_signal = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(control_mode_signal == NULL)
        goto create_control_mode_signal_failed;

    return TSIDE_ERROR_NONE;

create_control_mode_signal_failed:
    TSInt_FreeAbortSignal(abort_signal);
allocate_abort_signal_failed:
    return TSIDE_ERROR_TSINT_FAILURE;
}

void TSIDE_ShutdownRun (void)
{
    CloseHandle(control_mode_signal);
    TSInt_FreeAbortSignal(abort_signal);
}

void TSIDE_SetRunData (WCHAR* unit, unsigned int controller_mode)
{
    WCHAR            invocation_expression[MAX_PATH+sizeof("()")+1];
    struct run_data* data;

    data = (struct run_data*)GetWindowLongPtr(tside_run_window, GWLP_USERDATA);
    if(data == NULL)
        return;

    wcscpy(invocation_expression, unit);
    wcscat(invocation_expression, L"()");

    SetWindowText(data->invocation_input_window, invocation_expression);

    if(controller_mode != TSINT_CONTROL_RUN)
        SendMessage(data->debug_checkbox, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendMessage(data->debug_checkbox, BM_SETCHECK, BST_UNCHECKED, 0);
}

void TSIDE_SetRunControllerMode (unsigned int mode)
{
    if(!(run_module.execution_flags&EXECUTION_FLAG_BREAK))
        return;

    run_module.execution_flags &= ~EXECUTION_FLAG_BREAK;
    run_module.controller_mode  = mode;

    if(mode == TSINT_CONTROL_HALT)
        run_module.execution_flags |= EXECUTION_FLAG_MANUAL_STOP;

    SetEvent(control_mode_signal);
}

void TSIDE_PrintVariables (void)
{
    struct tsdef_block* current_scope;
    struct run_data*    data;

    data = (struct run_data*)GetWindowLongPtr(tside_run_window, GWLP_USERDATA);
    if(data == NULL)
        return;

    for(
        current_scope = run_module.current_unit_state->current_block;
        current_scope != NULL;
        current_scope = current_scope->parent_block
       )
    {
        struct tsdef_variable* variables;

        for(
            variables = current_scope->variables;
            variables != NULL;
            variables = variables->next_variable
           )
        {
            char                   number_value[MAX_NUMBER_LENGTH];
            struct tsint_variable* variable_data;
            char*                  variable_type;
            char*                  variable_name;
            char*                  variable_value;
            char*                  notification_text;
            WCHAR*                 notification_text_w;
            size_t                 print_length;

            variable_data = TSInt_LookupVariableAddress(variables, run_module.current_unit_state);

            if(!(variable_data->flags&TSINT_VARIABLE_FLAG_INITIALIZED))
                continue;

            variable_name = variables->name;

            switch(variables->primitive_type)
            {
            case TSDEF_PRIMITIVE_TYPE_BOOL:
                variable_type = "bool";

                if(variable_data->value.bool_data == TSDEF_BOOL_FALSE)
                    variable_value = "false";
                else
                    variable_value = "true";

                break;

            case TSDEF_PRIMITIVE_TYPE_INT:
                sprintf(number_value, "%d", variable_data->value.int_data);

                variable_type  = "integer";
                variable_value = number_value;

                break;

            case TSDEF_PRIMITIVE_TYPE_REAL:
                sprintf(number_value, "%f", variable_data->value.real_data);

                variable_type  = "real";
                variable_value = number_value;

                break;

            case TSDEF_PRIMITIVE_TYPE_STRING:
                variable_type  = "string";
                variable_value = variable_data->value.string_data;

                break;
            }

            print_length = strlen(variable_name)+strlen(variable_type)+strlen(variable_value)+sizeof("  = ")+1;

            notification_text_w = malloc(print_length*sizeof(WCHAR)+print_length);
            if(notification_text_w == NULL)
            {
                MessageBox(
                           tside_run_window,
                           TSIDE_GetResourceText(TSIDE_S_CANNOT_PRINT_VARIABLE),
                           TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                           MB_ICONINFORMATION|MB_OK
                          );

                continue;
            }

            notification_text = (char*)&notification_text_w[print_length];

            sprintf(notification_text, "%s %s = %s", variable_type, variable_name, variable_value);
            mbstowcs(notification_text_w, notification_text, print_length);

            AddNotification(notification_text_w, NULL, data);

            free(notification_text_w);
        }
    }
}

void TSIDE_AbortRun (void)
{
    if(current_run_module == NULL)
        return;

    run_module.execution_flags |= EXECUTION_FLAG_MANUAL_STOP;

    TSInt_SignalAbort(abort_signal);
}

INT_PTR CALLBACK TSIDE_RunMessageProc (
                                       HWND   window,
                                       UINT   message,
                                       WPARAM wparam,
                                       LPARAM lparam
                                      )
{
    NMHDR*           notify_data;
    NMITEMACTIVATE*  item;
    struct run_data* data;
    int              error;
    WORD             command;

    switch(message)
    {
    case WM_INITDIALOG:
        error = InitializeDialog(window);
        if(error != TSIDE_ERROR_NONE)
            goto initialize_dialog_failed;

        return TRUE;

    case WM_CLOSE:
        CloseDialog();

        return TRUE;

    case WM_DESTROY:
        data = (struct run_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_COMMAND:
        data = (struct run_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        command = LOWORD(wparam);

        switch(command)
        {
        case TSIDE_C_RUN_COMPILE_AND_RUN:
            BeginCompile(data);

            return TRUE;

        case TSIDE_C_RUN_STOP:
            SendMessage(
                        tside_ide_window,
                        WM_COMMAND,
                        MAKEWPARAM(TSIDE_M_IDE_RUN_STOP, HIWORD(wparam)),
                        (LPARAM)tside_run_window
                       );

            return TRUE;

        case TSIDE_C_RUN_CLEAR_NOTIFICATIONS:
            ClearNotifications(data);

            return TRUE;
        }

        return FALSE;

    case WM_NOTIFY:
        data = (struct run_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notify_data = (NMHDR*)lparam;

        switch(notify_data->code)
        {
        case NM_DBLCLK:
            if(notify_data->hwndFrom != data->notification_window)
                return FALSE;

            item = (NMITEMACTIVATE*)lparam;

            GotoNotification(item->iItem, data);

            return TRUE;
        }

        return FALSE;

    case MESSAGE_COMPILATION_COMPLETE:
        data = (struct run_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        ProcessCompileResult(data);

        return TRUE;

    case MESSAGE_EXECUTION_COMPLETE:
        data = (struct run_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        ProcessExecuteResult(data);

        return TRUE;

    case MESSAGE_EXECUTION_ALERT:
        data = (struct run_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        ProcessExecutionAlert((struct tside_alert*)lparam, data);

        return TRUE;

    case MESSAGE_EXECUTION_BREAK:
        data = (struct run_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        ProcessExecutionBreak(data);

        return TRUE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_RUN_ERROR);

    return FALSE;
}

