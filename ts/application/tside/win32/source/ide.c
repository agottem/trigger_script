/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "ide.h"
#include "gui.h"
#include "editor.h"
#include "select.h"
#include "reference.h"
#include "run.h"
#include "scintilla.h"
#include "text.h"
#include "resources.h"
#include "alerts.h"
#include "main.h"
#include "error.h"

#include <tsint/controller.h>

#include <malloc.h>
#include <string.h>
#include <commctrl.h>
#include <commdlg.h>
#include <scintilla/Scintilla.h>


#define RUN_STATE_HIDDEN    0
#define RUN_STATE_SHOWN     1
#define RUN_STATE_COMPILING 2
#define RUN_STATE_EXECUTING 3
#define RUN_STATE_BREAK     4

#define CONTINUE_OPERATION  0
#define ABORT_OPERATION    -1

#define DOCUMENT_FLAG_MODIFIED_WHILE_RUN_SHOWN 0x01


struct ide_data
{
    HWND toolbar_window;
    HWND editor_window;

    unsigned int run_state;
    WCHAR*       run_current_unit;
    unsigned int run_current_location;

    unsigned int document_flags;

    RECT original_dimensions;

    HMENU ide_menu;
};


static int  InitializeDialog   (HWND);
static void DestroyDialog      (struct ide_data*);
static void RepositionControls (struct ide_data*);
static void UpdateRunState     (unsigned int, struct ide_data*);
static void UpdateMenuState    (struct ide_data*);

static void MarkDocumentModified (struct ide_data*);

static void    PrepareForRun  (unsigned int, struct ide_data*);
static LRESULT RunBeginning   (struct ide_data*);
static void    StopRun        (struct ide_data*);
static void    BreakRun       (WCHAR*, unsigned int, struct ide_data*);
static void    PrintVariables (struct ide_data*);
static void    ContinueRun    (unsigned int, struct ide_data*);

static int TryDialogClose (struct ide_data*);

static void DisplayAbout (void);


