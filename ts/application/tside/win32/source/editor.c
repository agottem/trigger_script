/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "editor.h"
#include "search_text.h"
#include "search_goto.h"
#include "scintilla.h"
#include "file.h"
#include "text.h"
#include "resources.h"
#include "paths.h"
#include "main.h"
#include "ide.h"
#include "error.h"

#include <malloc.h>
#include <string.h>
#include <commctrl.h>
#include <commdlg.h>
#include <scintilla/Scintilla.h>


#define CONTINUE_OPERATION  0
#define ABORT_OPERATION    -1

#define SEARCH_STATE_HIDDEN 0
#define SEARCH_STATE_TEXT   1
#define SEARCH_STATE_GOTO   2

#define EDITOR_FLAG_READONLY 0x01


struct file_tab_data
{
    WCHAR file_name[MAX_PATH];

    HWND scintilla_window;

    struct file_tab_data* next_tab;
};

struct editor_data
{
    HWND parent_window;
    HWND editor_window;
    HWND file_tabs_window;
    HWND search_text_window;
    HWND search_goto_window;

    unsigned int search_state;

    struct tside_scintilla_configuration configuration;

    int                   total_tabs;
    int                   current_tab_index;
    struct file_tab_data* current_tab;

    struct file_tab_data* file_tabs;

    unsigned int editor_flags;
};


static int  InitializeDialog   (HWND);
static void DestroyDialog      (struct editor_data*);
static void RepositionControls (struct editor_data*);
static void SetSearchState     (unsigned int, struct editor_data*);

static WCHAR* MakeTabName (WCHAR*);

static int                   AddFileTab            (WCHAR*, char*, struct editor_data*);
static void                  RemoveFileTab         (int, struct editor_data*);
static int                   FindFileTab           (WCHAR*, struct editor_data*);
static int                   DetermineFileTabIndex (struct file_tab_data*, struct editor_data*);
static struct file_tab_data* GetFileTabData        (int, struct editor_data*);
static void                  SetFileTable          (int, struct editor_data*);
static void                  SwitchFileTab         (int, struct editor_data*);

static unsigned int HasUserTabs (struct editor_data*);

static int                   OpenNewFile      (struct editor_data*);
static int                   OpenExistingFile (struct editor_data*);
static struct file_tab_data* VerifyFileOpen   (WCHAR*, struct editor_data*, int*);
static int                   SaveFile         (struct file_tab_data*, struct editor_data*);
static int                   SaveFileAs       (struct file_tab_data*, struct editor_data*);
static int                   CloseCurrentFile (struct editor_data*);

static int ValidateFileSaved (struct file_tab_data*, struct editor_data*);


static int InitializeDialog (HWND window)
{
    struct editor_data* data;
    HWND                search_text_window;
    HWND                search_goto_window;
    int                 error;

    data = malloc(sizeof(struct editor_data));
    if(data == NULL)
        goto allocate_editor_data_failed;

    search_text_window = CreateDialog(
                                      tside_application_instance,
                                      MAKEINTRESOURCE(TSIDE_D_SEARCH_TEXT),
                                      window,
                                      &TSIDE_SearchTextMessageProc
                                     );
    if(search_text_window == NULL)
        goto create_search_text_window_failed;

    search_goto_window = CreateDialog(
                                      tside_application_instance,
                                      MAKEINTRESOURCE(TSIDE_D_SEARCH_GOTO),
                                      window,
                                      &TSIDE_SearchGotoMessageProc
                                     );
    if(search_goto_window == NULL)
        goto create_search_goto_window_failed;

    data->parent_window      = GetParent(window);
    data->editor_window      = window;
    data->file_tabs_window   = GetDlgItem(window, TSIDE_C_EDITOR_FILE_TABS);
    data->search_text_window = search_text_window;
    data->search_goto_window = search_goto_window;
    data->search_state       = SEARCH_STATE_HIDDEN;
    data->configuration      = tside_default_configuration;
    data->total_tabs         = 0;
    data->current_tab_index  = -1;
    data->current_tab        = NULL;
    data->file_tabs          = NULL;
    data->editor_flags       = 0;

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    error = OpenNewFile(data);
    if(error != TSIDE_ERROR_NONE)
        goto open_new_file_failed;

    RepositionControls(data);

    return TSIDE_ERROR_NONE;

open_new_file_failed:
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)NULL);
    DestroyWindow(search_goto_window);
create_search_goto_window_failed:
    DestroyWindow(search_text_window);
create_search_text_window_failed:
    free(data);

allocate_editor_data_failed:
    return TSIDE_ERROR_EDITOR_ERROR;
}

static void DestroyDialog (struct editor_data* data)
{
    struct file_tab_data* tab_data;

    tab_data = data->current_tab;
    while(tab_data != NULL)
    {
        struct file_tab_data* free_tab_data;

        free_tab_data = tab_data;
        tab_data      = tab_data->next_tab;

        DestroyWindow(free_tab_data->scintilla_window);
        free(free_tab_data);
    }

    free(data);
}

