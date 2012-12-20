/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "reference.h"
#include "ref_syntax.h"
#include "ref_plugins.h"
#include "ref_templates.h"
#include "gui.h"
#include "text.h"
#include "main.h"
#include "resources.h"
#include "error.h"

#include <malloc.h>
#include <string.h>
#include <commctrl.h>
#include <commdlg.h>


struct reference_data
{
    HWND reference_window;
    HWND tab_window;
    HWND ref_syntax_window;
    HWND ref_plugins_window;
    HWND ref_templates_window;

    unsigned int current_tab;
};


static int  InitializeDialog   (HWND);
static void DestroyDialog      (struct reference_data*);
static void RepositionControls (struct reference_data*);
static void SetSelectedTab     (unsigned int, struct reference_data*);


static int InitializeDialog (HWND window)
{
    TCITEM                 gui_tab_item;
    struct reference_data* data;
    HWND                   tab_window;
    HWND                   ref_syntax_window;
    HWND                   ref_plugins_window;
    HWND                   ref_templates_window;
    HICON                  icon;

    data = malloc(sizeof(struct reference_data));
    if(data == NULL)
        goto allocate_reference_data_failed;

    icon = LoadIcon(tside_application_instance, MAKEINTRESOURCE(TSIDE_I_GUI_REFERENCE));
    if(icon == NULL)
        goto load_reference_icon_failed;

    SendMessage(window, WM_SETICON, ICON_BIG, (LPARAM)icon);

    tab_window = GetDlgItem(window, TSIDE_C_REFERENCE_TABS);

    ref_syntax_window = CreateDialog(
                                     tside_application_instance,
                                     MAKEINTRESOURCE(TSIDE_D_REF_SYNTAX),
                                     window,
                                     &TSIDE_RefSyntaxMessageProc
                                    );
    if(ref_syntax_window == NULL)
        goto create_ref_syntax_window_failed;

    ref_plugins_window = CreateDialog(
                                      tside_application_instance,
                                      MAKEINTRESOURCE(TSIDE_D_REF_PLUGINS),
                                      window,
                                      &TSIDE_RefPluginsMessageProc
                                     );
    if(ref_plugins_window == NULL)
        goto create_ref_plugins_window_failed;

    ref_templates_window = CreateDialog(
                                        tside_application_instance,
                                        MAKEINTRESOURCE(TSIDE_D_REF_TEMPLATES),
                                        window,
                                        &TSIDE_RefTemplatesMessageProc
                                       );
    if(ref_templates_window == NULL)
        goto create_ref_templates_window_failed;

    gui_tab_item.mask    = TCIF_TEXT;
    gui_tab_item.pszText = TSIDE_GetResourceText(TSIDE_S_REFERENCE_TAB_SYNTAX);

    SendMessage(tab_window, TCM_INSERTITEM, TSIDE_REFERENCE_SYNTAX, (LPARAM)&gui_tab_item);

    gui_tab_item.mask    = TCIF_TEXT;
    gui_tab_item.pszText = TSIDE_GetResourceText(TSIDE_S_REFERENCE_TAB_PLUGINS);

    SendMessage(tab_window, TCM_INSERTITEM, TSIDE_REFERENCE_PLUGINS, (LPARAM)&gui_tab_item);

    gui_tab_item.mask    = TCIF_TEXT;
    gui_tab_item.pszText = TSIDE_GetResourceText(TSIDE_S_REFERENCE_TAB_TEMPLATES);

    SendMessage(tab_window, TCM_INSERTITEM, TSIDE_REFERENCE_TEMPLATES, (LPARAM)&gui_tab_item);

    data->reference_window     = window;
    data->tab_window           = tab_window;
    data->ref_syntax_window    = ref_syntax_window;
    data->ref_plugins_window   = ref_plugins_window;
    data->ref_templates_window = ref_templates_window;

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    RepositionControls(data);
    SetSelectedTab(TSIDE_REFERENCE_PLUGINS, data);

    return TSIDE_ERROR_NONE;

create_ref_templates_window_failed:
    DestroyWindow(ref_plugins_window);
create_ref_plugins_window_failed:
    DestroyWindow(ref_syntax_window);
create_ref_syntax_window_failed:
    DestroyIcon(icon);
load_reference_icon_failed:
    free(data);

allocate_reference_data_failed:
    return TSIDE_ERROR_REFERENCE_ERROR;
}