static int InitializeDialog (HWND window)
{
    TBBUTTON         toolbar_buttons[23];
    TBADDBITMAP      toolbar_bitmap;
    struct ide_data* data;
    HICON            icon;
    HWND             toolbar_window;
    HWND             editor_window;

    data = malloc(sizeof(struct ide_data));
    if(data == NULL)
        goto allocate_ide_data_failed;

    toolbar_window = CreateWindowEx(
                                    WS_EX_STATICEDGE,
                                    TOOLBARCLASSNAME,
                                    NULL,
                                    WS_CHILD|WS_VISIBLE|WS_BORDER|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT|TBSTYLE_SEP,
                                    0,
                                    0,
                                    0,
                                    0,
                                    window,
                                    (HMENU)TSIDE_C_IDE_TOOLBAR,
                                    tside_application_instance,
                                    NULL
                                   );
    if(toolbar_window == NULL)
        goto create_toolbar_window_failed;

    icon = LoadIcon(tside_application_instance, MAKEINTRESOURCE(TSIDE_I_GUI_MAIN));
    if(icon == NULL)
        goto load_main_icon_failed;

    SendMessage(window, WM_SETICON, ICON_BIG, (LPARAM)icon);

    SendMessage(toolbar_window, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

    toolbar_bitmap.nID   = TSIDE_B_IDE_TOOLBAR;
    toolbar_bitmap.hInst = tside_application_instance;

    SendMessage(toolbar_window, TB_ADDBITMAP, 23, (LPARAM)&toolbar_bitmap);

    ZeroMemory(&toolbar_buttons, sizeof(toolbar_buttons));

    toolbar_buttons[0].iBitmap   = 0;
    toolbar_buttons[0].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[0].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[0].idCommand = TSIDE_M_IDE_FILE_NEW;
    toolbar_buttons[0].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_NEW_FILE);

    toolbar_buttons[1].iBitmap   = 1;
    toolbar_buttons[1].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[1].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[1].idCommand = TSIDE_M_IDE_FILE_OPEN;
    toolbar_buttons[1].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_OPEN_FILE);

    toolbar_buttons[2].iBitmap   = 2;
    toolbar_buttons[2].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[2].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[2].idCommand = TSIDE_M_IDE_FILE_SAVE;
    toolbar_buttons[2].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_SAVE_FILE);

    toolbar_buttons[3].iBitmap   = 3;
    toolbar_buttons[3].fsState   = 0;
    toolbar_buttons[3].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[3].idCommand = TSIDE_M_IDE_FILE_CLOSE;
    toolbar_buttons[3].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_CLOSE_FILE);

    toolbar_buttons[4].iBitmap   = 4;
    toolbar_buttons[4].fsState   = 0;
    toolbar_buttons[4].fsStyle   = TBSTYLE_SEP;
    toolbar_buttons[4].idCommand = 0;
    toolbar_buttons[4].iString   = 0;

    toolbar_buttons[5].iBitmap   = 5;
    toolbar_buttons[5].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[5].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[5].idCommand = TSIDE_M_IDE_SEARCH_FIND;
    toolbar_buttons[5].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_FIND);

    toolbar_buttons[6].iBitmap   = 6;
    toolbar_buttons[6].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[6].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[6].idCommand = TSIDE_M_IDE_SEARCH_GOTO;
    toolbar_buttons[6].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_GOTO);

    toolbar_buttons[7].iBitmap   = 7;
    toolbar_buttons[7].fsState   = 0;
    toolbar_buttons[7].fsStyle   = TBSTYLE_SEP;
    toolbar_buttons[7].idCommand = 0;
    toolbar_buttons[7].iString   = 0;

    toolbar_buttons[8].iBitmap   = 8;
    toolbar_buttons[8].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[8].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[8].idCommand = TSIDE_M_IDE_EDIT_CUT;
    toolbar_buttons[8].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_CUT);

    toolbar_buttons[9].iBitmap   = 9;
    toolbar_buttons[9].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[9].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[9].idCommand = TSIDE_M_IDE_EDIT_COPY;
    toolbar_buttons[9].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_COPY);

    toolbar_buttons[10].iBitmap   = 10;
    toolbar_buttons[10].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[10].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[10].idCommand = TSIDE_M_IDE_EDIT_PASTE;
    toolbar_buttons[10].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_PASTE);

    toolbar_buttons[11].iBitmap   = 11;
    toolbar_buttons[11].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[11].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[11].idCommand = TSIDE_M_IDE_EDIT_UNDO;
    toolbar_buttons[11].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_UNDO);

    toolbar_buttons[12].iBitmap   = 12;
    toolbar_buttons[12].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[12].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[12].idCommand = TSIDE_M_IDE_EDIT_REDO;
    toolbar_buttons[12].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_REDO);

    toolbar_buttons[13].iBitmap   = 13;
    toolbar_buttons[13].fsState   = 0;
    toolbar_buttons[13].fsStyle   = TBSTYLE_SEP;
    toolbar_buttons[13].idCommand = 0;
    toolbar_buttons[13].iString   = 0;

    toolbar_buttons[14].iBitmap   = 14;
    toolbar_buttons[14].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[14].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[14].idCommand = TSIDE_M_IDE_REFERENCE_PLUGIN_FUNCTIONS;
    toolbar_buttons[14].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_REFERENCE);

    toolbar_buttons[15].iBitmap   = 15;
    toolbar_buttons[15].fsState   = 0;
    toolbar_buttons[15].fsStyle   = TBSTYLE_SEP;
    toolbar_buttons[15].idCommand = 0;
    toolbar_buttons[15].iString   = 0;

    toolbar_buttons[16].iBitmap   = 16;
    toolbar_buttons[16].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[16].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[16].idCommand = TSIDE_M_IDE_RUN_START;
    toolbar_buttons[16].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_START);

    toolbar_buttons[17].iBitmap   = 17;
    toolbar_buttons[17].fsState   = 0;
    toolbar_buttons[17].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[17].idCommand = TSIDE_M_IDE_RUN_STOP;
    toolbar_buttons[17].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_STOP);

    toolbar_buttons[18].iBitmap   = 18;
    toolbar_buttons[18].fsState   = TBSTATE_ENABLED;
    toolbar_buttons[18].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[18].idCommand = TSIDE_M_IDE_RUN_DEBUG;
    toolbar_buttons[18].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_DEBUG);

    toolbar_buttons[19].iBitmap   = 19;
    toolbar_buttons[19].fsState   = 0;
    toolbar_buttons[19].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[19].idCommand = TSIDE_M_IDE_RUN_CONTINUE;
    toolbar_buttons[19].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_CONTINUE);

    toolbar_buttons[20].iBitmap   = 20;
    toolbar_buttons[20].fsState   = 0;
    toolbar_buttons[20].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[20].idCommand = TSIDE_M_IDE_RUN_STEP;
    toolbar_buttons[20].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_STEP);

    toolbar_buttons[21].iBitmap   = 21;
    toolbar_buttons[21].fsState   = 0;
    toolbar_buttons[21].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[21].idCommand = TSIDE_M_IDE_RUN_STEP_INTO;
    toolbar_buttons[21].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_STEP_INTO);

    toolbar_buttons[22].iBitmap   = 22;
    toolbar_buttons[22].fsState   = 0;
    toolbar_buttons[22].fsStyle   = TBSTYLE_BUTTON;
    toolbar_buttons[22].idCommand = TSIDE_M_IDE_RUN_VARIABLES;
    toolbar_buttons[22].iString   = (INT_PTR)TSIDE_GetResourceText(TSIDE_S_TOOLTIP_VARIABLES);

    SendMessage(toolbar_window, TB_SETMAXTEXTROWS, 0, 0);
    SendMessage(
                toolbar_window,
                TB_ADDBUTTONS,
                sizeof(toolbar_buttons)/sizeof(TBBUTTON),
                (LPARAM)&toolbar_buttons
               );

    editor_window = CreateDialog(
                                 tside_application_instance,
                                 MAKEINTRESOURCE(TSIDE_D_EDITOR),
                                 window,
                                 &TSIDE_EditorMessageProc
                                );
    if(editor_window == NULL)
        goto create_editor_window_failed;

    GetClientRect(window, &data->original_dimensions);

    data->toolbar_window = toolbar_window;
    data->editor_window  = editor_window;
    data->run_state      = RUN_STATE_HIDDEN;
    data->document_flags = 0;
    data->ide_menu       = GetMenu(window);

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    UpdateMenuState(data);
    RepositionControls(data);

    return TSIDE_ERROR_NONE;

