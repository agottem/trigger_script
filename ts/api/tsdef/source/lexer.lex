/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

%{
    #include "lexer.h"

    #include <tsdef/def.h>

    #include "tokens.h"

    #include <stdlib.h>
    #include <string.h>


    #define YY_DECL int yylex (struct tsdef_parser_state* state)
%}

%option noyywrap


digit      [0-9]
letter     [a-zA-Z_]
whitespace [ \t]
eol        ((\r)|(\n)|(\r\n))


%%


"if"                            {return TOKEN_IF;}
"else"                          {return TOKEN_ELSE;}
"elseif"                        {return TOKEN_ELSEIF;}
"end"                           {return TOKEN_END;}

"loop"                          {return TOKEN_LOOP;}
"while"                         {return TOKEN_WHILE;}
"for"                           {return TOKEN_FOR;}
"to"                            {return TOKEN_TO;}
"downto"                        {return TOKEN_DOWNTO;}
"continue"                      {return TOKEN_CONTINUE;}
"break"                         {return TOKEN_BREAK;}
"finish"                        {return TOKEN_FINISH;}

"and"                           {return TOKEN_LOGICAL_AND;}
"or"                            {return TOKEN_LOGICAL_OR;}
"not"                           {return TOKEN_LOGICAL_NOT;}

"input"                         {return TOKEN_INPUT;}
"output"                        {return TOKEN_OUTPUT;}

"action"                        {return TOKEN_ACTION;}

"true"                          {yylval.bool_val = TSDEF_BOOL_TRUE; return TOKEN_BOOL;}
"false"                         {yylval.bool_val = TSDEF_BOOL_FALSE; return TOKEN_BOOL;}
{letter}({letter}|{digit})*     {yylval.text_val = strdup(yytext); return TOKEN_IDENTIFIER;}
{digit}+                        {yylval.int_val = (tsdef_int)atoi(yytext); return TOKEN_INT;}
{digit}+"."{digit}*             {yylval.real_val = (tsdef_real)atof(yytext); return TOKEN_REAL;}
"\""(\\.|[^\"])*"\""            {yylval.text_val = TSDef_TranslateStringLiteral(yytext); return TOKEN_STRING;}

"\\"{whitespace}*{eol}          {state->current_line_number++;}
{eol}                           {state->current_line_number++; return TOKEN_NEW_LINE;}

","                             {return TOKEN_COMMA;}
"="                             {return TOKEN_EQUALS;}
"("                             {return TOKEN_L_PAREN;}
")"                             {return TOKEN_R_PAREN;}

"=="                            {return TOKEN_COMPARISON_EQUAL;}
"~="                            {return TOKEN_COMPARISON_NOT_EQUAL;}
">"                             {return TOKEN_COMPARISON_GREATER;}
">="                            {return TOKEN_COMPARISON_GREATER_EQUAL;}
"<"                             {return TOKEN_COMPARISON_LESS;}
"<="                            {return TOKEN_COMPARISON_LESS_EQUAL;}

"+"                             {return TOKEN_ARITHMETIC_ADD;}
"-"                             {return TOKEN_ARITHMETIC_SUB;}
"*"                             {return TOKEN_ARITHMETIC_MUL;}
"/"                             {return TOKEN_ARITHMETIC_DIV;}
"^"                             {return TOKEN_ARITHMETIC_POW;}
"%"                             {return TOKEN_ARITHMETIC_MOD;}

{whitespace}*                   {}
"#".*                           {}

.                               {return LEXICAL_ERROR;}

<<EOF>>                         {
                                    static int handle_eof = 0;

                                    if(handle_eof == 0)
                                    {
                                        state->current_line_number++;
                                        handle_eof++;

                                        return TOKEN_NEW_LINE;
                                    }
                                    else
                                    {
                                        handle_eof = 0;

                                        yyterminate();
                                    }
                                }
