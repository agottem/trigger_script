/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "paths.h"
#include "error.h"

#include <tsutil/error.h>

#include <shlobj.h>
#include <string.h>
#include <stdlib.h>


char  tside_application_path[MAX_PATH];
WCHAR tside_application_path_w[MAX_PATH];

char  tside_user_plugin_path[MAX_PATH];
WCHAR tside_user_plugin_path_w[MAX_PATH];

char tside_default_units_path[MAX_PATH];
WCHAR tside_default_units_path_w[MAX_PATH];

char tside_user_units_path[MAX_PATH];
WCHAR tside_user_units_path_w[MAX_PATH];

char  tside_syntax_path[MAX_PATH];
WCHAR tside_syntax_path_w[MAX_PATH];

char tside_templates_path[MAX_PATH];
WCHAR tside_templates_path_w[MAX_PATH];

struct tsutil_path_collection tside_plugin_paths;
struct tsutil_path_collection tside_unit_paths;


int TSIDE_InitializePaths (void)
{
    char*   separator;
    char*   constant_env_data;
    char*   env_data;
    HRESULT shell_error;
    DWORD   winapi_error;
    size_t  path_separator_index;
    int     error;

    winapi_error = GetModuleFileNameA(NULL, tside_application_path, MAX_PATH);
    if(winapi_error == 0)
        goto get_application_path_failed;

    separator = strrchr(tside_application_path, '\\');
    if(separator != NULL)
        *separator = 0;

    strcpy(tside_default_units_path, tside_application_path);
    strcat(tside_default_units_path, "\\lib");

    mbstowcs(tside_default_units_path_w, tside_default_units_path, MAX_PATH);

    strcpy(tside_templates_path, tside_application_path);
    strcat(tside_templates_path, "\\templates");

    mbstowcs(tside_templates_path_w, tside_templates_path, MAX_PATH);

    strcpy(tside_syntax_path, tside_application_path);
    strcat(tside_syntax_path, "\\syntax");

    mbstowcs(tside_syntax_path_w, tside_syntax_path, MAX_PATH);

    shell_error = SHGetFolderPathA(
                                   NULL,
                                   CSIDL_PERSONAL,
                                   NULL,
                                   SHGFP_TYPE_CURRENT,
                                   tside_user_plugin_path
                                  );
    if(FAILED(shell_error))
        goto get_shell_folder_path_failed;

    path_separator_index = strlen(tside_user_plugin_path);
    if(tside_user_plugin_path[path_separator_index] != '\\')
    {
        tside_user_plugin_path[path_separator_index] = '\\';
        tside_user_plugin_path[path_separator_index+1] = 0;
    }

    strcat(tside_user_plugin_path, "My Trigger Scripts");

    CreateDirectoryA(tside_user_plugin_path, NULL);

    strcpy(tside_user_units_path, tside_user_plugin_path);
    strcat(tside_user_plugin_path, "\\Plugins");
    strcat(tside_user_units_path, "\\Units");

    CreateDirectoryA(tside_user_plugin_path, NULL);
    CreateDirectoryA(tside_user_units_path, NULL);

    mbstowcs(tside_application_path_w, tside_application_path, MAX_PATH);
    mbstowcs(tside_user_plugin_path_w, tside_user_plugin_path, MAX_PATH);
    mbstowcs(tside_user_units_path_w, tside_user_units_path, MAX_PATH);

    TSUtil_InitializePathCollection(&tside_plugin_paths);
    TSUtil_InitializePathCollection(&tside_unit_paths);

    error = TSUtil_AppendPath(tside_application_path, &tside_plugin_paths);
    if(error != TSUTIL_ERROR_NONE)
        goto append_plugin_path_failed;

    error = TSUtil_AppendPath(tside_user_plugin_path, &tside_plugin_paths);
    if(error != TSUTIL_ERROR_NONE)
        goto append_plugin_path_failed;

    constant_env_data = getenv("ts_plugin");
    if(constant_env_data != NULL)
    {
        char* scan_env;
        char* start_env;
        int   error;

        env_data = strdup(constant_env_data);
        if(env_data == NULL)
            goto duplicate_plugin_env_failed;

        start_env = env_data;

        do
        {
            scan_env = strchr(start_env, ';');
            if(scan_env != NULL)
                *scan_env = 0;

            error = TSUtil_AppendPath(scan_env, &tside_plugin_paths);
            if(error != TSUTIL_ERROR_NONE)
                goto append_env_plugin_path_failed;

            if(scan_env == NULL)
                break;

            scan_env++;
            start_env = scan_env;
        }while(*start_env != 0);

        free(env_data);
    }

    error = TSUtil_AppendPath(tside_default_units_path, &tside_unit_paths);
    if(error != TSUTIL_ERROR_NONE)
        goto append_unit_path_failed;

    error = TSUtil_AppendPath(tside_user_units_path, &tside_unit_paths);
    if(error != TSUTIL_ERROR_NONE)
        goto append_unit_path_failed;

    constant_env_data = getenv("ts_include");
    if(constant_env_data != NULL)
    {
        char* scan_env;
        char* start_env;
        int   error;

        env_data = strdup(constant_env_data);
        if(env_data == NULL)
            goto duplicate_unit_env_failed;

        start_env = env_data;

        do
        {
            scan_env = strchr(start_env, ';');
            if(scan_env != NULL)
                *scan_env = 0;

            error = TSUtil_AppendPath(start_env, &tside_unit_paths);
            if(error != TSUTIL_ERROR_NONE)
                goto env_append_unit_path_failed;

            if(scan_env == NULL)
                break;

            scan_env++;
            start_env = scan_env;
        }while(*start_env != 0);

        free(env_data);
    }

    return TSIDE_ERROR_NONE;

env_append_unit_path_failed:
append_env_plugin_path_failed:
    free(env_data);
append_unit_path_failed:
duplicate_unit_env_failed:
duplicate_plugin_env_failed:
    TSUtil_DestroyPathCollection(&tside_unit_paths);
    TSUtil_DestroyPathCollection(&tside_plugin_paths);
append_plugin_path_failed:
get_shell_folder_path_failed:
get_application_path_failed:
    return TSIDE_ERROR_SYSTEM_CALL_FAILED;
}

void TSIDE_ShutdownPaths   (void)
{
    TSUtil_DestroyPathCollection(&tside_unit_paths);
    TSUtil_DestroyPathCollection(&tside_plugin_paths);
}

