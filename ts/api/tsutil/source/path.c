/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsutil/path.h>
#include <tsutil/error.h>

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>


void TSUtil_InitializePathCollection (struct tsutil_path_collection* collection)
{
    collection->first_path = NULL;
    collection->last_path  = NULL;
    collection->max_path_length = 0;
}

void TSUtil_DestroyPathCollection (struct tsutil_path_collection* collection)
{
    struct tsutil_path* path;

    path = collection->first_path;
    while(path != NULL)
    {
        struct tsutil_path* free_path;

        free(path->path);

        free_path = path;
        path      = path->next_path;

        free(free_path);
    }
}

int TSUtil_AppendPath (char* path_name, struct tsutil_path_collection* collection)
{
    struct tsutil_path* path;
    size_t              path_length;

    path = malloc(sizeof(struct tsutil_path));
    if(path == NULL)
        goto allocated_path_failed;

    path->path = strdup(path_name);
    if(path->path == NULL)
        goto duplicate_path_name_failed;

    path->next_path = NULL;
    if(collection->last_path != NULL)
        collection->last_path->next_path = path;
    else
        collection->first_path = path;

    collection->last_path = path;

    path_length = strlen(path_name);
    if(path_length > collection->max_path_length)
        collection->max_path_length = path_length;

    return TSUTIL_ERROR_NONE;

duplicate_path_name_failed:
    free(path);

allocated_path_failed:
    return TSUTIL_ERROR_MEMORY;
}

int TSUtil_PrependPath (char* path_name, struct tsutil_path_collection* collection)
{
    struct tsutil_path* path;
    size_t              path_length;

    path = malloc(sizeof(struct tsutil_path));
    if(path == NULL)
        goto allocated_path_failed;

    path->path = strdup(path_name);
    if(path->path == NULL)
        goto duplicate_path_name_failed;

    path->next_path = collection->first_path;
    if(collection->first_path == NULL)
        collection->last_path = path;

    collection->first_path = path;

    path_length = strlen(path_name);

    if(path_name[path_length-1] == '/')
    {
        path_length--;
        path_name[path_length] = 0;
    }

    if(path_length > collection->max_path_length)
        collection->max_path_length = path_length;

    return TSUTIL_ERROR_NONE;

duplicate_path_name_failed:
    free(path);

allocated_path_failed:
    return TSUTIL_ERROR_MEMORY;
}

int TSUtil_FindUnitFile (
                         char*                          name,
                         char*                          extension,
                         struct tsutil_path_collection* path_collection,
                         char**                         found_path
                        )
{
    struct stat         stat_data;
    char*               search_path;
    struct tsutil_path* path;
    size_t              file_name_length;
    int                 result;

    file_name_length = sizeof("/")+strlen(name)+strlen(extension)+sizeof("/")+1;

    search_path = malloc(path_collection->max_path_length+file_name_length);
    if(search_path == NULL)
        return TSUTIL_ERROR_MEMORY;

    for(path = path_collection->first_path; path != NULL; path = path->next_path)
    {
        strcpy(search_path, path->path);
        strcat(search_path, "/");
        strcat(search_path, name);
        strcat(search_path, extension);

        result = stat(search_path, &stat_data);
        if(result == 0)
        {
            *found_path = search_path;

            return TSUTIL_ERROR_NONE;
        }
    }

    free(search_path);

    return TSUTIL_ERROR_NOT_FOUND;
}