static void RepositionControls (struct editor_data* data)
{
    RECT editor_client_area;
    RECT search_client_area;
    RECT file_tabs_client_area;
    RECT file_tabs_control_area;
    RECT scintilla_client_area;

    GetClientRect(data->editor_window, &editor_client_area);

    file_tabs_client_area = editor_client_area;

    switch(data->search_state)
    {
    case SEARCH_STATE_TEXT:
        GetClientRect(data->search_text_window, &search_client_area);

        file_tabs_client_area.bottom -= search_client_area.bottom;

        search_client_area.left   = editor_client_area.left;
        search_client_area.top    = file_tabs_client_area.bottom+1;
        search_client_area.right  = editor_client_area.right;
        search_client_area.bottom = search_client_area.top+search_client_area.bottom;

        SetWindowPos(
                     data->search_text_window,
                     NULL,
                     search_client_area.left,
                     search_client_area.top,
                     search_client_area.right-search_client_area.left,
                     search_client_area.bottom-search_client_area.top,
                     SWP_NOZORDER
                    );

        break;

    case SEARCH_STATE_GOTO:
        GetClientRect(data->search_goto_window, &search_client_area);

        file_tabs_client_area.bottom -= search_client_area.bottom;

        search_client_area.left   = editor_client_area.left;
        search_client_area.top    = file_tabs_client_area.bottom+1;
        search_client_area.right  = editor_client_area.right;
        search_client_area.bottom = search_client_area.top+search_client_area.bottom;

        SetWindowPos(
                     data->search_goto_window,
                     NULL,
                     search_client_area.left,
                     search_client_area.top,
                     search_client_area.right-search_client_area.left,
                     search_client_area.bottom-search_client_area.top,
                     SWP_NOZORDER
                    );

        break;
    }

    SetWindowPos(
                 data->file_tabs_window,
                 NULL,
                 file_tabs_client_area.left,
                 file_tabs_client_area.top,
                 file_tabs_client_area.right-file_tabs_client_area.left,
                 file_tabs_client_area.bottom-file_tabs_client_area.top,
                 SWP_NOZORDER
                );

    file_tabs_control_area = file_tabs_client_area;

    SendMessage(data->file_tabs_window, TCM_ADJUSTRECT, FALSE, (LPARAM)&file_tabs_control_area);

    if(data->current_tab != NULL)
    {
        scintilla_client_area.left   = file_tabs_control_area.left;
        scintilla_client_area.top    = file_tabs_control_area.top;
        scintilla_client_area.right  = file_tabs_control_area.right;
        scintilla_client_area.bottom = file_tabs_control_area.bottom;

        SetWindowPos(
                     data->current_tab->scintilla_window,
                     HWND_TOP,
                     scintilla_client_area.left,
                     scintilla_client_area.top,
                     scintilla_client_area.right-scintilla_client_area.left,
                     scintilla_client_area.bottom-scintilla_client_area.top,
                     0
                    );
    }
}

static void SetSearchState (unsigned int search_state, struct editor_data* data)
{
    data->search_state = search_state;

    switch(search_state)
    {
    case SEARCH_STATE_HIDDEN:
        ShowWindow(data->search_text_window, SW_HIDE);
        ShowWindow(data->search_goto_window, SW_HIDE);

        if(data->current_tab != NULL)
            SetFocus(data->current_tab->scintilla_window);

        break;

    case SEARCH_STATE_TEXT:
        ShowWindow(data->search_goto_window, SW_HIDE);
        ShowWindow(data->search_text_window, SW_SHOW);

        break;

    case SEARCH_STATE_GOTO:
        ShowWindow(data->search_text_window, SW_HIDE);
        ShowWindow(data->search_goto_window, SW_SHOW);

        break;
    }
}

static WCHAR* MakeTabName (WCHAR* tab_name)
{
    WCHAR* name_start;
    WCHAR* separator;

    name_start = tab_name;

    separator = wcsrchr(tab_name, '\\');
    if(separator != NULL)
    {
        separator++;

        name_start = separator;

        separator = wcsrchr(tab_name, '.');
        if(separator != NULL)
            *separator = 0;
    }

    return name_start;
}

static int AddFileTab (WCHAR* file_name, char* file_content, struct editor_data* ed_data)
{
    WCHAR                 tab_name[MAX_PATH];
    TCITEM                gui_tab_item;
    HWND                  scintilla_window;
    struct file_tab_data* tab_data;
    int                   error;

    tab_data = malloc(sizeof(struct file_tab_data));
    if(tab_data == NULL)
        goto alloc_tab_data_failed;

    scintilla_window = CreateWindowEx(
                                      0,
                                      L"Scintilla",
                                      NULL,
                                      WS_CHILD|WS_TABSTOP|WS_BORDER,
                                      0,
                                      0,
                                      0,
                                      0,
                                      ed_data->editor_window,
                                      NULL,
                                      tside_application_instance,
                                      NULL
                                     );
    if(scintilla_window == NULL)
        goto create_scintilla_window_failed;

    ShowWindow(scintilla_window, SW_HIDE);

    error = TSIDE_ConfigureScintilla(&ed_data->configuration, scintilla_window);
    if(error != TSIDE_ERROR_NONE)
        goto configure_scintilla_failed;

    TSIDE_SetScintillaText(file_content, scintilla_window);

    if(ed_data->editor_flags&EDITOR_FLAG_READONLY)
        TSIDE_SetScintillaReadOnly(TSIDE_READONLY, scintilla_window);

    tab_data->scintilla_window = scintilla_window;

    if(file_name != NULL)
        wcscpy(tab_data->file_name, file_name);
    else
        tab_data->file_name[0] = 0;

    tab_data->next_tab = ed_data->file_tabs;
    ed_data->file_tabs = tab_data;

    gui_tab_item.mask = TCIF_TEXT;

    if(file_name == NULL)
        gui_tab_item.pszText = TSIDE_GetResourceText(TSIDE_S_NEW_FILE_TAB_NAME);
    else
    {
        wcscpy(tab_name, file_name);

        gui_tab_item.pszText = MakeTabName(tab_name);
    }

    gui_tab_item.lParam = 0;

    if(ed_data->current_tab != NULL)
        ed_data->current_tab_index++;

    ed_data->total_tabs++;

    SendMessage(ed_data->file_tabs_window, TCM_INSERTITEM, 0, (LPARAM)&gui_tab_item);

    return TSIDE_ERROR_NONE;

configure_scintilla_failed:
    DestroyWindow(scintilla_window);
create_scintilla_window_failed:
    free(tab_data);

alloc_tab_data_failed:
    return TSIDE_ERROR_MEMORY;
}

