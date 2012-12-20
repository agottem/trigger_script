/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "context.h"
#include "main.h"

#include <tsutil/error.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>


#define MAX_LINE_LENGTH 4095


void TSI_DisplayContext (
                         char*        unit,
                         unsigned int line,
                         unsigned int context_lines,
                         unsigned int padding
                        )
{
    char         buffer[MAX_LINE_LENGTH+1];
    FILE*        file;
    char*        file_name;
    char*        line_text;
    unsigned int start_line;
    unsigned int end_line;
    unsigned int current_line;
    int          error;

    error = TSUtil_FindUnitFile(unit, TSI_SOURCE_EXTENSION, &tsi_search_paths, &file_name);
    if(error != TSUTIL_ERROR_NONE)
    {
        printf("WARNING: Could not print file context\n");

        goto unit_file_name_failed;
    }

    file = fopen(file_name, "rt");
    if(file == NULL)
        goto open_file_failed;

    if(context_lines > line)
        start_line = 0;
    else
        start_line = line-context_lines;

    end_line = line+context_lines;

    for(current_line = 1; current_line < start_line; current_line++)
    {
        line_text = fgets(buffer, MAX_LINE_LENGTH+1, file);
        if(line_text == NULL || strlen(line_text) >= MAX_LINE_LENGTH)
            goto end_of_file_reached;
    }

    while(current_line <= end_line)
    {
        unsigned int index;
        size_t       line_length;

        line_text = fgets(buffer, MAX_LINE_LENGTH+1, file);
        if(line_text == NULL)
            goto end_of_file_reached;

        line_length = strlen(line_text);
        if(line_length >= MAX_LINE_LENGTH)
            goto end_of_file_reached;

        for(index = 0; index < padding; index++)
            putchar(' ');

        if(current_line == line)
            printf("> ");
        else
            printf("  ");

        while(line_length--)
        {
            putchar(*line_text);

            line_text++;
        }

        current_line++;
    }

end_of_file_reached:
    fclose(file);
open_file_failed:
    free(file_name);

unit_file_name_failed:
    return;
}

