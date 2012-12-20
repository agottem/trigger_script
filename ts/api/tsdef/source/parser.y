/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

%{
    #include "parser.h"
    #include "lexer.h"

    #include <tsdef/def.h>

    #include <stdio.h>


    #define YYLEX_PARAM state

    #define VERIFY(op) do                                           \
                       {                                            \
                           int error;                               \
                                                                    \
                           error = (op);                            \
                                                                    \
                           if(error == TSDEF_UTIL_ABORT_STATEMENT)  \
                           {                                        \
                               YYERROR;                             \
                           }                                        \
                           else if(error == TSDEF_UTIL_ABORT_PARSE) \
                           {                                        \
                               YYABORT;                             \
                           }                                        \
                       }while(0)
%}

%start input
%parse-param {struct tsdef_parser_state* state}

%union
{
    tsdef_bool   bool_val;
    char*        text_val;
    tsdef_int    int_val;
    unsigned int uint_val;
    tsdef_real   real_val;
    tsdef_string string_val;


    struct
    {
        unsigned int      flags_val;
        struct tsdef_exp* exp_val;
    }logical_sub_exp_val;

    struct tsdef_exp_list*           exp_list_val;
    struct tsdef_exp*                exp_val;
    struct tsdef_assignment*         assignment_val;
    struct tsdef_logical_exp*        logical_exp_val;
    struct tsdef_comparison_exp*     comparison_exp_val;
    struct tsdef_primary_exp*        primary_exp_val;
    struct tsdef_exp_value_type*     exp_value_type_val;
    struct tsdef_function_call_list* function_call_list_val;
    struct tsdef_function_call*      function_call_val;
    struct tsdef_variable_list*      variable_list_val;
    struct tsdef_variable_reference* variable_val;
}

%type <exp_list_val>           exp_list
%type <exp_val>                exp
%type <assignment_val>         assignment_exp
%type <uint_val>               for_loop_direction
%type <logical_exp_val>        logical_exp
%type <uint_val>               logical_exp_op
%type <logical_sub_exp_val>    logical_not_sub_exp
%type <logical_sub_exp_val>    logical_sub_exp
%type <uint_val>               logical_not_qualifier
%type <comparison_exp_val>     comparison_exp
%type <uint_val>               comparison_exp_op
%type <primary_exp_val>        primary_exp
%type <primary_exp_val>        base_primary_exp
%type <uint_val>               primary_exp_op
%type <exp_value_type_val>     exp_value_type
%type <function_call_list_val> function_call_list
%type <function_call_val>      function_call
%type <variable_list_val>      variable_list
%type <variable_val>           variable

%token TOKEN_IF
%token TOKEN_ELSE
%token TOKEN_ELSEIF
%token TOKEN_END

%token TOKEN_LOOP
%token TOKEN_WHILE
%token TOKEN_FOR
%token TOKEN_TO
%token TOKEN_DOWNTO
%token TOKEN_CONTINUE
%token TOKEN_BREAK
%token TOKEN_FINISH

%token TOKEN_LOGICAL_AND
%token TOKEN_LOGICAL_OR
%token TOKEN_LOGICAL_NOT

%token TOKEN_INPUT
%token TOKEN_OUTPUT

%token TOKEN_ACTION

%token <bool_val>   TOKEN_BOOL
%token <text_val>   TOKEN_IDENTIFIER
%token <int_val>    TOKEN_INT
%token <real_val>   TOKEN_REAL
%token <string_val> TOKEN_STRING

%token TOKEN_NEW_LINE
%token TOKEN_COMMA
%token TOKEN_EQUALS
%token TOKEN_L_PAREN
%token TOKEN_R_PAREN

%token TOKEN_COMPARISON_EQUAL
%token TOKEN_COMPARISON_NOT_EQUAL
%token TOKEN_COMPARISON_GREATER
%token TOKEN_COMPARISON_GREATER_EQUAL
%token TOKEN_COMPARISON_LESS
%token TOKEN_COMPARISON_LESS_EQUAL

%token TOKEN_ARITHMETIC_ADD
%token TOKEN_ARITHMETIC_SUB
%token TOKEN_ARITHMETIC_MUL
%token TOKEN_ARITHMETIC_DIV
%token TOKEN_ARITHMETIC_POW
%token TOKEN_ARITHMETIC_MOD

%token LEXICAL_ERROR

