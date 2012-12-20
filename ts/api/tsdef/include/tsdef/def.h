/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSDEF_DEF_H_
#define _TSDEF_DEF_H_


#define TSDEF_PRIMITIVE_CONVERSION_ALLOWED     0
#define TSDEF_PRIMITIVE_CONVERSION_DISALLOWED -1

#define TSDEF_OP_ALLOWED     0
#define TSDEF_OP_DISALLOWED -1

#define TSDEF_BOOL_FALSE 0
#define TSDEF_BOOL_TRUE  1

#define TSDEF_BOOL_FALSE_STRING "false"
#define TSDEF_BOOL_TRUE_STRING  "true"

#define TSDEF_PRIMITIVE_TYPE_VOID    0
#define TSDEF_PRIMITIVE_TYPE_DELAYED 1
#define TSDEF_PRIMITIVE_TYPE_BOOL    2
#define TSDEF_PRIMITIVE_TYPE_INT     3
#define TSDEF_PRIMITIVE_TYPE_REAL    4
#define TSDEF_PRIMITIVE_TYPE_STRING  5

#define TSDEF_EXP_VALUE_TYPE_BOOL            0
#define TSDEF_EXP_VALUE_TYPE_INT             1
#define TSDEF_EXP_VALUE_TYPE_REAL            2
#define TSDEF_EXP_VALUE_TYPE_STRING          3
#define TSDEF_EXP_VALUE_TYPE_FUNCTION_CALL   4
#define TSDEF_EXP_VALUE_TYPE_VARIABLE        5
#define TSDEF_EXP_VALUE_TYPE_EXP             6

#define TSDEF_PRIMARY_EXP_FLAG_NEGATE 0x01

#define TSDEF_PRIMARY_EXP_OP_VALUE      0
#define TSDEF_PRIMARY_EXP_OP_ADD        1
#define TSDEF_PRIMARY_EXP_OP_SUB        2
#define TSDEF_PRIMARY_EXP_OP_MUL        3
#define TSDEF_PRIMARY_EXP_OP_DIV        4
#define TSDEF_PRIMARY_EXP_OP_MOD        5
#define TSDEF_PRIMARY_EXP_OP_POW        6

#define TSDEF_COMPARISON_EXP_OP_EQUAL         0
#define TSDEF_COMPARISON_EXP_OP_NOT_EQUAL     1
#define TSDEF_COMPARISON_EXP_OP_GREATER       2
#define TSDEF_COMPARISON_EXP_OP_GREATER_EQUAL 3
#define TSDEF_COMPARISON_EXP_OP_LESS          4
#define TSDEF_COMPARISON_EXP_OP_LESS_EQUAL    5

#define TSDEF_LOGICAL_EXP_FLAG_NOT 0x01

#define TSDEF_LOGICAL_EXP_OP_VALUE 0
#define TSDEF_LOGICAL_EXP_OP_OR    1
#define TSDEF_LOGICAL_EXP_OP_AND   2

#define TSDEF_EXP_TYPE_PRIMARY    0
#define TSDEF_EXP_TYPE_COMPARISON 1
#define TSDEF_EXP_TYPE_LOGICAL    2

#define TSDEF_IF_STATEMENT_FLAG_ELSE 0x01

#define TSDEF_LOOP_TYPE_FOR   0x01
#define TSDEF_LOOP_TYPE_WHILE 0x02

#define TSDEF_FOR_LOOP_FLAG_UP   0x01
#define TSDEF_FOR_LOOP_FLAG_DOWN 0x02

#define TSDEF_STATEMENT_TYPE_FUNCTION_CALL 0
#define TSDEF_STATEMENT_TYPE_ASSIGNMENT    1
#define TSDEF_STATEMENT_TYPE_IF_STATEMENT  2
#define TSDEF_STATEMENT_TYPE_LOOP          3
#define TSDEF_STATEMENT_TYPE_CONTINUE      4
#define TSDEF_STATEMENT_TYPE_BREAK         5
#define TSDEF_STATEMENT_TYPE_FINISH        6


typedef char   tsdef_bool;
typedef int    tsdef_int;
typedef double tsdef_real;
typedef char*  tsdef_string;

struct tsdef_variable
{
    char*        name;
    unsigned int primitive_type;