static void RemoveFileTab (int index, struct editor_data* ed_data)
{
    struct file_tab_data* tab_data;
    struct file_tab_data* previous_tab;
    int                   original_index;

    original_index = index;
    previous_tab   = NULL;
    tab_data       = ed_data->file_tabs;
    while(index--)
    {
        if(tab_data == NULL)
            return;

        previous_tab = tab_data;
        tab_data     = tab_data->next_tab;
    }

    if(tab_data == NULL)
        return;

    if(previous_tab == NULL)
        ed_data->file_tabs = tab_data->next_tab;
    else
        previous_tab->next_tab = tab_data->next_tab;

    if(original_index == ed_data->current_tab_index)
    {
        ed_data->current_tab       = NULL;
        ed_data->current_tab_index = -1;
    }
    else
    {
        if(original_index < ed_data->current_tab_index)
            ed_data->current_tab_index--;
    }

    ed_data->total_tabs--;

    SendMessage(ed_data->file_tabs_window, TCM_DELETEITEM, original_index, 0);
    DestroyWindow(tab_data->scintilla_window);

    free(tab_data);

}

int FindFileTab (WCHAR* file_name, struct editor_data* ed_data)
{
    struct file_tab_data* tab_data;
    int                   index;

    if(file_name == NULL)
        return -1;

    index    = 0;
    tab_data = ed_data->file_tabs;
    while(tab_data != NULL)
    {
        if(_wcsicmp(file_name, tab_data->file_name) == 0)
            return index;

        tab_data = tab_data->next_tab;

        index++;
    }

    return -1;
}

static int DetermineFileTabIndex (struct file_tab_data* tab_data, struct editor_data* ed_data)
{
    struct file_tab_data* scan_tab;
    int                   tab_index;

    tab_index = 0;
    for(scan_tab = ed_data->file_tabs; scan_tab != tab_data; scan_tab = scan_tab->next_tab)
        tab_index++;

    return tab_index;
}

static struct file_tab_data* GetFileTabData (int index, struct editor_data* ed_data)
{
    struct file_tab_data* tab_data;

    tab_data = ed_data->file_tabs;
    while(index--)
        tab_data = tab_data->next_tab;

    return tab_data;
}

static void SetFileTab (int index, struct editor_data* ed_data)
{
    struct file_tab_data* tab_data;
    int                   original_index;

    original_index             = index;
    ed_data->current_tab_index = index;

    tab_data = ed_data->file_tabs;
    while(index--)
        tab_data = tab_data->next_tab;

    ed_data->current_tab = tab_data;

    SendMessage(ed_data->file_tabs_window, TCM_SETCURSEL, original_index, 0);
    ShowWindow(tab_data->scintilla_window, SW_SHOW);
    RepositionControls(ed_data);
    SetFocus(tab_data->scintilla_window);
}

static void SwitchFileTab (int new_tab_index, struct editor_data* ed_data)
{
    if(new_tab_index != ed_data->current_tab_index)
    {
        if(ed_data->current_tab != NULL)
            ShowWindow(ed_data->current_tab->scintilla_window, SW_HIDE);

        SetFileTab(new_tab_index, ed_data);
    }
}

static unsigned int HasUserTabs (struct editor_data* data)
{
    struct file_tab_data* current_tab;
    unsigned int          modified;

    current_tab = data->current_tab;
    if(current_tab == NULL)
        return TSIDE_NO_USER_TABS;

    modified = TSIDE_ScintillaHasModifiedText(current_tab->scintilla_window);
    if(data->total_tabs <= 1 && modified == TSIDE_TEXT_UNMODIFIED && current_tab->file_name[0] == 0)
        return TSIDE_NO_USER_TABS;

    return TSIDE_HAS_USER_TABS;
}

static int OpenNewFile (struct editor_data* data)
{
    int error;

    error = AddFileTab(NULL, NULL, data);
    if(error != TSIDE_ERROR_NONE)
        return error;

    SwitchFileTab(0, data);

    return TSIDE_ERROR_NONE;
}