%destructor {}                                    <bool_val>
%destructor {free($$);}                           <text_val>
%destructor {}                                    <int_val>
%destructor {}                                    <uint_val>
%destructor {}                                    <real_val>
%destructor {free($$);}                           <string_val>
%destructor {TSDef_DestroyExpList($$);}           <exp_list_val>
%destructor {TSDef_DestroyExp($$);}               <exp_val>
%destructor {TSDef_DestroyAssignment($$);}        <assignment_val>
%destructor {TSDef_DestroyLogicalExp($$);}        <logical_exp>
%destructor {TSDef_DestroyExp($$.exp_val);}       <logical_sub_exp>
%destructor {TSDef_DestroyComparisonExp($$);}     <comparison_exp_val>
%destructor {TSDef_DestroyPrimaryExp($$);}        <primary_exp_val>
%destructor {TSDef_DestroyExpValueType($$);}      <exp_value_type_val>
%destructor {TSDef_DestroyFunctionCallList($$);}  <function_call_list_val>
%destructor {TSDef_DestroyFunctionCall($$);}      <function_call_val>
%destructor {TSDef_DestroyVariableList($$);}      <variable_list_val>
%destructor {TSDef_DestroyVariableReference($$);} <variable_val>


%%


input:   header_input header_output unit_definition
       | header_input unit_definition
       | header_output unit_definition
       | unit_definition
       ;


header_input:   TOKEN_INPUT variable_list separator {VERIFY(TSDef_Parser_Input($2, state));}
              | separator header_input
              ;

header_output:   TOKEN_OUTPUT assignment_exp separator {VERIFY(TSDef_Parser_Output($2, state));}
               | separator header_output
               ;


unit_definition:   statement_list
                 | statement_list action_list
                 ;


action_list:   action
             | action separator_list
             | action action_list
             | action separator_list action_list
             ;


action: action_start statement_list end_action
      ;

action_start: TOKEN_ACTION function_call_list separator {VERIFY(TSDef_Parser_Action($2, state));}
            ;

end_action: TOKEN_END separator {TSDef_Parser_EndAction(state);}
          ;


statement_list: /* empty */
                | statement statement_list
                | separator statement_list
                ;

statement:   function_call_statement
           | assignment
           | if_statement
           | loop
           | loop_flow_control
           | finish
           | error separator   {TSDef_Parser_ProcessError(state); yyerrok;}
           ;


function_call_statement: function_call separator {VERIFY(TSDef_Parser_AddFunctionCall($1, state));}
                         ;


assignment: assignment_exp separator {VERIFY(TSDef_Parser_AddAssignment($1, state));}
            ;

assignment_exp: variable TOKEN_EQUALS exp {VERIFY(TSDef_Parser_Assignment($1, $3, state, &$$));}
                ;


if_statement:   if_statement_start end_block
              | if_statement_start if_statement_elseif end_block
              ;

if_statement_start: if_statement_start_exp statement_list
                    ;

if_statement_start_exp: TOKEN_IF exp separator {VERIFY(TSDef_Parser_AddIfStatement($2, 0, state));}
                        ;

if_statement_elseif:   if_statement_else
                     | if_statement_elseif_exp statement_list
                     | if_statement_elseif_exp statement_list if_statement_elseif
                     ;

if_statement_elseif_exp: TOKEN_ELSEIF exp separator {TSDef_Parser_EndBlock(state); VERIFY(TSDef_Parser_AddIfStatement($2, TSDEF_IF_STATEMENT_FLAG_ELSE, state));}
                         ;

if_statement_else: if_statement_else_exp statement_list
                   ;

if_statement_else_exp: TOKEN_ELSE separator {TSDef_Parser_EndBlock(state); VERIFY(TSDef_Parser_AddIfStatement(NULL, TSDEF_IF_STATEMENT_FLAG_ELSE, state));}
                       ;


loop:   infinite_loop
      | while_loop
      | for_loop
      ;

infinite_loop: infinite_loop_exp statement_list end_block
               ;

infinite_loop_exp: TOKEN_LOOP separator {VERIFY(TSDef_Parser_AddLoop(state, NULL));}
                   ;

while_loop: while_loop_exp statement_list end_block
            ;

while_loop_exp: TOKEN_WHILE exp separator {VERIFY(TSDef_Parser_AddWhileLoop($2, state));}
                ;

for_loop:  for_loop_exp statement_list end_block
           ;

for_loop_exp:   TOKEN_FOR variable       for_loop_direction exp separator {VERIFY(TSDef_Parser_AddForLoop(NULL,    $2, $4, $3, state));}
              | TOKEN_FOR assignment_exp for_loop_direction exp separator {VERIFY(TSDef_Parser_AddForLoop($2,    NULL, $4, $3, state));}
              ;

