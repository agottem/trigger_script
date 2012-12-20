/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TTSDEF_PARSERUTIL_H_
#define _TTSDEF_PARSERUTIL_H_


#include <tsdef/def.h>


#define TSDEF_UTIL_CONTINUE_PARSE   0
#define TSDEF_UTIL_ABORT_STATEMENT -1
#define TSDEF_UTIL_ABORT_PARSE     -2


struct tsdef_parser_state
{
    struct tsdef_unit* unit;

    struct tsdef_block* current_block;
    unsigned int        current_line_number;

    struct tsdef_def_error_list* error_list;
    unsigned int                 error_count;
    unsigned int                 warning_count;
};


extern void yyerror                   (struct tsdef_parser_state*, char*);
extern void TSDef_Parser_ProcessError (struct tsdef_parser_state*);

extern int TSDef_Parser_Input  (struct tsdef_variable_list*, struct tsdef_parser_state*);
extern int TSDef_Parser_Output (struct tsdef_assignment*, struct tsdef_parser_state*);

extern int  TSDef_Parser_Action    (struct tsdef_function_call_list*, struct tsdef_parser_state*);
extern void TSDef_Parser_EndAction (struct tsdef_parser_state*);

extern int TSDef_Parser_AddFunctionCall (
                                         struct tsdef_function_call*,
                                         struct tsdef_parser_state*
                                        );

extern int TSDef_Parser_Assignment       (
                                          struct tsdef_variable_reference*,
                                          struct tsdef_exp*,
                                          struct tsdef_parser_state*,
                                          struct tsdef_assignment**
                                         );
extern int TSDef_Parser_AddAssignment (
                                       struct tsdef_assignment*,
                                       struct tsdef_parser_state*
                                      );

extern int TSDef_Parser_AddIfStatement (
                                        struct tsdef_exp*,
                                        unsigned int,
                                        struct tsdef_parser_state*
                                       );

extern int TSDef_Parser_AddLoop            (struct tsdef_parser_state*, struct tsdef_loop**);
extern int TSDef_Parser_AddWhileLoop       (struct tsdef_exp*, struct tsdef_parser_state*);
extern int TSDef_Parser_AddForLoop         (
                                            struct tsdef_assignment*,
                                            struct tsdef_variable_reference*,
                                            struct tsdef_exp*,
                                            unsigned int,
                                            struct tsdef_parser_state*
                                           );
extern int TSDef_Parser_AddLoopFlowControl (unsigned int, struct tsdef_parser_state*);

extern void TSDef_Parser_EndBlock (struct tsdef_parser_state*);

extern int TSDef_Parser_ExpList (
                                 struct tsdef_exp*,
                                 struct tsdef_exp_list*,
                                 struct tsdef_parser_state*,
                                 struct tsdef_exp_list**
                                );

extern int TSDef_Parser_AddFinish (struct tsdef_parser_state*);

extern int TSDef_Parser_Exp (
                             unsigned int,
                             void*,
                             struct tsdef_parser_state*,
                             struct tsdef_exp**
                            );

extern int TSDef_Parser_LogicalExp        (
                                           unsigned int,
                                           struct tsdef_exp*,
                                           unsigned int,
                                           struct tsdef_exp*,
                                           unsigned int,
                                           struct tsdef_logical_exp*,
                                           struct tsdef_parser_state*,
                                           struct tsdef_logical_exp**
                                          );
extern int TSDef_Parser_ComparisonExp     (
                                           unsigned int,
                                           struct tsdef_primary_exp*,
                                           struct tsdef_primary_exp*,
                                           struct tsdef_comparison_exp*,
                                           struct tsdef_parser_state*,
                                           struct tsdef_comparison_exp**
                                          );
extern int TSDef_Parser_PrimaryExp        (
                                           unsigned int,
                                           struct tsdef_exp_value_type*,
                                           struct tsdef_primary_exp*,
                                           struct tsdef_parser_state*,
                                           struct tsdef_primary_exp**
                                          );
extern int TSDef_Parser_SetPrimaryExpFlag (
                                           unsigned int,
                                           struct tsdef_primary_exp*,
                                           struct tsdef_parser_state*
                                          );

extern int TSDef_Parser_ExpValueType (
                                      unsigned int,
                                      void*,
                                      struct tsdef_parser_state*,
                                      struct tsdef_exp_value_type**
                                     );

extern int TSDef_Parser_FunctionCallList (
                                          struct tsdef_function_call*,
                                          struct tsdef_function_call_list*,
                                          struct tsdef_parser_state*,
                                          struct tsdef_function_call_list**
                                         );

extern int TSDef_Parser_FunctionCall (
                                      char*,
                                      struct tsdef_exp_list*,
                                      struct tsdef_parser_state*,
                                      struct tsdef_function_call**
                                     );

extern int TSDef_Parser_VariableList (
                                      struct tsdef_variable_reference*,
                                      struct tsdef_variable_list*,
                                      struct tsdef_parser_state*,
                                      struct tsdef_variable_list**
                                     );

extern int TSDef_Parser_ReferenceVariable (
                                           char*,
                                           struct tsdef_parser_state*,
                                           struct tsdef_variable_reference**
                                          );


#endif

