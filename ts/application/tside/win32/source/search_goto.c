/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "search_goto.h"
#include "text.h"
#include "resources.h"
#include "main.h"
#include "error.h"

#include <stdlib.h>
#include <malloc.h>


#define MAX_LINE_TEXT_LENGTH 100


struct search_goto_data
{
    HWND parent_window;
    HWND search_goto_window;
    HWND line_input_window;
};


static int  InitializeDialog   (HWND);
static void DestroyDialog      (struct search_goto_data*);
static void RepositionControls (struct search_goto_data*);


static int InitializeDialog (HWND window)
{
    struct search_goto_data* data;

    data = malloc(sizeof(struct search_goto_data));
    if(data == NULL)
        goto allocate_search_goto_data_failed;

    data->parent_window        = GetParent(window);
    data->search_goto_window   = window;
    data->line_input_window    = GetDlgItem(window, TSIDE_C_SEARCH_GOTO_LINE_INPUT);

    SendMessage(data->line_input_window, EM_LIMITTEXT, MAX_LINE_TEXT_LENGTH-1, 0);

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    return TSIDE_ERROR_NONE;

allocate_search_goto_data_failed:
    return TSIDE_ERROR_SEARCH_TEXT_ERROR;
}

static void DestroyDialog (struct search_goto_data* data)
{
    free(data);
}

static void RepositionControls (struct search_goto_data* data)
{
}


int TSIDE_InitializeSearchGoto (void)
{
    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownSearchGoto (void)
{
}

int TSIDE_GetGotoLineNumber (HWND window)
{
    char                     line_text[MAX_LINE_TEXT_LENGTH];
    struct search_goto_data* data;
    int                      line_number;

    data = (struct search_goto_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return 0;

    GetWindowTextA(data->line_input_window, line_text, MAX_LINE_TEXT_LENGTH);

    line_number = atoi(line_text);

    return line_number-1;
}

INT_PTR CALLBACK TSIDE_SearchGotoMessageProc (
                                              HWND   window,
                                              UINT   message,
                                              WPARAM wparam,
                                              LPARAM lparam
                                             )
{
    struct search_goto_data* data;
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
        data = (struct search_goto_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_SIZE:
        data = (struct search_goto_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        RepositionControls(data);

        return TRUE;

    case WM_SHOWWINDOW:
        data = (struct search_goto_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        if(wparam == TRUE)
            SetFocus(data->line_input_window);

        return TRUE;

    case WM_COMMAND:
        data = (struct search_goto_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL || data->parent_window == NULL)
            return FALSE;

        command = LOWORD(wparam);

        switch(command)
        {
        case TSIDE_C_SEARCH_GOTO_PERFORM_GOTO:
            SendMessage(
                        data->parent_window,
                        WM_COMMAND,
                        MAKEWPARAM(TSIDE_M_IDE_SEARCH_PERFORM_GOTO, HIWORD(wparam)),
                        (LPARAM)data->search_goto_window
                       );

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_SEARCH_GOTO_ERROR);

    return FALSE;
}