for_loop_direction:   TOKEN_TO     {$$ = TSDEF_FOR_LOOP_FLAG_UP;}
                    | TOKEN_DOWNTO {$$ = TSDEF_FOR_LOOP_FLAG_DOWN;}
                    ;


loop_flow_control:   TOKEN_CONTINUE separator {VERIFY(TSDef_Parser_AddLoopFlowControl(TSDEF_STATEMENT_TYPE_CONTINUE, state));}
                   | TOKEN_BREAK    separator {VERIFY(TSDef_Parser_AddLoopFlowControl(TSDEF_STATEMENT_TYPE_BREAK,    state));}
                   ;


end_block: TOKEN_END separator {TSDef_Parser_EndBlock(state);}
           ;


finish: TOKEN_FINISH separator {VERIFY(TSDef_Parser_AddFinish(state));}
        ;


exp_list:   exp                      {VERIFY(TSDef_Parser_ExpList($1, NULL, state, &$$));}
          | exp TOKEN_COMMA exp_list {VERIFY(TSDef_Parser_ExpList($1,   $3, state, &$$));}
          ;


exp:   primary_exp    {VERIFY(TSDef_Parser_Exp(TSDEF_EXP_TYPE_PRIMARY,    $1, state, &$$));}
     | comparison_exp {VERIFY(TSDef_Parser_Exp(TSDEF_EXP_TYPE_COMPARISON, $1, state, &$$));}
     | logical_exp    {VERIFY(TSDef_Parser_Exp(TSDEF_EXP_TYPE_LOGICAL,    $1, state, &$$));}
     ;


logical_exp:   logical_sub_exp     logical_exp_op logical_sub_exp {VERIFY(TSDef_Parser_LogicalExp($2,                         $1.exp_val, $1.flags_val, $3.exp_val, $3.flags_val, NULL, state, &$$));}
             | logical_not_sub_exp logical_exp_op logical_sub_exp {VERIFY(TSDef_Parser_LogicalExp($2,                         $1.exp_val, $1.flags_val, $3.exp_val, $3.flags_val, NULL, state, &$$));}
             | logical_sub_exp     logical_exp_op logical_exp     {VERIFY(TSDef_Parser_LogicalExp($2,                         $1.exp_val, $1.flags_val,       NULL,            0,   $3, state, &$$));}
             | logical_not_sub_exp logical_exp_op logical_exp     {VERIFY(TSDef_Parser_LogicalExp($2,                         $1.exp_val, $1.flags_val,       NULL,            0,   $3, state, &$$));}
             | logical_not_sub_exp                                {VERIFY(TSDef_Parser_LogicalExp(TSDEF_LOGICAL_EXP_OP_VALUE, $1.exp_val, $1.flags_val,       NULL,            0, NULL, state, &$$));}
             ;

logical_exp_op:   TOKEN_LOGICAL_AND {$$ = TSDEF_LOGICAL_EXP_OP_AND;}
                | TOKEN_LOGICAL_OR  {$$ = TSDEF_LOGICAL_EXP_OP_OR;}

logical_not_sub_exp: logical_not_qualifier logical_sub_exp {$$.flags_val = $1; $$.exp_val = $2.exp_val;}
                     ;

logical_sub_exp:   primary_exp    {$$.flags_val = 0;  VERIFY(TSDef_Parser_Exp(TSDEF_EXP_TYPE_PRIMARY,    $1, state, &$$.exp_val));}
                 | comparison_exp {$$.flags_val = 0;  VERIFY(TSDef_Parser_Exp(TSDEF_EXP_TYPE_COMPARISON, $1, state, &$$.exp_val));}
                 ;

logical_not_qualifier:   TOKEN_LOGICAL_NOT                       {$$ = TSDEF_LOGICAL_EXP_FLAG_NOT;}
                       | TOKEN_LOGICAL_NOT logical_not_qualifier {$$ = ($2)^TSDEF_LOGICAL_EXP_FLAG_NOT;}
                       ;


comparison_exp:   primary_exp comparison_exp_op primary_exp    {VERIFY(TSDef_Parser_ComparisonExp($2, $1,   $3, NULL, state, &$$));}
                | primary_exp comparison_exp_op comparison_exp {VERIFY(TSDef_Parser_ComparisonExp($2, $1, NULL,   $3, state, &$$));}
                ;

