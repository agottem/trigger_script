/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "register.h"
#include "main.h"
#include "error.h"

#include <tsdef/error.h>
#include <tsffi/register.h>

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>


int TSI_RegisterFFI (char* path, struct tsdef_module* module)
{
    char*           search_path;
    char*           directory_separator;
    WIN32_FIND_DATA find_data;
    HANDLE          find_handle;
    size_t          path_length;
    size_t          alloc_size;

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

    find_handle = FindFirstFile(search_path, &find_data);
    if(find_handle == NULL)
        goto no_files_found;

    *directory_separator = 0;

    do
    {
        tsffi_register                   register_function;
        tsffi_configure                  configure_function;
        struct tsi_variable*             variable;
        char*                            library_path;
        struct tsffi_registration_group* registration_group;
        HMODULE                          loaded_library;
        unsigned int                     count;

        alloc_size   = path_length+strlen(find_data.cFileName)+1;
        library_path = malloc(alloc_size);
        if(library_path == NULL)
            goto allocate_library_path_failed;

        strcpy(library_path, search_path);
        strcat(library_path, "\\");
        strcat(library_path, find_data.cFileName);

        loaded_library = LoadLibrary(library_path);

        free(library_path);

        if(loaded_library == NULL)
            continue;

        register_function = (tsffi_register)GetProcAddress(
                                                           loaded_library,
                                                           TSFFI_REGISTER_FUNCTION_NAME
                                                          );
        if(register_function == NULL)
        {
            FreeLibrary(loaded_library);

            continue;
        }

        printf("Found TS plugin: %s\n", find_data.cFileName);

        register_function(&registration_group, &count);
        if(count == 0)
        {
            FreeLibrary(loaded_library);

            continue;
        }

        while(count--)
        {
            int error;

            error = TSDef_AddFFIGroup(find_data.cFileName, &registration_group[count], module);
            if(error != TSDEF_ERROR_NONE)
                goto register_ffi_group_failed;
        }

        configure_function = (tsffi_configure)GetProcAddress(
                                                            loaded_library,
                                                            TSFFI_CONFIGURE_FUNCTION_NAME
                                                           );
        if(configure_function == NULL)
            continue;

        for(variable = tsi_set_variables; variable != NULL; variable = variable->next_variable)
            configure_function(variable->name, variable->value);
    }while(FindNextFile(find_handle, &find_data) != 0);

    FindClose(find_handle);

no_files_found:
    free(search_path);

    return TSI_ERROR_NONE;

register_ffi_group_failed:
allocate_library_path_failed:
    free(search_path);
alloc_search_path_failed:
    return TSI_ERROR_FAILURE;
}