static int OpenExistingFile (struct editor_data* data)
{
    WCHAR        selected_file[MAX_PATH];
    char*        file_text;
    OPENFILENAME open_data;
    BOOL         winapi_error;
    unsigned int existing_file_new_and_unmodified;
    int          tab_index;
    int          error;

    selected_file[0] = 0;

    open_data.lStructSize       = sizeof(OPENFILENAME);
    open_data.hwndOwner         = data->editor_window;
    open_data.hInstance         = tside_application_instance;
    open_data.lpstrFilter       = L"Trigger Script Files (*.ts)\0*.ts\0\0";
    open_data.lpstrCustomFilter = NULL;
    open_data.nFilterIndex      = 0;
    open_data.lpstrFile         = selected_file;
    open_data.nMaxFile          = MAX_PATH;
    open_data.lpstrFileTitle    = NULL;
    open_data.lpstrInitialDir   = tside_user_units_path_w;
    open_data.lpstrTitle        = NULL;
    open_data.Flags             = OFN_CREATEPROMPT|OFN_FILEMUSTEXIST;
    open_data.nFileOffset       = 0;
    open_data.lpstrDefExt       = TSIDE_GetResourceText(TSIDE_S_TS_FILE_EXTENSION);
    open_data.lCustData         = 0;
    open_data.lpfnHook          = NULL;
    open_data.lpTemplateName    = NULL;
    open_data.pvReserved        = NULL;
    open_data.dwReserved        = 0;
    open_data.FlagsEx           = 0;

    winapi_error = GetOpenFileName(&open_data);
    if(winapi_error == 0)
        goto no_open_selection;

    tab_index = FindFileTab(selected_file, data);
    if(tab_index >= 0)
    {
        SwitchFileTab(tab_index, data);

        return TSIDE_ERROR_NONE;
    }

    existing_file_new_and_unmodified = 0;

    if(data->total_tabs == 1)
    {
        struct file_tab_data* remaining_tab;

        remaining_tab = data->file_tabs;
        if(remaining_tab->file_name[0] == 0)
        {
            unsigned int modified;

            modified = TSIDE_ScintillaHasModifiedText(remaining_tab->scintilla_window);
            if(modified == TSIDE_TEXT_UNMODIFIED)
                existing_file_new_and_unmodified = 1;
        }
    }

    error = TSIDE_GetFileText(selected_file, &file_text);
    if(error != TSIDE_ERROR_NONE)
        goto get_file_text_failed;

    error = AddFileTab(selected_file, file_text, data);

    free(file_text);

    if(error != TSIDE_ERROR_NONE)
        goto add_file_tab_failed;

    if(existing_file_new_and_unmodified != 0)
        RemoveFileTab(1, data);

    SwitchFileTab(0, data);

    return TSIDE_ERROR_NONE;

add_file_tab_failed:
get_file_text_failed:
    return error;

no_open_selection:
    return TSIDE_ERROR_CANCELLED;
}


static struct file_tab_data* VerifyFileOpen (
                                             WCHAR*              file_name,
                                             struct editor_data* ed_data,
                                             int*                index
                                            )
{
    char* file_text;
    int   tab_index;
    int   error;

    tab_index = FindFileTab(file_name, ed_data);
    if(tab_index >= 0)
    {
        struct file_tab_data* tab_data;

        if(index != NULL)
            *index = tab_index;

        tab_data = GetFileTabData(tab_index, ed_data);

        return tab_data;
    }

    error = TSIDE_GetFileText(file_name, &file_text);
    if(error != TSIDE_ERROR_NONE)
        goto get_file_text_failed;

    error = AddFileTab(file_name, file_text, ed_data);

    free(file_text);

    if(error != TSIDE_ERROR_NONE)
        goto add_file_tab_failed;

    SwitchFileTab(0, ed_data);

    if(index != NULL)
        *index = 0;

    return ed_data->file_tabs;

add_file_tab_failed:
get_file_text_failed:
    return NULL;
}

static int SaveFile (struct file_tab_data* tab_data, struct editor_data* ed_data)
{
    char* file_content;
    int   error;

    if(tab_data->file_name[0] == 0)
    {
        error = SaveFileAs(tab_data, ed_data);

        return error;
    }

    error = TSIDE_GetScintillaText(&file_content, tab_data->scintilla_window);
    if(error != TSIDE_ERROR_NONE)
        goto get_file_content_failed;

    error = TSIDE_SaveFileText(tab_data->file_name, file_content);
    if(error != TSIDE_ERROR_NONE)
        goto save_file_content_failed;

    free(file_content);

    TSIDE_ResetModifiedState(tab_data->scintilla_window);

    return TSIDE_ERROR_NONE;

save_file_content_failed:
    free(file_content);

get_file_content_failed:
    return TSIDE_ERROR_EDITOR_ERROR;
}