create_editor_window_failed:
    DestroyIcon(icon);
load_main_icon_failed:
    DestroyWindow(toolbar_window);
create_toolbar_window_failed:
    free(data);

allocate_ide_data_failed:
    return TSIDE_ERROR_IDE_ERROR;
}

static void DestroyDialog (struct ide_data* data)
{
    free(data);
}

static void RepositionControls (struct ide_data* data)
{
    RECT ide_client_area;
    RECT toolbar_window_area;
    RECT editor_client_area;
    LONG toolbar_height;

    GetClientRect(tside_ide_window, &ide_client_area);
    GetWindowRect(data->toolbar_window, &toolbar_window_area);

    toolbar_height = toolbar_window_area.bottom-toolbar_window_area.top;

    SetWindowPos(
                 data->toolbar_window,
                 NULL,
                 0,
                 0,
                 ide_client_area.right,
                 toolbar_height,
                 SWP_NOZORDER
                );

    editor_client_area.left   = ide_client_area.left;
    editor_client_area.top    = ide_client_area.top+toolbar_height;
    editor_client_area.right  = ide_client_area.right;
    editor_client_area.bottom = ide_client_area.bottom;

    SetWindowPos(
                 data->editor_window,
                 NULL,
                 editor_client_area.left,
                 editor_client_area.top,
                 editor_client_area.right-editor_client_area.left,
                 editor_client_area.bottom-editor_client_area.top,
                 SWP_NOZORDER
                );
}

static void UpdateRunState (unsigned int run_state, struct ide_data* data)
{
    switch(run_state)
    {
    case RUN_STATE_HIDDEN:
        data->document_flags &= ~DOCUMENT_FLAG_MODIFIED_WHILE_RUN_SHOWN;

        TSIDE_SetReadOnlyMode(TSIDE_WRITABLE, data->editor_window);

        break;

    case RUN_STATE_SHOWN:
        TSIDE_SetReadOnlyMode(TSIDE_WRITABLE, data->editor_window);

        break;

    case RUN_STATE_COMPILING:
        TSIDE_SetReadOnlyMode(TSIDE_READONLY, data->editor_window);

        break;

    case RUN_STATE_EXECUTING:
    case RUN_STATE_BREAK:
        break;
    }

    data->run_state = run_state;

    UpdateMenuState(data);
}

