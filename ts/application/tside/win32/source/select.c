/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "select.h"
#include "ide.h"
#include "gui.h"
#include "text.h"
#include "main.h"
#include "ref_templates.h"
#include "resources.h"
#include "error.h"

#include <malloc.h>
#include <string.h>
#include <commctrl.h>


struct select_data
{
    HWND select_window;
    HWND template_window;
};


static int  InitializeDialog (HWND);
static void DestroyDialog    (struct select_data*);
static void PopulateControls (struct select_data*);

static void SelectTemplate (int, struct select_data*);


static int InitializeDialog (HWND window)
{
    struct select_data* data;

    data = malloc(sizeof(struct select_data));
    if(data == NULL)
        goto allocate_select_data_failed;

    data->select_window   = window;
    data->template_window = GetDlgItem(window, TSIDE_C_SELECT_TEMPLATES);

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    PopulateControls(data);
    SendMessage(data->template_window, LB_SETCURSEL, 0, 0);

    return TSIDE_ERROR_NONE;

allocate_select_data_failed:
    return TSIDE_ERROR_REFERENCE_ERROR;
}

static void DestroyDialog (struct select_data* data)
{
    free(data);
}

static void PopulateControls (struct select_data* data)
{
    struct tside_template_data* template;
    HWND                        template_window;
    LRESULT                     existing_template;

    template_window = data->template_window;

    for(template = tside_found_templates; template != NULL; template = template->next_template)
    {
        WCHAR template_name[MAX_PATH*2+sizeof(" - ")];

        wcscpy(template_name, template->category_name);
        wcscat(template_name, L" - ");
        wcscat(template_name, template->template_name);

        existing_template = SendMessage(
                                        template_window,
                                        LB_FINDSTRING,
                                        (WPARAM)-1,
                                        (LPARAM)template_name
                                       );
        if(existing_template != LB_ERR)
            continue;

        existing_template = SendMessage(
                                        template_window,
                                        LB_ADDSTRING,
                                        0,
                                        (LPARAM)template_name
                                       );
        if(existing_template == LB_ERR || existing_template == LB_ERRSPACE)
            goto add_template_failed;

        SendMessage(template_window, LB_SETITEMDATA, existing_template, (LPARAM)template);
    }

    existing_template = SendMessage(
                                    template_window,
                                    LB_ADDSTRING,
                                    0,
                                    (LPARAM)TSIDE_GetResourceText(TSIDE_S_BLANK_TEMPLATE)
                                   );
    if(existing_template == LB_ERR || existing_template == LB_ERRSPACE)
        goto add_template_failed;

    SendMessage(template_window, LB_SETITEMDATA, existing_template, 0);

    return;

add_template_failed:
    MessageBox(
               tside_ide_window,
               TSIDE_GetResourceText(TSIDE_S_CANNOT_POPULATE_REFERENCE_DATA),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

static void SelectTemplate (int template_index, struct select_data* data)
{
    struct tside_template_data* template;
    HWND                        template_window;
    LRESULT                     item_data;

    template_window = data->template_window;

    item_data = SendMessage(template_window, LB_GETITEMDATA, template_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    template = (struct tside_template_data*)item_data;

    SendMessage(
                tside_ide_window,
                TSIDE_MESSAGE_USE_TEMPLATE,
                0,
                (LPARAM)template->content
               );
}


int TSIDE_InitializeSelect (void)
{
    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownSelect (void)
{
}

INT_PTR CALLBACK TSIDE_SelectMessageProc (
                                          HWND   window,
                                          UINT   message,
                                          WPARAM wparam,
                                          LPARAM lparam
                                         )
{
    struct select_data* data;
    HWND                origin_window;
    LRESULT             item_index;
    int                 error;
    WORD                notification_code;
    WORD                command;

    switch(message)
    {
    case WM_INITDIALOG:
        error = InitializeDialog(window);
        if(error != TSIDE_ERROR_NONE)
            goto initialize_dialog_failed;

        return TRUE;

    case WM_CLOSE:
        data = (struct select_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        EndDialog(data->select_window, TSIDE_ERROR_NONE);

        return TRUE;

    case WM_DESTROY:
        data = (struct select_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_COMMAND:
        data = (struct select_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notification_code = HIWORD(wparam);

        switch(notification_code)
        {
        case LBN_DBLCLK:
            origin_window = (HWND)lparam;

            item_index = SendMessage(origin_window, LB_GETCURSEL, 0, 0);
            if(item_index == LB_ERR)
                return FALSE;

            else if(origin_window == data->template_window)
            {
                SelectTemplate(item_index, data);
                EndDialog(data->select_window, TSIDE_ERROR_NONE);
            }

            return TRUE;
        }

        command = LOWORD(wparam);

        switch(command)
        {
        case TSIDE_C_SELECT_USE_TEMPLATE:

            item_index = SendMessage(data->template_window, LB_GETCURSEL, 0, 0);
            if(item_index == LB_ERR)
                return FALSE;

            SelectTemplate(item_index, data);
            EndDialog(data->select_window, TSIDE_ERROR_NONE);

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_SELECT_ERROR);

    return FALSE;
}