static int SaveFileAs (struct file_tab_data* tab_data, struct editor_data* ed_data)
{
    WCHAR        selected_file[MAX_PATH];
    TCITEM       updated_item;
    OPENFILENAME open_data;
    BOOL         winapi_error;
    int          tab_index;
    int          error;

    wcscpy(selected_file, tab_data->file_name);

    open_data.lStructSize       = sizeof(OPENFILENAME);
    open_data.hwndOwner         = ed_data->editor_window;
    open_data.hInstance         = tside_application_instance;
    open_data.lpstrFilter       = L"Trigger Script Files (*.ts)\0*.ts\0\0";
    open_data.lpstrCustomFilter = NULL;
    open_data.nFilterIndex      = 0;
    open_data.lpstrFile         = selected_file;
    open_data.nMaxFile          = MAX_PATH;
    open_data.lpstrFileTitle    = NULL;
    open_data.lpstrInitialDir   = tside_user_units_path_w;
    open_data.lpstrTitle        = NULL;
    open_data.Flags             = 0;
    open_data.nFileOffset       = 0;
    open_data.lpstrDefExt       = TSIDE_GetResourceText(TSIDE_S_TS_FILE_EXTENSION);
    open_data.lCustData         = 0;
    open_data.lpfnHook          = NULL;
    open_data.lpTemplateName    = NULL;
    open_data.pvReserved        = NULL;
    open_data.dwReserved        = 0;
    open_data.FlagsEx           = 0;

    winapi_error = GetSaveFileName(&open_data);
    if(winapi_error == 0)
        goto no_save_selection;

    tab_index = FindFileTab(selected_file, ed_data);
    if(tab_index >= 0)
        return TSIDE_ERROR_FILE_IN_USE;

    tab_index = DetermineFileTabIndex(tab_data, ed_data);

    wcscpy(tab_data->file_name, selected_file);

    updated_item.mask    = TCIF_TEXT;
    updated_item.pszText = MakeTabName(selected_file);
    updated_item.lParam  = 0;

    SendMessage(ed_data->file_tabs_window, TCM_SETITEM, tab_index, (LPARAM)&updated_item);
    RepositionControls(ed_data);

    error = SaveFile(tab_data, ed_data);

    return error;

no_save_selection:
    return TSIDE_ERROR_CANCELLED;
}

static int CloseCurrentFile (struct editor_data* ed_data)
{
    int switch_to_tab;
    int remove_tab;
    int proceed;

    if(ed_data->current_tab == NULL)
        return CONTINUE_OPERATION;

    proceed = ValidateFileSaved(ed_data->current_tab, ed_data);
    if(proceed == ABORT_OPERATION)
        return ABORT_OPERATION;

    if(ed_data->total_tabs <= 1)
    {
        int error;

        error = OpenNewFile(ed_data);
        if(error != TSIDE_ERROR_NONE)
        {
            MessageBox(
                       ed_data->editor_window,
                       TSIDE_GetResourceText(TSIDE_S_ADD_FILE_TAB_ERROR),
                       TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                       MB_ICONINFORMATION|MB_OK
                      );

            return ABORT_OPERATION;
        }

        RemoveFileTab(1, ed_data);
    }
    else
    {
        remove_tab    = ed_data->current_tab_index;
        switch_to_tab = remove_tab;
        if(switch_to_tab == 0)
            switch_to_tab++;
        else
            switch_to_tab--;

        SwitchFileTab(switch_to_tab, ed_data);
        RemoveFileTab(remove_tab, ed_data);
    }

    return CONTINUE_OPERATION;
}

static int ValidateFileSaved (struct file_tab_data* tab_data, struct editor_data* ed_data)
{
    unsigned int modified;
    int          error;
    int          selection;

    modified = TSIDE_ScintillaHasModifiedText(tab_data->scintilla_window);
    if(modified == TSIDE_TEXT_UNMODIFIED)
        return CONTINUE_OPERATION;

    selection = MessageBox(
                           ed_data->editor_window,
                           TSIDE_GetResourceText(TSIDE_S_SAVE_MODIFIED_FILE),
                           TSIDE_GetResourceText(TSIDE_S_PROCEED_CAPTION),
                           MB_ICONINFORMATION|MB_YESNOCANCEL
                          );
    if(selection == IDNO)
        return CONTINUE_OPERATION;
    else if(selection == IDCANCEL)
        return ABORT_OPERATION;

    error = SaveFile(tab_data, ed_data);
    if(error != TSIDE_ERROR_NONE)
    {
        if(error != TSIDE_ERROR_CANCELLED)
        {
            MessageBox(
                       ed_data->editor_window,
                       TSIDE_GetResourceText(TSIDE_S_SAVE_FILE_FAILED),
                       TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                       MB_ICONINFORMATION|MB_OK
                      );
        }

        return ABORT_OPERATION;
    }

    return CONTINUE_OPERATION;
}


int TSIDE_InitializeEditor (void)
{
    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownEditor (void)
{
}

unsigned int TSIDE_HasUserTabs (HWND window)
{
    struct editor_data* data;
    unsigned int        has_user_tabs;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return TSIDE_NO_USER_TABS;

    has_user_tabs = HasUserTabs(data);

    return has_user_tabs;
}

int TSIDE_CheckFilesSaved (HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return TSIDE_ERROR_NONE;

    for(tab_data = ed_data->file_tabs; tab_data != NULL; tab_data = tab_data->next_tab)
    {
        int proceed;

        proceed = ValidateFileSaved(tab_data, ed_data);
        if(proceed == ABORT_OPERATION)
            return TSIDE_ERROR_CANCELLED;
    }

    return TSIDE_ERROR_NONE;
}

unsigned int TSIDE_HasUnsavedFiles (HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return TSIDE_ERROR_NONE;

    for(tab_data = ed_data->file_tabs; tab_data != NULL; tab_data = tab_data->next_tab)
    {
        unsigned int modified;

        modified = TSIDE_ScintillaHasModifiedText(tab_data->scintilla_window);
        if(modified != TSIDE_TEXT_UNMODIFIED)
            return TSIDE_FILES_UNSAVED;
    }

    return TSIDE_FILES_UP_TO_DATE;
}

void TSIDE_GetRunnableSelection (WCHAR* selection, HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;
    WCHAR*                tab_name;

    selection[0] = 0;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    tab_data = ed_data->current_tab;
    if(tab_data == NULL)
        return;

    if(tab_data->file_name[0] == 0)
        return;

    wcscpy(selection, tab_data->file_name);

    tab_name = MakeTabName(selection);

    wcscpy(selection, tab_name);
}

void TSIDE_OpenNewFile (HWND window)
{
    struct editor_data* data;
    int                 error;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    error = OpenNewFile(data);
    if(error != TSIDE_ERROR_NONE)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_ADD_FILE_TAB_ERROR),
                   TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                   MB_ICONINFORMATION|MB_OK
                  );
    }
}

