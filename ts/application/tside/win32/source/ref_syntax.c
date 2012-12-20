/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "ref_syntax.h"
#include "gui.h"
#include "ide.h"
#include "text.h"
#include "file.h"
#include "main.h"
#include "resources.h"
#include "scintilla.h"
#include "paths.h"
#include "error.h"

#include <malloc.h>
#include <string.h>
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>


#define MAX_CATEGORY_NAME_LENGTH 1024


struct ref_syntax_data
{
    HWND ref_syntax_window;
    HWND syntax_window;
    HWND content_window;

    struct tside_scintilla_configuration content_configuration;
};


static int  FindSyntax         (void);
static void DestroyFoundSyntax (void);

static int  InitializeDialog (HWND);
static void DestroyDialog    (struct ref_syntax_data*);
static void PopulateControls (struct ref_syntax_data*);

static void SelectSyntax (int, struct ref_syntax_data*);


struct tside_syntax_data* tside_found_syntax;


static int FindSyntax (void)
{
    WIN32_FIND_DATA find_data;
    WCHAR*          search_path;
    WCHAR*          syntax_path;
    char*           file_content;
    HANDLE          find_handle;
    size_t          path_length;
    size_t          alloc_size;

    tside_found_syntax = NULL;

    path_length = wcslen(tside_syntax_path_w);

    alloc_size  = path_length+sizeof(L"\\*.ts")+1;
    search_path = malloc(sizeof(WCHAR)*alloc_size);
    if(search_path == NULL)
        goto alloc_search_path_failed;

    wcscpy(search_path, tside_syntax_path_w);
    wcscat(search_path, L"\\*.ts");

    find_handle = FindFirstFile(search_path, &find_data);
    if(find_handle == NULL)
        goto no_files_found;

    do
    {
        WCHAR                     syntax_name[MAX_PATH];
        WCHAR*                    replace_position;
        struct tside_syntax_data* syntax;
        size_t                    divided_length;
        int                       error;

        replace_position = wcsrchr(find_data.cFileName, '.');
        if(replace_position == NULL || replace_position == find_data.cFileName)
            continue;

        divided_length = replace_position-find_data.cFileName;

        wcsncpy(syntax_name, find_data.cFileName, divided_length);

        syntax_name[divided_length] = 0;
        replace_position            = syntax_name;

        while(*replace_position != 0)
        {
            if(*replace_position == '_')
                *replace_position = ' ';

            replace_position++;
        }

        alloc_size  = path_length+wcslen(find_data.cFileName)+sizeof("\\")+1;
        syntax_path = malloc(alloc_size*sizeof(WCHAR));
        if(syntax_path == NULL)
            goto allocate_syntax_path_failed;

        wcscpy(syntax_path, tside_syntax_path_w);
        wcscat(syntax_path, L"\\");
        wcscat(syntax_path, find_data.cFileName);

        error = TSIDE_GetFileText(syntax_path, &file_content);
        if(error != TSIDE_ERROR_NONE)
            goto get_file_text_failed;

        syntax = malloc(sizeof(struct tside_syntax_data));
        if(syntax == NULL)
            goto allocate_syntax_failed;

        free(syntax_path);

        syntax->content = file_content;

        wcscpy(syntax->syntax_name, syntax_name);

        syntax->next_syntax = tside_found_syntax;
        tside_found_syntax  = syntax;
    }while(FindNextFile(find_handle, &find_data) != 0);

    FindClose(find_handle);

no_files_found:
    free(search_path);

    return TSIDE_ERROR_NONE;

allocate_syntax_failed:
    free(file_content);
get_file_text_failed:
    free(syntax_path);
allocate_syntax_path_failed:
    free(search_path);

alloc_search_path_failed:
    return TSIDE_ERROR_SYSTEM_CALL_FAILED;
}

static void DestroyFoundSyntax (void)
{
    struct tside_syntax_data* syntax;

    syntax = tside_found_syntax;
    while(syntax != NULL)
    {
        struct tside_syntax_data* free_syntax;

        free_syntax = syntax;
        syntax      = syntax->next_syntax;

        free(free_syntax->content);
        free(free_syntax);
    }
}