static void UpdateMenuState (struct ide_data* data)
{
    unsigned int has_user_tabs;

    has_user_tabs = TSIDE_HasUserTabs(data->editor_window);
    if(has_user_tabs == TSIDE_NO_USER_TABS)
    {
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_FILE_CLOSE, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_FILE_CLOSE_ALL, MF_DISABLED|MF_GRAYED);

        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_FILE_CLOSE, 0);
    }
    else
    {
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_FILE_CLOSE, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_FILE_CLOSE_ALL, MF_ENABLED);

        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_FILE_CLOSE, TBSTATE_ENABLED);
    }

    switch(data->run_state)
    {
    case RUN_STATE_HIDDEN:
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_START, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_DEBUG, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_CONTINUE, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STOP, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP_INTO, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_VARIABLES, MF_DISABLED|MF_GRAYED);

        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_START, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_DEBUG, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_CONTINUE, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STOP, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP_INTO, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_VARIABLES, 0);

        break;

    case RUN_STATE_SHOWN:
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_START, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_DEBUG, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_CONTINUE, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STOP, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP_INTO, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_VARIABLES, MF_DISABLED|MF_GRAYED);

        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_START, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_DEBUG, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_CONTINUE, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STOP, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP_INTO, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_VARIABLES, 0);

        break;

    case RUN_STATE_COMPILING:
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_START, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_DEBUG, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_CONTINUE, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STOP, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP_INTO, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_VARIABLES, MF_DISABLED|MF_GRAYED);

        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_START, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_DEBUG, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_CONTINUE, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STOP, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP_INTO, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_VARIABLES, 0);

        break;

    case RUN_STATE_EXECUTING:
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_START, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_DEBUG, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_CONTINUE, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STOP, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP_INTO, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_VARIABLES, MF_DISABLED|MF_GRAYED);

        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_START, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_DEBUG, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_CONTINUE, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STOP, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP_INTO, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_VARIABLES, 0);

        break;

    case RUN_STATE_BREAK:
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_START, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_DEBUG, MF_DISABLED|MF_GRAYED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_CONTINUE, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STOP, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_STEP_INTO, MF_ENABLED);
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_RUN_VARIABLES, MF_ENABLED);

        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_START, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_DEBUG, 0);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_CONTINUE, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STOP, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_STEP_INTO, TBSTATE_ENABLED);
        SendMessage(data->toolbar_window, TB_SETSTATE, TSIDE_M_IDE_RUN_VARIABLES, TBSTATE_ENABLED);

        break;
    }

    if(data->run_state != RUN_STATE_HIDDEN && data->run_state != RUN_STATE_SHOWN)
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_FILE_EXIT, MF_DISABLED|MF_GRAYED);
    else
        EnableMenuItem(data->ide_menu, TSIDE_M_IDE_FILE_EXIT, MF_ENABLED);
}

static void MarkDocumentModified (struct ide_data* data)
{
    if(data->run_state != RUN_STATE_HIDDEN)
        data->document_flags |= DOCUMENT_FLAG_MODIFIED_WHILE_RUN_SHOWN;

    UpdateMenuState(data);
}

static void PrepareForRun (unsigned int mode, struct ide_data* data)
{
    WCHAR        selected_name[MAX_PATH];
    unsigned int file_state;

    if(data->run_state != RUN_STATE_HIDDEN && data->run_state != RUN_STATE_SHOWN)
        return;

    file_state = TSIDE_HasUnsavedFiles(data->editor_window);
    if(file_state == TSIDE_FILES_UNSAVED)
    {
        int selection;

        selection = MessageBox(
                               data->editor_window,
                               TSIDE_GetResourceText(TSIDE_S_UNSAVED_FILES_ON_RUN),
                               TSIDE_GetResourceText(TSIDE_S_CONTINUE_UNSAVED),
                               MB_ICONINFORMATION|MB_YESNOCANCEL
                              );
        if(selection == IDCANCEL)
            return;

        if(selection == IDYES)
            TSIDE_SaveAll(data->editor_window);
    }

    TSIDE_GetRunnableSelection(selected_name, data->editor_window);
    if(selected_name[0] == 0)
    {
        MessageBox(
                   data->editor_window,
                   TSIDE_GetResourceText(TSIDE_S_NO_RUNNABLE_SELECTION),
                   TSIDE_GetResourceText(TSIDE_S_CANNOT_RUN),
                   MB_ICONINFORMATION|MB_OK
                  );

        return;
    }

    TSIDE_SetRunData(selected_name, mode);

    if(data->run_state == RUN_STATE_SHOWN)
        BringWindowToTop(tside_run_window);
    else
        ShowWindow(tside_run_window, SW_SHOWNORMAL);

    UpdateRunState(RUN_STATE_SHOWN, data);
}