comparison_exp_op:   TOKEN_COMPARISON_EQUAL         {$$ = TSDEF_COMPARISON_EXP_OP_EQUAL;}
                   | TOKEN_COMPARISON_NOT_EQUAL     {$$ = TSDEF_COMPARISON_EXP_OP_NOT_EQUAL;}
                   | TOKEN_COMPARISON_GREATER       {$$ = TSDEF_COMPARISON_EXP_OP_GREATER;}
                   | TOKEN_COMPARISON_GREATER_EQUAL {$$ = TSDEF_COMPARISON_EXP_OP_GREATER_EQUAL;}
                   | TOKEN_COMPARISON_LESS          {$$ = TSDEF_COMPARISON_EXP_OP_LESS;}
                   | TOKEN_COMPARISON_LESS_EQUAL    {$$ = TSDEF_COMPARISON_EXP_OP_LESS_EQUAL;}
                   ;


primary_exp:   base_primary_exp                      {$$ = $1;}
             | TOKEN_ARITHMETIC_SUB base_primary_exp {$$ = $2; VERIFY(TSDef_Parser_SetPrimaryExpFlag(TSDEF_PRIMARY_EXP_FLAG_NEGATE, $$, state));}
             ;

base_primary_exp:   exp_value_type                                 {VERIFY(TSDef_Parser_PrimaryExp(TSDEF_PRIMARY_EXP_OP_VALUE, $1, NULL, state, &$$));}
                  | exp_value_type primary_exp_op base_primary_exp {VERIFY(TSDef_Parser_PrimaryExp($2,                         $1,   $3, state, &$$));}
                  ;

primary_exp_op:   TOKEN_ARITHMETIC_ADD {$$ = TSDEF_PRIMARY_EXP_OP_ADD;}
                | TOKEN_ARITHMETIC_SUB {$$ = TSDEF_PRIMARY_EXP_OP_SUB;}
                | TOKEN_ARITHMETIC_MUL {$$ = TSDEF_PRIMARY_EXP_OP_MUL;}
                | TOKEN_ARITHMETIC_DIV {$$ = TSDEF_PRIMARY_EXP_OP_DIV;}
                | TOKEN_ARITHMETIC_POW {$$ = TSDEF_PRIMARY_EXP_OP_POW;}
                | TOKEN_ARITHMETIC_MOD {$$ = TSDEF_PRIMARY_EXP_OP_MOD;}
                ;


exp_value_type:   TOKEN_BOOL                      {VERIFY(TSDef_Parser_ExpValueType(TSDEF_EXP_VALUE_TYPE_BOOL,          &$1, state, &$$));}
                | TOKEN_INT                       {VERIFY(TSDef_Parser_ExpValueType(TSDEF_EXP_VALUE_TYPE_INT,           &$1, state, &$$));}
                | TOKEN_REAL                      {VERIFY(TSDef_Parser_ExpValueType(TSDEF_EXP_VALUE_TYPE_REAL,          &$1, state, &$$));}
                | TOKEN_STRING                    {VERIFY(TSDef_Parser_ExpValueType(TSDEF_EXP_VALUE_TYPE_STRING,         $1, state, &$$));}
                | function_call                   {VERIFY(TSDef_Parser_ExpValueType(TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL,  $1, state, &$$));}
                | variable                        {VERIFY(TSDef_Parser_ExpValueType(TSDEF_EXP_VALUE_TYPE_VARIABLE,       $1, state, &$$));}
                | TOKEN_L_PAREN exp TOKEN_R_PAREN {VERIFY(TSDef_Parser_ExpValueType(TSDEF_EXP_VALUE_TYPE_EXP,            $2, state, &$$));}
                ;


function_call_list:   function_call                                {VERIFY(TSDef_Parser_FunctionCallList($1, NULL, state, &$$));}
                    | function_call TOKEN_COMMA function_call_list {VERIFY(TSDef_Parser_FunctionCallList($1,   $3, state, &$$));}
                    ;


function_call:   TOKEN_IDENTIFIER TOKEN_L_PAREN exp_list TOKEN_R_PAREN {VERIFY(TSDef_Parser_FunctionCall($1,   $3, state, &$$));}
               | TOKEN_IDENTIFIER TOKEN_L_PAREN TOKEN_R_PAREN          {VERIFY(TSDef_Parser_FunctionCall($1, NULL, state, &$$));}
               ;


variable_list:   variable                           {VERIFY(TSDef_Parser_VariableList($1, NULL, state, &$$));}
               | variable TOKEN_COMMA variable_list {VERIFY(TSDef_Parser_VariableList($1,   $3, state, &$$));}
               ;


variable: TOKEN_IDENTIFIER {VERIFY(TSDef_Parser_ReferenceVariable($1, state, &$$));}
          ;


separator_list:   separator
                | separator separator_list


separator: TOKEN_NEW_LINE
           ;

