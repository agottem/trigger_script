/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "search_text.h"
#include "text.h"
#include "resources.h"
#include "main.h"
#include "error.h"

#include <malloc.h>


struct search_text_data
{
    HWND parent_window;
    HWND search_text_window;
    HWND find_input_window;
    HWND replace_input_window;
};


static int  InitializeDialog   (HWND);
static void DestroyDialog      (struct search_text_data*);
static void RepositionControls (struct search_text_data*);


static int InitializeDialog (HWND window)
{
    struct search_text_data* data;

    data = malloc(sizeof(struct search_text_data));
    if(data == NULL)
        goto allocate_search_text_data_failed;

    data->parent_window        = GetParent(window);
    data->search_text_window   = window;
    data->find_input_window    = GetDlgItem(window, TSIDE_C_SEARCH_TEXT_FIND_INPUT);
    data->replace_input_window = GetDlgItem(window, TSIDE_C_SEARCH_TEXT_REPLACE_INPUT);

    SendMessage(data->find_input_window, EM_LIMITTEXT, TSIDE_MAX_SEARCH_LENGTH-1, 0);
    SendMessage(data->replace_input_window, EM_LIMITTEXT, TSIDE_MAX_SEARCH_LENGTH-1, 0);

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    return TSIDE_ERROR_NONE;

allocate_search_text_data_failed:
    return TSIDE_ERROR_SEARCH_TEXT_ERROR;
}

static void DestroyDialog (struct search_text_data* data)
{
    free(data);
}

static void RepositionControls (struct search_text_data* data)
{
}


int TSIDE_InitializeSearchText (void)
{
    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownSearchText (void)
{
}

void TSIDE_GetFindText (HWND window, char* find_text)
{
    struct search_text_data* data;

    data = (struct search_text_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
    {
        find_text[0] = 0;

        return;
    }

    GetWindowTextA(data->find_input_window, find_text, TSIDE_MAX_SEARCH_LENGTH);
}

void TSIDE_GetReplaceText (HWND window, char* replace_text)
{
    struct search_text_data* data;

    data = (struct search_text_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
    {
        replace_text[0] = 0;

        return;
    }

    GetWindowTextA(data->replace_input_window, replace_text, TSIDE_MAX_SEARCH_LENGTH);
}

INT_PTR CALLBACK TSIDE_SearchTextMessageProc (
                                              HWND   window,
                                              UINT   message,
                                              WPARAM wparam,
                                              LPARAM lparam
                                             )
{
    struct search_text_data* data;
    int                      error;
    WORD                     command;

    switch(message)
    {
    case WM_INITDIALOG:
        error = InitializeDialog(window);
        if(error != TSIDE_ERROR_NONE)
            goto initialize_dialog_failed;

        return TRUE;

    case WM_CLOSE:
        return TRUE;

    case WM_DESTROY:
        data = (struct search_text_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_SIZE:
        data = (struct search_text_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        RepositionControls(data);

        return TRUE;

    case WM_SHOWWINDOW:
        data = (struct search_text_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        if(wparam == TRUE)
            SetFocus(data->find_input_window);

        return TRUE;

    case WM_COMMAND:
        data = (struct search_text_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL || data->parent_window == NULL)
            return FALSE;

        command = LOWORD(wparam);

        switch(command)
        {
        case TSIDE_C_SEARCH_TEXT_FIND_NEXT:
            SendMessage(
                        data->parent_window,
                        WM_COMMAND,
                        MAKEWPARAM(TSIDE_M_IDE_SEARCH_FIND_NEXT, HIWORD(wparam)),
                        (LPARAM)data->search_text_window
                       );

            return TRUE;

        case TSIDE_C_SEARCH_TEXT_FIND_PREV:
            SendMessage(
                        data->parent_window,
                        WM_COMMAND,
                        MAKEWPARAM(TSIDE_M_IDE_SEARCH_FIND_PREVIOUS, HIWORD(wparam)),
                        (LPARAM)data->search_text_window
                       );

            return TRUE;

        case TSIDE_C_SEARCH_TEXT_REPLACE_NEXT:
            SendMessage(
                        data->parent_window,
                        WM_COMMAND,
                        MAKEWPARAM(TSIDE_M_IDE_SEARCH_REPLACE_NEXT, HIWORD(wparam)),
                        (LPARAM)data->search_text_window
                       );

            return TRUE;

        case TSIDE_C_SEARCH_TEXT_REPLACE_ALL:
            SendMessage(
                        data->parent_window,
                        WM_COMMAND,
                        MAKEWPARAM(TSIDE_M_IDE_SEARCH_REPLACE_ALL, HIWORD(wparam)),
                        (LPARAM)data->search_text_window
                       );

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_SEARCH_TEXT_ERROR);

    return FALSE;
}

