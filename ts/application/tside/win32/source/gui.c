/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "gui.h"
#include "main.h"
#include "resources.h"
#include "ide.h"
#include "select.h"
#include "editor.h"
#include "search_text.h"
#include "search_goto.h"
#include "run.h"
#include "reference.h"
#include "ref_syntax.h"
#include "ref_plugins.h"
#include "ref_templates.h"
#include "scintilla.h"
#include "error.h"

#include <commctrl.h>
#include <shellapi.h>

#include <scintilla/Scintilla.h>


HWND tside_ide_window;
HWND tside_run_window;
HWND tside_reference_window;

static HACCEL gui_accelerators;


int TSIDE_InitializeGUI (void)
{
    INITCOMMONCONTROLSEX common_controls;
    BOOL                 error;

    common_controls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    common_controls.dwICC  = ICC_LISTVIEW_CLASSES|
                             ICC_PROGRESS_CLASS|
                             ICC_STANDARD_CLASSES|
                             ICC_TREEVIEW_CLASSES|
                             ICC_BAR_CLASSES;

    error = InitCommonControlsEx(&common_controls);
    if(error == FALSE)
        goto initialize_controls_failed;

    error = Scintilla_RegisterClasses(tside_application_instance);
    if(error == 0)
        goto initialize_scintilla_classes_failed;

    error = TSIDE_InitializeScintilla();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_scintilla_failed;

    error = TSIDE_InitializeSearchText();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_search_text_failed;

    error = TSIDE_InitializeSearchGoto();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_search_goto_failed;

    error = TSIDE_InitializeRefPlugins();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_ref_plugins_failed;

    error = TSIDE_InitializeRefTemplates();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_ref_templates_failed;

    error = TSIDE_InitializeRefSyntax();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_ref_syntax_failed;

    error = TSIDE_InitializeEditor();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_editor_failed;

    error = TSIDE_InitializeSelect();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_select_failed;

    error = TSIDE_InitializeRun();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_run_failed;

    error = TSIDE_InitializeReference();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_reference_failed;

    error = TSIDE_InitializeIDE();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_ide_failed;

    tside_ide_window = CreateDialog(
                                    tside_application_instance,
                                    MAKEINTRESOURCE(TSIDE_D_IDE),
                                    NULL,
                                    &TSIDE_IDEMessageProc
                                   );
    if(tside_ide_window == NULL)
        goto create_ide_window_failed;

    tside_run_window = CreateDialog(
                                    tside_application_instance,
                                    MAKEINTRESOURCE(TSIDE_D_RUN),
                                    NULL,
                                    &TSIDE_RunMessageProc
                                   );
    if(tside_run_window == NULL)
        goto create_run_window_failed;

    tside_reference_window = CreateDialog(
                                          tside_application_instance,
                                          MAKEINTRESOURCE(TSIDE_D_REFERENCE),
                                          NULL,
                                          &TSIDE_ReferenceMessageProc
                                         );
    if(tside_reference_window == NULL)
        goto create_reference_window_failed;

    gui_accelerators = LoadAccelerators(tside_application_instance, MAKEINTRESOURCE(TSIDE_A_GUI));
    if(gui_accelerators == NULL)
        goto load_gui_accelerators_failed;

    return TSIDE_ERROR_NONE;

load_gui_accelerators_failed:
    DestroyWindow(tside_reference_window);
create_reference_window_failed:
    DestroyWindow(tside_run_window);
create_run_window_failed:
    DestroyWindow(tside_ide_window);
create_ide_window_failed:
    TSIDE_ShutdownIDE();
initialize_ide_failed:
    TSIDE_ShutdownReference();
initialize_reference_failed:
    TSIDE_ShutdownRun();
initialize_run_failed:
    TSIDE_ShutdownSelect();
initialize_select_failed:
    TSIDE_ShutdownEditor();
initialize_editor_failed:
    TSIDE_ShutdownRefSyntax();
initialize_ref_syntax_failed:
    TSIDE_ShutdownRefTemplates();
initialize_ref_templates_failed:
    TSIDE_ShutdownRefPlugins();
initialize_ref_plugins_failed:
    TSIDE_ShutdownSearchGoto();
initialize_search_goto_failed:
    TSIDE_ShutdownSearchText();
initialize_search_text_failed:
    TSIDE_ShutdownScintilla();

initialize_scintilla_failed:
initialize_scintilla_classes_failed:
initialize_controls_failed:
    return TSIDE_ERROR_SYSTEM_CALL_FAILED;
}

void TSIDE_ShutdownGUI (void)
{
    DestroyAcceleratorTable(gui_accelerators);
    DestroyWindow(tside_run_window);
    DestroyWindow(tside_reference_window);
    DestroyWindow(tside_ide_window);
    TSIDE_ShutdownIDE();
    TSIDE_ShutdownReference();
    TSIDE_ShutdownRun();
    TSIDE_ShutdownSelect();
    TSIDE_ShutdownEditor();
    TSIDE_ShutdownRefSyntax();
    TSIDE_ShutdownRefTemplates();
    TSIDE_ShutdownRefPlugins();
    TSIDE_ShutdownSearchGoto();
    TSIDE_ShutdownSearchText();
    TSIDE_ShutdownScintilla();
}

int TSIDE_GUIMain (void)
{
    MSG message;

    TSIDE_PresentIDE();

    while(GetMessage(&message, NULL, 0, 0) > 0)
    {
        WCHAR class_name[sizeof("Scintilla")+1];
        int   accelerator_result;
        int   delta;
        HWND  parent_window;
        HWND  root_window;
        BOOL  error;

        accelerator_result = TranslateAccelerator(tside_ide_window, gui_accelerators, &message);
        if(accelerator_result != 0)
            continue;

        GetClassName(message.hwnd, class_name, sizeof("Scintilla")+1);

        delta = wcscmp(L"Scintilla", class_name);
        if(delta == 0)
        {
            if(message.message == WM_CHAR && (message.wParam < 0x20 || message.wParam >= 0x7F))
                continue;
        }

        parent_window = message.hwnd;

        do
        {
            root_window   = parent_window;
            parent_window = GetParent(root_window);
        }while(parent_window != NULL);

        error = IsDialogMessage(root_window, &message);
        if(error == FALSE)
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return message.wParam;
}