void TSIDE_OpenExistingFile (HWND window)
{
    struct editor_data* data;
    int                 error;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    error = OpenExistingFile(data);
    if(error != TSIDE_ERROR_NONE && error != TSIDE_ERROR_CANCELLED)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_OPEN_FILE_ERROR),
                   TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                   MB_ICONINFORMATION|MB_OK
                  );
    }
}

void TSIDE_SaveCurrentFile (HWND window)
{
    struct editor_data* data;
    int                 error;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL || data->current_tab == NULL)
        return;

    error = SaveFile(data->current_tab, data);
    if(error != TSIDE_ERROR_NONE && error != TSIDE_ERROR_CANCELLED)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_SAVE_FILE_FAILED),
                   TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                   MB_ICONINFORMATION|MB_OK
                  );
    }
}

void TSIDE_SaveCurrentFileAs (HWND window)
{
    struct editor_data* data;
    int                 error;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL || data->current_tab == NULL)
        return;

    error = SaveFileAs(data->current_tab, data);
    if(error != TSIDE_ERROR_NONE && error != TSIDE_ERROR_CANCELLED)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_SAVE_FILE_FAILED),
                   TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                   MB_ICONINFORMATION|MB_OK
                  );
    }
}

void TSIDE_SaveAll (HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    for(tab_data = ed_data->file_tabs; tab_data != NULL; tab_data = tab_data->next_tab)
    {
        int error;

        error = SaveFile(tab_data, ed_data);
        if(error != TSIDE_ERROR_NONE)
        {
            if(error != TSIDE_ERROR_CANCELLED)
            {
                MessageBox(
                           ed_data->editor_window,
                           TSIDE_GetResourceText(TSIDE_S_SAVE_FILE_FAILED),
                           TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                           MB_ICONINFORMATION|MB_OK
                          );
            }
        }
    }
}

void TSIDE_CloseCurrentFile (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    CloseCurrentFile(data);
}

void TSIDE_CloseAll (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    while(1)
    {
        unsigned int has_user_tabs;
        int          proceed;

        has_user_tabs = HasUserTabs(data);
        if(has_user_tabs == TSIDE_NO_USER_TABS)
            break;

        proceed = CloseCurrentFile(data);
        if(proceed == ABORT_OPERATION)
            break;
    }
}

void TSIDE_ToggleSearchText (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->search_state != SEARCH_STATE_TEXT)
        SetSearchState(SEARCH_STATE_TEXT, data);
    else
        SetSearchState(SEARCH_STATE_HIDDEN, data);

    RepositionControls(data);
}

void TSIDE_ToggleSearchGoto (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->search_state != SEARCH_STATE_GOTO)
        SetSearchState(SEARCH_STATE_GOTO, data);
    else
        SetSearchState(SEARCH_STATE_HIDDEN, data);

    RepositionControls(data);
}

void TSIDE_GotoLineNumber (HWND window)
{
    struct editor_data*   data;
    struct file_tab_data* current_tab;
    HWND                  scintilla_window;
    int                   line_number;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    current_tab = data->current_tab;
    if(current_tab == NULL)
        return;

    line_number      = TSIDE_GetGotoLineNumber(data->search_goto_window);
    scintilla_window = current_tab->scintilla_window;

    TSIDE_GotoScintillaLine(line_number, scintilla_window);
    SetSearchState(SEARCH_STATE_HIDDEN, data);
    RepositionControls(data);
}

void TSIDE_SearchFindNext (HWND window)
{
    char                find_text[TSIDE_MAX_SEARCH_LENGTH];
    struct editor_data* data;
    HWND                scintilla_window;
    unsigned int        find_result;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    scintilla_window = data->current_tab->scintilla_window;

    TSIDE_GetFindText(data->search_text_window, find_text);

    if(find_text[0] == 0)
    {
        TSIDE_ToggleSearchText(window);

        return;
    }

    find_result = TSIDE_SearchNextScintillaText(
                                                -1,
                                                find_text,
                                                NULL,
                                                scintilla_window
                                               );
    if(find_result == TSIDE_TEXT_NOT_FOUND)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_FIND_END_OF_DOCUMENT_REACHED),
                   TSIDE_GetResourceText(TSIDE_S_FIND_COMPLETE),
                   MB_ICONINFORMATION|MB_OK
                  );
    }

    SetFocus(scintilla_window);
}

