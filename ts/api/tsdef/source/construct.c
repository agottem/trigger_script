/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsdef/construct.h>
#include <tsdef/error.h>

#include "parserutil.h"
#include "parser.h"
#include "lexer.h"


static int ParseInput (char*, struct tsdef_unit*, struct tsdef_def_error_list*);


static int ParseInput (char* name, struct tsdef_unit* unit, struct tsdef_def_error_list* errors)
{
    struct tsdef_parser_state state;
    int                       error;

    error = TSDef_InitializeUnit(name, unit);
    if(error != TSDEF_ERROR_NONE)
        return error;

    state.unit = unit;

    state.current_block       = &unit->global_block;
    state.current_line_number = 1;
    state.error_list          = errors;
    state.error_count         = 0;
    state.warning_count       = 0;

    error = yyparse(&state);

    if(error != 0 || state.error_count != 0)
        return TSDEF_ERROR_CONSTRUCT_ERROR;
    else if(state.warning_count != 0)
        return TSDEF_ERROR_CONSTRUCT_WARNING;

    return TSDEF_ERROR_NONE;
}


int TSDef_ConstructUnitFromFile (
                                 char*                        file,
                                 char*                        name,
                                 struct tsdef_unit*           unit,
                                 struct tsdef_def_error_list* errors
                                )
{
    int error;

    yyin = fopen(file, "rt");
    if(yyin == NULL)
        return TSDEF_ERROR_FILE_OPEN;

    yyrestart(yyin);

    error = ParseInput(name, unit, errors);

    fclose(yyin);

    return error;
}

int TSDef_ConstructUnitFromString (
                                   char*                        program_string,
                                   char*                        name,
                                   struct tsdef_unit*           unit,
                                   struct tsdef_def_error_list* errors
                                  )
{
    struct yy_buffer_state* lex_buffer;
    int                     error;

    lex_buffer = yy_scan_string(program_string);
    if(lex_buffer == NULL)
        return TSDEF_ERROR_MEMORY;

    error = ParseInput(name, unit, errors);

    yy_delete_buffer(lex_buffer);

    return error;
}

