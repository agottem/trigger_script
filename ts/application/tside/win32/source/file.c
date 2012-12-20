/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "file.h"
#include "error.h"

#include <stdio.h>
#include <malloc.h>


int TSIDE_GetFileText (WCHAR* file_name, char** file_content)
{
    FILE* open_file;
    char* text;
    long  size;

    open_file = _wfopen(file_name, L"rb");
    if(open_file == NULL)
        goto open_file_failed;

    fseek(open_file, 0, SEEK_END);

    size = ftell(open_file);
    if(size < 0)
        goto get_size_failed;

    fseek(open_file, 0, SEEK_SET);

    text = malloc(size+1);
    if(text == NULL)
        goto alloc_text_failed;

    if(size != 0)
    {
        size_t read_size;

        read_size = fread(text, size, 1, open_file);
        if(read_size != 1)
            goto read_text_failed;
    }

    fclose(open_file);

    text[size] = 0;

    *file_content = text;

    return TSIDE_ERROR_NONE;

read_text_failed:
    free(text);
alloc_text_failed:
get_size_failed:
    fclose(open_file);

open_file_failed:
    return TSIDE_ERROR_FILE_IO;
}

int TSIDE_SaveFileText (WCHAR* file_name, char* file_content)
{
    FILE*  open_file;
    size_t size;

    open_file = _wfopen(file_name, L"wb");
    if(open_file == NULL)
        goto open_file_failed;

    size = strlen(file_content);

    if(size != 0)
    {
        size_t write_size;

        write_size = fwrite(file_content, size, 1, open_file);
        if(write_size != 1)
            goto write_text_failed;
    }

    fclose(open_file);

    return TSIDE_ERROR_NONE;

write_text_failed:
    fclose(open_file);

open_file_failed:
    return TSIDE_ERROR_FILE_IO;
}