void TSIDE_SearchFindPrevious (HWND window)
{
    char                find_text[TSIDE_MAX_SEARCH_LENGTH];
    struct editor_data* data;
    HWND                scintilla_window;
    unsigned int        find_result;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    scintilla_window = data->current_tab->scintilla_window;

    TSIDE_GetFindText(data->search_text_window, find_text);

    if(find_text[0] == 0)
    {
        TSIDE_ToggleSearchText(window);

        return;
    }

    find_result = TSIDE_SearchPrevScintillaText(
                                                -1,
                                                find_text,
                                                NULL,
                                                scintilla_window
                                               );
    if(find_result == TSIDE_TEXT_NOT_FOUND)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_FIND_START_OF_DOCUMENT_REACHED),
                   TSIDE_GetResourceText(TSIDE_S_FIND_COMPLETE),
                   MB_ICONINFORMATION|MB_OK
                  );
    }

    SetFocus(scintilla_window);
}

void TSIDE_SearchReplaceNext (HWND window)
{
    char                find_text[TSIDE_MAX_SEARCH_LENGTH];
    char                replace_text[TSIDE_MAX_SEARCH_LENGTH];
    struct editor_data* data;
    HWND                scintilla_window;
    unsigned int        find_result;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    scintilla_window = data->current_tab->scintilla_window;

    TSIDE_GetFindText(data->search_text_window, find_text);

    if(find_text[0] == 0)
    {
        TSIDE_ToggleSearchText(window);

        return;
    }

    TSIDE_GetReplaceText(data->search_text_window, replace_text);

    find_result = TSIDE_SearchNextScintillaText(
                                                -1,
                                                find_text,
                                                replace_text,
                                                scintilla_window
                                               );
    if(find_result == TSIDE_TEXT_NOT_FOUND)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_FIND_END_OF_DOCUMENT_REACHED),
                   TSIDE_GetResourceText(TSIDE_S_FIND_COMPLETE),
                   MB_ICONINFORMATION|MB_OK
                  );
    }

    SetFocus(scintilla_window);
}

void TSIDE_SearchReplaceAll (HWND window)
{
    char                find_text[TSIDE_MAX_SEARCH_LENGTH];
    char                replace_text[TSIDE_MAX_SEARCH_LENGTH];
    struct editor_data* data;
    HWND                scintilla_window;
    unsigned int        find_result;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    scintilla_window = data->current_tab->scintilla_window;

    TSIDE_GetFindText(data->search_text_window, find_text);

    if(find_text[0] == 0)
    {
        TSIDE_ToggleSearchText(window);

        return;
    }

    TSIDE_GetReplaceText(data->search_text_window, replace_text);

    do
    {
        find_result = TSIDE_SearchNextScintillaText(
                                                    0,
                                                    find_text,
                                                    replace_text,
                                                    scintilla_window
                                                   );
    }while(find_result != TSIDE_TEXT_NOT_FOUND);

    SetFocus(scintilla_window);
}

void TSIDE_EditUndo (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    TSIDE_ScintillaUndo(data->current_tab->scintilla_window);
}

void TSIDE_EditRedo (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    TSIDE_ScintillaRedo(data->current_tab->scintilla_window);
}

void TSIDE_EditCut (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    TSIDE_ScintillaCut(data->current_tab->scintilla_window);
}

void TSIDE_EditCopy (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    TSIDE_ScintillaCopy(data->current_tab->scintilla_window);
}

void TSIDE_EditPaste (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    TSIDE_ScintillaPaste(data->current_tab->scintilla_window);
}

void TSIDE_EditSelectAll (HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    TSIDE_ScintillaSelectAll(data->current_tab->scintilla_window);
}

void TSIDE_InsertText (char* text, HWND window)
{
    struct editor_data* data;

    data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(data == NULL)
        return;

    if(data->current_tab == NULL)
        return;

    TSIDE_ScintillaInsertText(text, data->current_tab->scintilla_window);
}