    struct tsdef_block* block;
    unsigned int        index;

    struct tsdef_variable* next_variable;
};

struct tsdef_variable_reference
{
    char*                  name;
    struct tsdef_variable* variable;
};

struct tsdef_variable_list_node
{
    struct tsdef_variable_reference* variable;

    struct tsdef_variable_list_node* next_variable;
};

struct tsdef_variable_list
{
    struct tsdef_variable_list_node  end;
    struct tsdef_variable_list_node* start;

    unsigned int count;
};

struct tsdef_function_call
{
    char*                       name;
    struct tsdef_module_object* module_object;
    struct tsdef_exp_list*      arguments;
};

struct tsdef_function_call_list_node
{
    struct tsdef_function_call* function_call;

    struct tsdef_function_call_list_node* next_function_call;
};

struct tsdef_function_call_list
{
    struct tsdef_function_call_list_node  end;
    struct tsdef_function_call_list_node* start;

    unsigned int count;
};

struct tsdef_block
{
    unsigned int depth;

    struct tsdef_variable* variables;
    unsigned int           variable_count;

    struct tsdef_statement* statements;
    struct tsdef_statement* last_statement;
    unsigned int            statement_count;

    struct tsdef_block*     parent_block;
    struct tsdef_statement* parent_statement;
};

struct tsdef_exp_value_type
{
    unsigned int type;

    union
    {
        tsdef_bool                       bool_constant;
        tsdef_int                        int_constant;
        tsdef_real                       real_constant;
        tsdef_string                     string_constant;
        struct tsdef_exp*                exp;
        struct tsdef_function_call*      function_call;
        struct tsdef_variable_reference* variable;
    }data;
};

struct tsdef_primary_exp_node
{
    unsigned int                 op;
    struct tsdef_exp_value_type* exp_value_type;

    struct tsdef_primary_exp_node* remaining_exp;
};

struct tsdef_primary_exp
{
    unsigned int flags;
    unsigned int effective_primitive_type;

    struct tsdef_primary_exp_node  end;
    struct tsdef_primary_exp_node* start;
};

struct tsdef_comparison_exp_node
{
    unsigned int op;
    unsigned int primitive_type;

    struct tsdef_primary_exp* left_exp;
    struct tsdef_primary_exp* right_exp;

    struct tsdef_comparison_exp_node* remaining_exp;
};

struct tsdef_comparison_exp
{
    struct tsdef_comparison_exp_node  end;
    struct tsdef_comparison_exp_node* start;
};

struct tsdef_logical_exp_node
{
    unsigned int op;

    struct tsdef_exp* left_exp;
    unsigned int      left_exp_flags;

    struct tsdef_exp* right_exp;
    unsigned int      right_exp_flags;

    struct tsdef_logical_exp_node* remaining_exp;
};

struct tsdef_logical_exp
{
    struct tsdef_logical_exp_node  end;
    struct tsdef_logical_exp_node* start;
};

struct tsdef_exp
{
    unsigned int type;

    union
    {
        struct tsdef_primary_exp*    primary_exp;
        struct tsdef_comparison_exp* comparison_exp;
        struct tsdef_logical_exp*    logical_exp;
    }data;
};

struct tsdef_exp_list_node
{
    struct tsdef_exp* exp;

    struct tsdef_exp_list_node* next_exp;
};

struct tsdef_exp_list
{
    struct tsdef_exp_list_node  end;
    struct tsdef_exp_list_node* start;

    unsigned int count;
};

struct tsdef_assignment
{
    struct tsdef_variable_reference* lvalue;
    struct tsdef_exp*                rvalue;
};

struct tsdef_if_statement
{
    struct tsdef_exp* exp;
    unsigned int      flags;

    struct tsdef_block block;
};

struct tsdef_for_loop
{
    struct tsdef_variable_reference*   variable;
    struct tsdef_assignment*           assignment;
    struct tsdef_exp*                  to_exp;

    unsigned int flags;
};

struct tsdef_while_loop
{
    struct tsdef_exp* exp;
};

struct tsdef_loop
{
    unsigned int type;

    struct
    {
        struct tsdef_for_loop   for_loop;
        struct tsdef_while_loop while_loop;
    }data;

    struct tsdef_block block;
};

struct tsdef_statement
{
    unsigned int type;

