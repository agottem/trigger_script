/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSDEF_LEXER_H_
#define _TSDEF_LEXER_H_


#include <stdio.h>

#include "parserutil.h"
#include "lexerutil.h"


extern FILE* yyin;


extern int                     yylex            (struct tsdef_parser_state*);
extern void                    yyrestart        (FILE*);
extern struct yy_buffer_state* yy_scan_string   (const char*);
extern void                    yy_delete_buffer (struct yy_buffer_state*);


#endif