void TSIDE_UseTemplate (char* text, HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* current_tab;
    int                   error;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    current_tab = ed_data->current_tab;
    if(current_tab != NULL && current_tab->file_name[0] == 0)
    {
        unsigned int modified;

        modified = TSIDE_ScintillaHasModifiedText(current_tab->scintilla_window);
        if(modified == TSIDE_TEXT_UNMODIFIED)
        {
            TSIDE_SetScintillaText(text, current_tab->scintilla_window);

            return;
        }
    }

    error = AddFileTab(NULL, text, ed_data);
    if(error != TSIDE_ERROR_NONE)
        goto use_template_failed;

    SwitchFileTab(0, ed_data);

    return;

use_template_failed:
    MessageBox(
               ed_data->editor_window,
               TSIDE_GetResourceText(TSIDE_S_ADD_FILE_TAB_ERROR),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

void TSIDE_AnnotateAlert (struct tside_alert* alert, HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;
    int                   error;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    if(alert->file_name[0] == 0)
        return;

    tab_data = VerifyFileOpen(alert->file_name, ed_data, NULL);
    if(tab_data == NULL)
        goto verify_file_open_failed;

    error = TSIDE_AnnotateScintilla(
                                    alert->alert_text,
                                    alert->location,
                                    alert->type,
                                    tab_data->scintilla_window
                                   );
    if(error != TSIDE_ERROR_NONE)
        goto annotate_failed;

    return;

annotate_failed:
verify_file_open_failed:
    MessageBox(
               ed_data->editor_window,
               TSIDE_GetResourceText(TSIDE_S_COULD_NOT_ANNOTATE),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

void TSIDE_GotoAlert (struct tside_alert* alert, HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;
    int                   tab_index;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    if(alert->file_name[0] == 0)
        return;

    tab_data = VerifyFileOpen(alert->file_name, ed_data, &tab_index);
    if(tab_data == NULL)
        goto verify_file_open_failed;

    SwitchFileTab(tab_index, ed_data);
    TSIDE_GotoScintillaLine(alert->location, tab_data->scintilla_window);
    SetFocus(tab_data->scintilla_window);

    return;

verify_file_open_failed:
    MessageBox(
               ed_data->editor_window,
               TSIDE_GetResourceText(TSIDE_S_OPEN_FILE_ERROR),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

void TSIDE_ClearAnnotations (HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    for(
        tab_data = ed_data->file_tabs;
        tab_data != NULL;
        tab_data = tab_data->next_tab
       )
    {
        TSIDE_ClearScintillaAnnotations(tab_data->scintilla_window);
    }
}

void TSIDE_HighlightLocation (
                              WCHAR*       file_name,
                              unsigned int line_number,
                              unsigned int type,
                              HWND         window
                             )
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;
    int                   tab_index;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    if(file_name[0] == 0)
        return;

    if(type == TSIDE_HIGHLIGHT_NONE)
    {
        tab_index = FindFileTab(file_name, ed_data);
        if(tab_index < 0)
            return;

        tab_data = GetFileTabData(tab_index, ed_data);
    }
    else
    {
        tab_data = VerifyFileOpen(file_name, ed_data, &tab_index);
        if(tab_data == NULL)
            goto verify_file_open_failed;
    }

    SwitchFileTab(tab_index, ed_data);
    TSIDE_HighlightScintillaLine(line_number, type, tab_data->scintilla_window);
    SetFocus(tab_data->scintilla_window);

    return;

verify_file_open_failed:
    MessageBox(
               ed_data->editor_window,
               TSIDE_GetResourceText(TSIDE_S_OPEN_FILE_ERROR),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

void TSIDE_SetReadOnlyMode (unsigned int mode, HWND window)
{
    struct editor_data*   ed_data;
    struct file_tab_data* tab_data;

    ed_data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
    if(ed_data == NULL)
        return;

    switch(mode)
    {
    case TSIDE_READONLY:
        ed_data->editor_flags |= EDITOR_FLAG_READONLY;

        break;

    case TSIDE_WRITABLE:
        ed_data->editor_flags &= ~EDITOR_FLAG_READONLY;

        break;
    }

    for(
        tab_data = ed_data->file_tabs;
        tab_data != NULL;
        tab_data = tab_data->next_tab
       )
    {
        TSIDE_SetScintillaReadOnly(mode, tab_data->scintilla_window);
    }
}

INT_PTR CALLBACK TSIDE_EditorMessageProc (
                                          HWND   window,
                                          UINT   message,
                                          WPARAM wparam,
                                          LPARAM lparam
                                         )
{
    struct editor_data* data;
    NMHDR*              notify_data;
    LRESULT             current_selection;
    int                 error;
    WORD                command;

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
        data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_SETFOCUS:
        return TRUE;

    case WM_SIZE:
        data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        RepositionControls(data);

        return TRUE;

    case WM_COMMAND:
        data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        command = LOWORD(wparam);

        switch(command)
        {
        case TSIDE_M_IDE_SEARCH_FIND_NEXT:
            TSIDE_SearchFindNext(window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_FIND_PREVIOUS:
            TSIDE_SearchFindPrevious(window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_REPLACE_NEXT:
            TSIDE_SearchReplaceNext(window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_REPLACE_ALL:
            TSIDE_SearchReplaceAll(window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_PERFORM_GOTO:
            TSIDE_GotoLineNumber(window);

            return TRUE;
        }

        return FALSE;

    case WM_NOTIFY:
        data = (struct editor_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notify_data = (LPNMHDR)lparam;

        switch(notify_data->code)
        {
        case SCN_SAVEPOINTLEFT:
            if(data->parent_window != NULL)
            {
                NMHDR forward_notify;

                forward_notify.hwndFrom = data->editor_window;
                forward_notify.idFrom   = 0;
                forward_notify.code     = TSIDE_NOTIFY_DOCUMENT_MODIFIED;

                SendMessage(data->parent_window, WM_NOTIFY, 0, (LPARAM)&forward_notify);
            }

            return TRUE;

        case SCN_MODIFYATTEMPTRO:
            MessageBox(
                       window,
                       TSIDE_GetResourceText(TSIDE_S_CANNOT_MODIFY_WHILE_RUNNING),
                       TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
                       MB_ICONINFORMATION|MB_OK
                      );

            return TRUE;

        case TCN_SELCHANGE:
            current_selection = SendMessage(data->file_tabs_window, TCM_GETCURSEL, 0, 0);
            if(current_selection < 0)
                return TRUE;

            SwitchFileTab(current_selection, data);

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_EDITOR_ERROR);

    return FALSE;
}