static LRESULT RunBeginning (struct ide_data* data)
{
    if(data->document_flags&DOCUMENT_FLAG_MODIFIED_WHILE_RUN_SHOWN)
    {
        unsigned int file_state;

        file_state = TSIDE_HasUnsavedFiles(data->editor_window);
        if(file_state == TSIDE_FILES_UNSAVED)
        {
            int selection;

            selection = MessageBox(
                                   data->editor_window,
                                   TSIDE_GetResourceText(TSIDE_S_UNSAVED_FILES_ON_RUN),
                                   TSIDE_GetResourceText(TSIDE_S_CONTINUE_UNSAVED),
                                   MB_ICONINFORMATION|MB_YESNOCANCEL
                                  );
            if(selection == IDCANCEL)
                return FALSE;

            if(selection == IDYES)
                TSIDE_SaveAll(data->editor_window);
        }

        data->document_flags &= ~DOCUMENT_FLAG_MODIFIED_WHILE_RUN_SHOWN;
    }

    return TRUE;
}

static void StopRun (struct ide_data* data)
{
    if(data->run_state == RUN_STATE_BREAK)
        ContinueRun(TSINT_CONTROL_HALT, data);
    else
        TSIDE_AbortRun();
}

static void BreakRun (WCHAR* file_name, unsigned int line, struct ide_data* data)
{
    UpdateRunState(RUN_STATE_BREAK, data);

    TSIDE_HighlightLocation(
                            file_name,
                            line,
                            TSIDE_HIGHLIGHT_EXECUTION,
                            data->editor_window
                           );

    data->run_current_unit     = file_name;
    data->run_current_location = line;
}

static void PrintVariables (struct ide_data* data)
{
    if(data->run_state != RUN_STATE_BREAK)
        return;

    TSIDE_PrintVariables();
}

static void ContinueRun (unsigned int mode, struct ide_data* data)
{
    if(data->run_state != RUN_STATE_BREAK)
        return;

    TSIDE_HighlightLocation(
                            data->run_current_unit,
                            data->run_current_location,
                            TSIDE_HIGHLIGHT_NONE,
                            data->editor_window
                           );

    UpdateRunState(RUN_STATE_EXECUTING, data);
    TSIDE_SetRunControllerMode(mode);
}

static int TryDialogClose (struct ide_data* data)
{
    int error;

    if(data->run_state != RUN_STATE_SHOWN && data->run_state != RUN_STATE_HIDDEN)
    {
        MessageBox(
                   tside_ide_window,
                   TSIDE_GetResourceText(TSIDE_S_CANNOT_CLOSE_WHILE_RUNNING),
                   TSIDE_GetResourceText(TSIDE_S_RUN_IN_PROGRESS),
                   MB_ICONINFORMATION|MB_OK
                  );

        return ABORT_OPERATION;
    }

    error = TSIDE_CheckFilesSaved(data->editor_window);
    if(error != TSIDE_ERROR_NONE)
        return ABORT_OPERATION;

    return CONTINUE_OPERATION;
}

static void DisplayAbout (void)
{
    MessageBox(
               tside_ide_window,
               L"The TS language, compiler, and interpreter, along with the "
               L"corresponding TS command line interface and TS IDE, were "
               L"architected and implemented by Andrew Gottemoller between "
               L"late March and early June of 2011.\n"
               L"\n"
               L"Copyright 2011 Andrew Gottemoller.\n"
               L"\n"
               L"Trigger Script is free software: you can redistribute it and/or modify "
               L"it under the terms of the GNU General Public License as published by "
               L"the Free Software Foundation, either version 3 of the License, or "
               L"(at your option) any later version.\n"
               L"\n"
               L"Trigger Script is distributed in the hope that it will be useful, "
               L"but WITHOUT ANY WARRANTY; without even the implied warranty of "
               L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
               L"GNU General Public License for more details.\n"
               L"\n"
               L"You should have received a copy of the GNU General Public License "
               L"along with Trigger Script.  If not, see <http://www.gnu.org/licenses/>.",
               L"Don't you have anything better to do?",
               MB_OK|MB_ICONEXCLAMATION
              );
}