static int InitializeDialog (HWND window)
{
    struct ref_syntax_data* data;
    HWND                    scintilla_window;
    int                     error;

    data = malloc(sizeof(struct ref_syntax_data));
    if(data == NULL)
        goto allocate_ref_syntax_data_failed;

    EnableThemeDialogTexture(window, ETDT_USETABTEXTURE|ETDT_ENABLE);

    scintilla_window = GetDlgItem(window, TSIDE_C_REF_SYNTAX_CONTENT);

    data->ref_syntax_window     = window;
    data->syntax_window         = GetDlgItem(window, TSIDE_C_REF_SYNTAX_SYNTAX);
    data->content_window        = scintilla_window;
    data->content_configuration = tside_default_configuration;

    data->content_configuration.flags &= ~TSIDE_SCINTILLA_CONFIG_SHOW_LINE_NUMBERS;

    error = TSIDE_ConfigureScintilla(&data->content_configuration, scintilla_window);
    if(error != TSIDE_ERROR_NONE)
        goto configure_scintilla_failed;

    TSIDE_SetScintillaReadOnly(TSIDE_READONLY, scintilla_window);

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    PopulateControls(data);
    SelectSyntax(0, data);

    return TSIDE_ERROR_NONE;

configure_scintilla_failed:
    free(data);

allocate_ref_syntax_data_failed:
    return TSIDE_ERROR_REFERENCE_ERROR;
}

static void DestroyDialog (struct ref_syntax_data* data)
{
    free(data);
}

static void PopulateControls (struct ref_syntax_data* data)
{
    struct tside_syntax_data* syntax;
    HWND                      syntax_window;

    syntax_window = data->syntax_window;

    for(syntax = tside_found_syntax; syntax != NULL; syntax = syntax->next_syntax)
    {
        WCHAR*  syntax_name;
        LRESULT existing_syntax;

        syntax_name = syntax->syntax_name;

        existing_syntax = SendMessage(
                                      syntax_window,
                                      LB_FINDSTRING,
                                      (WPARAM)-1,
                                      (LPARAM)syntax_name
                                     );
        if(existing_syntax != LB_ERR)
            continue;

        existing_syntax = SendMessage(
                                      syntax_window,
                                      LB_ADDSTRING,
                                      0,
                                      (LPARAM)syntax_name
                                     );
        if(existing_syntax == LB_ERR || existing_syntax == LB_ERRSPACE)
            goto add_syntax_failed;

        SendMessage(syntax_window, LB_SETITEMDATA, existing_syntax, (LPARAM)syntax);
    }

    return;


add_syntax_failed:
    MessageBox(
               tside_reference_window,
               TSIDE_GetResourceText(TSIDE_S_CANNOT_POPULATE_REFERENCE_DATA),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

static void SelectSyntax (int syntax_index, struct ref_syntax_data* data)
{
    struct tside_syntax_data* syntax;
    HWND                      syntax_window;
    HWND                      content_window;
    LRESULT                   item_data;

    syntax_window = data->syntax_window;

    SendMessage(syntax_window, LB_SETCURSEL, syntax_index, 0);

    item_data = SendMessage(syntax_window, LB_GETITEMDATA, syntax_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    syntax = (struct tside_syntax_data*)item_data;

    content_window = data->content_window;

    TSIDE_SetScintillaReadOnly(TSIDE_WRITABLE, content_window);

    TSIDE_SetScintillaText(syntax->content, content_window);

    TSIDE_SetScintillaReadOnly(TSIDE_READONLY, content_window);
}


int TSIDE_InitializeRefSyntax (void)
{
    int error;

    error = FindSyntax();
    if(error != TSIDE_ERROR_NONE)
        goto find_syntax_failed;

    return TSIDE_ERROR_NONE;

find_syntax_failed:
    DestroyFoundSyntax();

    return error;
}

void TSIDE_ShutdownRefSyntax (void)
{
    DestroyFoundSyntax();
}

INT_PTR CALLBACK TSIDE_RefSyntaxMessageProc (
                                             HWND   window,
                                             UINT   message,
                                             WPARAM wparam,
                                             LPARAM lparam
                                            )
{
    struct ref_syntax_data* data;
    HWND                    origin_window;
    LRESULT                 item_index;
    int                     error;
    WORD                    notification_code;

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
        data = (struct ref_syntax_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_COMMAND:
        data = (struct ref_syntax_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notification_code = HIWORD(wparam);

        switch(notification_code)
        {
        case LBN_SELCHANGE:
            origin_window = (HWND)lparam;

            item_index = SendMessage(origin_window, LB_GETCURSEL, 0, 0);
            if(item_index == LB_ERR)
                return FALSE;

            SelectSyntax(item_index, data);

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_REF_SYNTAX_ERROR);

    return FALSE;
}