static void DestroyDialog (struct reference_data* data)
{
    free(data);
}

static void RepositionControls (struct reference_data* data)
{
    POINT client_coordinates;
    RECT  tab_window_area;
    RECT  tab_client_area;
    RECT  tab_control_area;
    RECT  child_client_area;
    HWND  tab_window;

    tab_window = data->tab_window;

    GetClientRect(tab_window, &tab_client_area);

    tab_control_area = tab_client_area;

    SendMessage(tab_window, TCM_ADJUSTRECT, FALSE, (LPARAM)&tab_control_area);
    GetWindowRect(tab_window, &tab_window_area);

    client_coordinates.x = tab_window_area.left;
    client_coordinates.y = tab_window_area.top;

    ScreenToClient(data->reference_window, &client_coordinates);

    child_client_area.left   = client_coordinates.x+tab_control_area.left;
    child_client_area.top    = client_coordinates.y+tab_control_area.top;
    child_client_area.right  = tab_control_area.right-tab_control_area.left;
    child_client_area.bottom = tab_control_area.bottom-tab_control_area.top;

    SetWindowPos(
                 data->ref_syntax_window,
                 NULL,
                 child_client_area.left,
                 child_client_area.top,
                 child_client_area.right,
                 child_client_area.bottom,
                 SWP_NOZORDER
                );

    SetWindowPos(
                 data->ref_plugins_window,
                 NULL,
                 child_client_area.left,
                 child_client_area.top,
                 child_client_area.right,
                 child_client_area.bottom,
                 SWP_NOZORDER
                );

    SetWindowPos(
                 data->ref_templates_window,
                 NULL,
                 child_client_area.left,
                 child_client_area.top,
                 child_client_area.right,
                 child_client_area.bottom,
                 SWP_NOZORDER
                );
}

static void SetSelectedTab (unsigned int selected_tab, struct reference_data* data)
{
    ShowWindow(data->ref_syntax_window, SW_HIDE);
    ShowWindow(data->ref_plugins_window, SW_HIDE);
    ShowWindow(data->ref_templates_window, SW_HIDE);

    switch(selected_tab)
    {
    case TSIDE_REFERENCE_SYNTAX:
        ShowWindow(data->ref_syntax_window, SW_SHOW);

        break;

    case TSIDE_REFERENCE_PLUGINS:
        ShowWindow(data->ref_plugins_window, SW_SHOW);

        break;

    case TSIDE_REFERENCE_TEMPLATES:
        ShowWindow(data->ref_templates_window, SW_SHOW);

        break;
    }

    data->current_tab = selected_tab;

    SendMessage(data->tab_window, TCM_SETCURSEL, selected_tab, 0);
}


int TSIDE_InitializeReference (void)
{
    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownReference (void)
{
}

void TSIDE_ActivateReference (unsigned int selected_tab)
{
    struct reference_data* data;

    data = (struct reference_data*)GetWindowLongPtr(tside_reference_window, GWLP_USERDATA);
    if(data == NULL)
        return;

    SetSelectedTab(selected_tab, data);
    ShowWindow(data->reference_window, SW_SHOWNORMAL);
    BringWindowToTop(data->reference_window);
}

INT_PTR CALLBACK TSIDE_ReferenceMessageProc (
                                             HWND   window,
                                             UINT   message,
                                             WPARAM wparam,
                                             LPARAM lparam
                                            )
{
    struct reference_data* data;
    NMHDR*                 notify_data;
    LRESULT                current_selection;
    int                    error;

    switch(message)
    {
    case WM_INITDIALOG:
        error = InitializeDialog(window);
        if(error != TSIDE_ERROR_NONE)
            goto initialize_dialog_failed;

        return TRUE;

    case WM_CLOSE:
        ShowWindow(tside_reference_window, SW_HIDE);

        return TRUE;

    case WM_DESTROY:
        data = (struct reference_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_NOTIFY:
        data = (struct reference_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notify_data = (LPNMHDR)lparam;

        switch(notify_data->code)
        {
        case TCN_SELCHANGE:
            current_selection = SendMessage(data->tab_window, TCM_GETCURSEL, 0, 0);
            if(current_selection < 0)
                return TRUE;

            SetSelectedTab(current_selection, data);

            return TRUE;
        }
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_REFERENCE_ERROR);

    return FALSE;
}