int TSIDE_InitializeIDE (void)
{
    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownIDE (void)
{
}

void TSIDE_PresentIDE (void)
{
    ShowWindow(tside_ide_window, SW_SHOWNORMAL);

    DialogBox(
              tside_application_instance,
              MAKEINTRESOURCE(TSIDE_D_SELECT),
              tside_ide_window,
              &TSIDE_SelectMessageProc
             );
}

INT_PTR CALLBACK TSIDE_IDEMessageProc (
                                       HWND   window,
                                       UINT   message,
                                       WPARAM wparam,
                                       LPARAM lparam
                                      )
{
    struct ide_data* data;
    NMHDR*           notify_data;
    MINMAXINFO*      min_max_info;
    LRESULT          result;
    int              error;
    int              proceed;
    WORD             notify_id;
    WORD             control_id;

    switch(message)
    {
    case WM_INITDIALOG:
        error = InitializeDialog(window);
        if(error != TSIDE_ERROR_NONE)
            goto initialize_dialog_failed;

        return TRUE;

    case WM_CLOSE:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
        {
            PostQuitMessage(TSIDE_ERROR_NONE);

            return TRUE;
        }

        proceed = TryDialogClose(data);
        if(proceed == ABORT_OPERATION)
            return TRUE;

        PostQuitMessage(TSIDE_ERROR_NONE);

        return TRUE;

    case WM_DESTROY:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_SETFOCUS:
        return TRUE;

    case WM_GETMINMAXINFO:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        min_max_info = (MINMAXINFO*)lparam;

        min_max_info->ptMinTrackSize.x = data->original_dimensions.right;
        min_max_info->ptMinTrackSize.y = data->original_dimensions.bottom;

        return TRUE;

    case WM_SIZE:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        RepositionControls(data);

        return TRUE;

    case WM_COMMAND:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notify_id  = HIWORD(wparam);
        control_id = LOWORD(wparam);

        switch(control_id)
        {
        case TSIDE_M_IDE_FILE_NEW:
            TSIDE_OpenNewFile(data->editor_window);
            UpdateMenuState(data);

            return TRUE;

        case TSIDE_M_IDE_FILE_OPEN:
            TSIDE_OpenExistingFile(data->editor_window);
            UpdateMenuState(data);

            return TRUE;

        case TSIDE_M_IDE_FILE_SAVE:
            TSIDE_SaveCurrentFile(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_FILE_SAVE_AS:
            TSIDE_SaveCurrentFileAs(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_FILE_SAVE_ALL:
            TSIDE_SaveAll(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_FILE_CLOSE:
            TSIDE_CloseCurrentFile(data->editor_window);
            UpdateMenuState(data);

            return TRUE;

        case TSIDE_M_IDE_FILE_CLOSE_ALL:
            TSIDE_CloseAll(data->editor_window);
            UpdateMenuState(data);

            return TRUE;

        case TSIDE_M_IDE_FILE_EXIT:
            PostMessage(window, WM_CLOSE, 0, 0);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_FIND:
            TSIDE_ToggleSearchText(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_FIND_NEXT:
            TSIDE_SearchFindNext(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_FIND_PREVIOUS:
            TSIDE_SearchFindPrevious(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_REPLACE:
            TSIDE_ToggleSearchText(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_REPLACE_NEXT:
            TSIDE_SearchReplaceNext(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_REPLACE_ALL:
            TSIDE_SearchReplaceAll(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_SEARCH_GOTO:
            TSIDE_ToggleSearchGoto(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_EDIT_UNDO:
            TSIDE_EditUndo(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_EDIT_REDO:
            TSIDE_EditRedo(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_EDIT_CUT:
            TSIDE_EditCut(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_EDIT_COPY:
            TSIDE_EditCopy(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_EDIT_PASTE:
            TSIDE_EditPaste(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_EDIT_SELECT_ALL:
            TSIDE_EditSelectAll(data->editor_window);

            return TRUE;

        case TSIDE_M_IDE_REFERENCE_LANGUAGE_SYNTAX:
            TSIDE_ActivateReference(TSIDE_REFERENCE_SYNTAX);

            return TRUE;

        case TSIDE_M_IDE_REFERENCE_PLUGIN_FUNCTIONS:
            TSIDE_ActivateReference(TSIDE_REFERENCE_PLUGINS);

            return TRUE;

        case TSIDE_M_IDE_REFERENCE_TEMPLATES:
            TSIDE_ActivateReference(TSIDE_REFERENCE_TEMPLATES);

            return TRUE;

        case TSIDE_M_IDE_RUN_START:
            PrepareForRun(TSINT_CONTROL_RUN, data);

            return TRUE;

        case TSIDE_M_IDE_RUN_DEBUG:
            PrepareForRun(TSINT_CONTROL_STEP_INTO, data);

            return TRUE;

        case TSIDE_M_IDE_RUN_CONTINUE:
            ContinueRun(TSINT_CONTROL_RUN, data);

            return TRUE;

        case TSIDE_M_IDE_RUN_STOP:
            StopRun(data);

            return TRUE;

        case TSIDE_M_IDE_RUN_STEP:
            ContinueRun(TSINT_CONTROL_STEP, data);

            return TRUE;

        case TSIDE_M_IDE_RUN_STEP_INTO:
            ContinueRun(TSINT_CONTROL_STEP_INTO, data);

            return TRUE;

        case TSIDE_M_IDE_RUN_VARIABLES:
            PrintVariables(data);

            return TRUE;

        case TSIDE_M_IDE_HELP_ABOUT:
            DisplayAbout();

            return TRUE;
        }

        return FALSE;

    case WM_NOTIFY:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notify_data = (LPNMHDR)lparam;

        if(
           notify_data->hwndFrom != data->editor_window &&
           notify_data->hwndFrom != tside_run_window
          )
        {
            return FALSE;
        }

        switch(notify_data->code)
        {
        case TSIDE_NOTIFY_DOCUMENT_MODIFIED:
            MarkDocumentModified(data);

            return TRUE;

        case TSIDE_NOTIFY_RUN_COMPILING:
            UpdateRunState(RUN_STATE_COMPILING, data);

            return TRUE;

        case TSIDE_NOTIFY_RUN_EXECUTING:
            UpdateRunState(RUN_STATE_EXECUTING, data);

            return TRUE;

        case TSIDE_NOTIFY_RUN_FINISHED:
            UpdateRunState(RUN_STATE_SHOWN, data);

            return TRUE;

        case TSIDE_NOTIFY_RUN_CLOSED:
            UpdateRunState(RUN_STATE_HIDDEN, data);

            return TRUE;
        }

        return FALSE;

    case TSIDE_MESSAGE_RUN_BEGINNING:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        result = RunBeginning(data);

        SetWindowLongPtr(tside_ide_window, DWL_MSGRESULT, result);

        return result;

    case TSIDE_MESSAGE_RUN_ALERT:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        TSIDE_AnnotateAlert((struct tside_alert*)lparam, data->editor_window);

        return TRUE;

    case TSIDE_MESSAGE_RUN_CLEAR_ALERTS:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        TSIDE_ClearAnnotations(data->editor_window);

        return TRUE;

    case TSIDE_MESSAGE_RUN_GOTO_ALERT:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        TSIDE_GotoAlert((struct tside_alert*)lparam, data->editor_window);

        return TRUE;

    case TSIDE_MESSAGE_RUN_BREAK:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        BreakRun((WCHAR*)lparam, wparam, data);

        return TRUE;

    case TSIDE_MESSAGE_INSERT_TEXT:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        TSIDE_InsertText((char*)lparam, data->editor_window);
        BringWindowToTop(tside_ide_window);

        return TRUE;

    case TSIDE_MESSAGE_USE_TEMPLATE:
        data = (struct ide_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        TSIDE_UseTemplate((char*)lparam, data->editor_window);
        BringWindowToTop(tside_ide_window);

        return TRUE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_IDE_ERROR);

    return FALSE;
}

