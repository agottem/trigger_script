/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "text.h"
#include "main.h"


WCHAR* TSIDE_GetResourceText (UINT resource_id)
{
    HGLOBAL loaded_handle;
    HRSRC   resource_handle;
    WCHAR*  resource_string;
    UINT    transformed_resource_id;
    UINT    string_table_offset;

    transformed_resource_id = resource_id/16+1;

    resource_handle = FindResourceEx(
                                     tside_application_instance,
                                     RT_STRING,
                                     MAKEINTRESOURCE(transformed_resource_id),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
                                    );
    if(resource_handle == NULL)
        goto find_resource_failed;

    loaded_handle = LoadResource(tside_application_instance, resource_handle);
    if(loaded_handle == NULL)
        goto load_resource_failed;

    resource_string = (WCHAR*)LockResource(loaded_handle);
    if(resource_string == NULL)
        goto lock_resource_failed;

    string_table_offset = resource_id%16;
    while(string_table_offset--)
        resource_string += 1+(UINT)*resource_string;

    resource_string++;

    return resource_string;

lock_resource_failed:
    FreeResource(loaded_handle);

load_resource_failed:
find_resource_failed:
    return NULL;
}
