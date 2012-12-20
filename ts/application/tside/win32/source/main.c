/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "main.h"
#include "paths.h"
#include "variables.h"
#include "plugins.h"
#include "gui.h"
#include "error.h"


HINSTANCE tside_application_instance;

#include <crtdbg.h>
int WINAPI WinMain (
                    HINSTANCE application_instance,
                    HINSTANCE previous_instance,
                    LPSTR     command_line,
                    int       show
                   )
{
    int error;

    tside_application_instance = application_instance;

    error = TSIDE_InitializePaths();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_paths_failed;

    error = TSIDE_InitializeVariables();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_variables_failed;

    error = TSIDE_InitializePlugins();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_plugins_failed;

    error = TSIDE_InitializeGUI();
    if(error != TSIDE_ERROR_NONE)
        goto initialize_gui_failed;

    error = TSIDE_GUIMain();
    if(error != TSIDE_ERROR_NONE)
        goto main_failed;

    TSIDE_ShutdownGUI();
    TSIDE_ShutdownPlugins();
    TSIDE_ShutdownVariables();
    TSIDE_ShutdownPaths();

    return error;

main_failed:
    TSIDE_ShutdownGUI();
initialize_gui_failed:
    TSIDE_ShutdownPlugins();
initialize_plugins_failed:
    TSIDE_ShutdownVariables();
initialize_variables_failed:
    TSIDE_ShutdownPaths();
initialize_paths_failed:
    error = MessageBoxA(
                        NULL,
                        "TS IDE encountered a critical error.  The application will now exit.",
                        "TS IDE Error",
                        MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_SERVICE_NOTIFICATION
                       );

    return error;
}