    union
    {
        struct tsdef_function_call* function_call;
        struct tsdef_assignment*    assignment;
        struct tsdef_if_statement*  if_statement;
        struct tsdef_loop*          loop;
    }data;

    unsigned int location;

    struct tsdef_statement* next_statement;
};

struct tsdef_action
{
    struct tsdef_function_call_list* trigger_list;
    unsigned int                     location;

    struct tsdef_block block;

    struct tsdef_action* next_action;
};

struct tsdef_input
{
    struct tsdef_variable_list* input_variables;

    unsigned int location;
};

struct tsdef_output
{
    struct tsdef_assignment* output_variable_assignment;

    unsigned int location;
};

struct tsdef_unit
{
    char*        name;
    unsigned int unit_id;

    struct tsdef_input*  input;
    struct tsdef_output* output;

    struct tsdef_block global_block;

    struct tsdef_action* actions;
    unsigned int         action_count;
};


extern unsigned int tsdef_primary_exp_op_precedence[];
extern unsigned int tsdef_logical_exp_op_precedence[];

extern unsigned int tsdef_primitive_type_rank[];


extern struct tsdef_variable*           TSDef_LookupVariable           (char*, struct tsdef_block*);
extern int                              TSDef_DeclareVariable          (
                                                                        char*,
                                                                        struct tsdef_block*,
                                                                        struct tsdef_variable**
                                                                       );
extern int                              TSDef_ReferenceVariable        (char*, struct tsdef_variable_reference**);
extern struct tsdef_variable_reference* TSDef_CloneVariableReference   (struct tsdef_variable_reference*);
extern void                             TSDef_DestroyVariableReference (struct tsdef_variable_reference*);

extern int                         TSDef_ConstructVariableList (
                                                                struct tsdef_variable_reference*,
                                                                struct tsdef_variable_list*,
                                                                struct tsdef_variable_list**
                                                               );
extern struct tsdef_variable_list* TSDef_CloneVariableList     (struct tsdef_variable_list*);
extern void                        TSDef_DestroyVariableList   (struct tsdef_variable_list*);

extern int                         TSDef_DefineFunctionCall  (
                                                              char*,
                                                              struct tsdef_exp_list*,
                                                              struct tsdef_function_call**
                                                             );
extern struct tsdef_function_call* TSDef_CloneFunctionCall   (struct tsdef_function_call*);
extern void                        TSDef_DestroyFunctionCall (struct tsdef_function_call*);

extern int                              TSDef_ConstructFunctionCallList (
                                                                         struct tsdef_function_call*,
                                                                         struct tsdef_function_call_list*,
                                                                         struct tsdef_function_call_list**
                                                                        );
extern struct tsdef_function_call_list* TSDef_CloneFunctionCallList     (struct tsdef_function_call_list*);
extern void                             TSDef_DestroyFunctionCallList   (struct tsdef_function_call_list*);

extern int          TSDef_AllowPrimitiveConversion (unsigned int, unsigned int);
extern unsigned int TSDef_SelectPrimitivePromotion (unsigned int, unsigned int);
extern int          TSDef_OpAllowed                (unsigned int, unsigned int, unsigned int);
extern int          TSDef_SteppablePrimitive       (unsigned int);

extern int                          TSDef_CreateExpValueType    (
                                                                 unsigned int,
                                                                 void*,
                                                                 struct tsdef_exp_value_type**
                                                                );
extern unsigned int                 TSDef_ExpValuePrimitiveType (struct tsdef_exp_value_type*);
extern struct tsdef_exp_value_type* TSDef_CloneExpValueType     (struct tsdef_exp_value_type*);
extern void                         TSDef_DestroyExpValueType   (struct tsdef_exp_value_type*);

extern int                       TSDef_ConstructPrimaryExp (
                                                            unsigned int,
                                                            struct tsdef_exp_value_type*,
                                                            struct tsdef_primary_exp*,
                                                            struct tsdef_primary_exp**
                                                           );
extern int                       TSDef_SetPrimaryExpFlag   (
                                                            unsigned int,
                                                            struct tsdef_primary_exp*
                                                           );
extern struct tsdef_primary_exp* TSDef_ClonePrimaryExp     (struct tsdef_primary_exp*);
extern void                      TSDef_DestroyPrimaryExp   (struct tsdef_primary_exp*);

