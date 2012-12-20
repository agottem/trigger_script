/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "plugins.h"
#include "variables.h"
#include "paths.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <windows.h>


struct tside_registered_plugin* tside_available_plugins;


static int RegisterPlugin (char* path)
{
    char*            search_path;
    char*            directory_separator;
    WIN32_FIND_DATAA find_data;
    HANDLE           find_handle;
    size_t           path_length;
    size_t           alloc_size;

    path_length = strlen(path);

    alloc_size  = path_length+sizeof("\\*.ts.dll")+1;
    search_path = malloc(alloc_size);
    if(search_path == NULL)
        goto alloc_search_path_failed;

    strcpy(search_path, path);
    if(search_path[path_length-1] != '\\')
    {
        directory_separator = &search_path[path_length];
        path_length++;

        strcat(search_path, "\\");
    }
    else
        directory_separator = &search_path[path_length-1];

    strcat(search_path, "*.ts.dll");

    find_handle = FindFirstFileA(search_path, &find_data);
    if(find_handle == NULL)
        goto no_files_found;

    *directory_separator = 0;

    do
    {
        tsffi_register                   register_function;
        tsffi_configure                  configure_function;
        char*                            library_path;
        struct tsffi_registration_group* registration_group;
        struct tside_registered_plugin*  plugin;
        HMODULE                          loaded_library;
        unsigned int                     count;

        alloc_size   = path_length+strlen(find_data.cFileName)+1;
        library_path = malloc(alloc_size);
        if(library_path == NULL)
            goto allocate_library_path_failed;

        strcpy(library_path, search_path);
        strcat(library_path, "\\");
        strcat(library_path, find_data.cFileName);

        loaded_library = LoadLibraryA(library_path);
        if(loaded_library == NULL)
        {
            free(library_path);

            continue;
        }

        register_function = (tsffi_register)GetProcAddress(
                                                           loaded_library,
                                                           TSFFI_REGISTER_FUNCTION_NAME
                                                          );
        if(register_function == NULL)
        {
            free(library_path);
            FreeLibrary(loaded_library);

            continue;
        }

        register_function(&registration_group, &count);
        if(count == 0)
        {
            free(library_path);
            FreeLibrary(loaded_library);

            continue;
        }

        configure_function = (tsffi_configure)GetProcAddress(
                                                             loaded_library,
                                                             TSFFI_CONFIGURE_FUNCTION_NAME
                                                            );
        if(configure_function != NULL)
        {
            struct tside_variable* variable;

            for(
                variable = tside_set_variables;
                variable != NULL;
                variable = variable->next_variable
               )
            {
                configure_function(variable->name, variable->value);
            }
        }


        plugin = malloc(sizeof(struct tside_registered_plugin));
        if(plugin == NULL)
            goto allocate_plugin_failed;

        plugin->path               = library_path;
        plugin->register_function  = register_function;
        plugin->configure_function = configure_function;
        plugin->groups             = registration_group;
        plugin->count              = count;

        plugin->next_plugin     = tside_available_plugins;
        tside_available_plugins = plugin;
    }while(FindNextFileA(find_handle, &find_data) != 0);

    FindClose(find_handle);

no_files_found:
    free(search_path);

    return TSIDE_ERROR_NONE;

allocate_plugin_failed:
allocate_library_path_failed:
    free(search_path);
alloc_search_path_failed:
    return TSIDE_ERROR_SYSTEM_CALL_FAILED;
}


int TSIDE_InitializePlugins (void)
{
    struct tsutil_path* path;
    int                 error;

    tside_available_plugins = NULL;

    for(path = tside_plugin_paths.first_path; path != NULL; path = path->next_path)
    {
        error = RegisterPlugin(path->path);
        if(error != TSIDE_ERROR_NONE)
            goto register_plugin_failed;
    }

    return TSIDE_ERROR_NONE;

register_plugin_failed:
    TSIDE_ShutdownPlugins();

    return error;
}

void TSIDE_ShutdownPlugins (void)
{
    struct tside_registered_plugin* plugins;

    plugins = tside_available_plugins;
    while(plugins != NULL)
    {
        struct tside_registered_plugin* free_plugin;

        free_plugin = plugins;
        plugins     = plugins->next_plugin;

        free(free_plugin->path);
        free(free_plugin);
    }
}

