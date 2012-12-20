/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "lexerutil.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>


char* TSDef_TranslateStringLiteral (char* raw_literal)
{
    char*        translated_literal;
    char*        insertion_position;
    size_t       raw_length;
    char         current_character;

    raw_length = strlen(raw_literal)-2;

    translated_literal = malloc(raw_length+1);
    if(translated_literal == NULL)
        return NULL;

    raw_literal++;

    insertion_position = translated_literal;
    for(
        current_character = *raw_literal;
        current_character != '"';
        current_character = *raw_literal
       )
    {
        if(current_character == '\\')
        {
            raw_literal++;

            current_character = *raw_literal;

            switch(current_character)
            {
            case 'n':
                *insertion_position = '\n';

                break;

            case 'a':
                *insertion_position = '\a';

                break;

            default:
                *insertion_position = current_character;

                break;
            }
        }
        else
            *insertion_position = current_character;

        insertion_position++;
        raw_literal++;
    }

    *insertion_position = 0;

    return translated_literal;
}