extern int                          TSDef_ConstructComparisonExp (
                                                                  unsigned int,
                                                                  struct tsdef_primary_exp*,
                                                                  struct tsdef_primary_exp*,
                                                                  struct tsdef_comparison_exp*,
                                                                  struct tsdef_comparison_exp**
                                                                 );
extern struct tsdef_comparison_exp* TSDef_CloneComparisonExp     (struct tsdef_comparison_exp*);
extern void                         TSDef_DestroyComparisonExp   (struct tsdef_comparison_exp*);

extern int                       TSDef_ConstructLogicalExp (
                                                            unsigned int,
                                                            struct tsdef_exp*,
                                                            unsigned int,
                                                            struct tsdef_exp*,
                                                            unsigned int,
                                                            struct tsdef_logical_exp*,
                                                            struct tsdef_logical_exp**
                                                           );
extern struct tsdef_logical_exp* TSDef_CloneLogicalExp     (struct tsdef_logical_exp*);
extern void                      TSDef_DestroyLogicalExp   (struct tsdef_logical_exp*);

extern int               TSDef_CreateExp        (
                                                 unsigned int,
                                                 void*,
                                                 struct tsdef_exp**
                                                );
extern unsigned int      TSDef_ExpPrimitiveType (struct tsdef_exp*);
extern struct tsdef_exp* TSDef_CloneExp         (struct tsdef_exp*);
extern void              TSDef_DestroyExp       (struct tsdef_exp*);

extern int                    TSDef_ConstructExpList (
                                                      struct tsdef_exp*,
                                                      struct tsdef_exp_list*,
                                                      struct tsdef_exp_list**
                                                     );
extern struct tsdef_exp_list* TSDef_CloneExpList     (struct tsdef_exp_list*);
extern void                   TSDef_DestroyExpList   (struct tsdef_exp_list*);

extern int                      TSDef_DefineAssignment  (
                                                         struct tsdef_variable_reference*,
                                                         struct tsdef_exp*,
                                                         struct tsdef_assignment**
                                                        );
extern struct tsdef_assignment* TSDef_CloneAssignment   (struct tsdef_assignment*);
extern void                     TSDef_DestroyAssignment (struct tsdef_assignment*);

extern int                        TSDef_DefineIfStatement  (
                                                            struct tsdef_exp*,
                                                            unsigned int,
                                                            struct tsdef_if_statement**
                                                           );
extern struct tsdef_if_statement* TSDef_CloneIfStatement   (struct tsdef_if_statement*);
extern void                       TSDef_DestroyIfStatement (struct tsdef_if_statement*);

extern int          TSDef_DeclareLoop   (struct tsdef_loop**);
extern void         TSDef_MakeLoopWhile (
                                         struct tsdef_exp*,
                                         struct tsdef_loop*
                                        );
extern void         TSDef_MakeLoopFor   (
                                         struct tsdef_variable_reference*,
                                         struct tsdef_assignment*,
                                         struct tsdef_exp*,
                                         unsigned int,
                                         struct tsdef_loop*
                                        );
struct tsdef_loop* TSDef_CloneLoop      (struct tsdef_loop*);
extern void        TSDef_DestroyLoop    (struct tsdef_loop*);

extern int TSDef_AppendStatement (
                                  unsigned int,
                                  void*,
                                  unsigned int,
                                  struct tsdef_block*,
                                  struct tsdef_statement**
                                 );

extern int TSDef_AddAction (
                            struct tsdef_function_call_list*,
                            unsigned int,
                            struct tsdef_unit*,
                            struct tsdef_action**
                           );

extern int TSDef_DefineInput  (
                               struct tsdef_variable_list*,
                               unsigned int,
                               struct tsdef_unit*,
                               struct tsdef_input**
                              );
extern int TSDef_DefineOutput (
                               struct tsdef_assignment*,
                               unsigned int,
                               struct tsdef_unit*,
                               struct tsdef_output**
                              );

extern int  TSDef_InitializeUnit (char*, struct tsdef_unit*);
extern int  TSDef_CloneUnit      (struct tsdef_unit*, struct tsdef_unit*);
extern void TSDef_DestroyUnit    (struct tsdef_unit*);


#endif

