/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSDEF_DEFERROR_H_
#define _TSDEF_DEFERROR_H_


#define TSDEF_DEF_ERROR_NONE                       0
#define TSDEF_DEF_ERROR_INTERNAL                  -1
#define TSDEF_DEF_ERROR_SYNTAX                    -2
#define TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES        -3
#define TSDEF_DEF_ERROR_INVALID_USE_OF_OPERATOR   -4
#define TSDEF_DEF_ERROR_UNDEFINED_VARIABLE        -5
#define TSDEF_DEF_ERROR_FLOW_CONTROL_OUTSIDE_LOOP -6
#define TSDEF_DEF_ERROR_WRONG_ARGUMENT_COUNT      -7
#define TSDEF_DEF_ERROR_VARIABLE_REDEFINITION     -8
#define TSDEF_DEF_ERROR_UNDEFINED_FUNCTION        -9
#define TSDEF_DEF_ERROR_USING_VOID_TYPE           -10
#define TSDEF_DEF_ERROR_USING_DELAYED_TYPE        -11
#define TSDEF_DEF_ERROR_FUNCTION_REDEFINITION     -12
#define TSDEF_DEF_ERROR_TYPE_NOT_STEPPABLE        -13
#define TSDEF_DEF_ERROR_FUNCTION_NOT_ACTIONABLE   -14
#define TSDEF_DEF_ERROR_FUNCTION_NOT_INVOCABLE    -15

#define TSDEF_DEF_ERROR_FLAG_WARNING    0x01
#define TSDEF_DEF_ERROR_FLAG_INFO_VALID 0x02


struct tsdef_def_error_info
{
    union
    {
        struct
        {
            char*        convert_to_name;
            unsigned int to_type;
            unsigned int from_type;
        }incompatible_types;

        struct
        {
            char* name;
        }undefined_variable;

        struct
        {
            unsigned int operator;
            unsigned int type;
        }invalid_use_of_operator;

        struct
        {
            char*        name;
            unsigned int passed_count;
            unsigned int required_count;
        }wrong_argument_count;

        struct
        {
            char* name;
        }variable_redefinition;

        struct
        {
            char* name;
        }undefined_function;

        struct
        {
            char* name;
        }function_redefinition;

        struct
        {
            unsigned int type;
        }type_not_steppable;

        struct
        {
            char* name;
        }function_not_actionable;

        struct
        {
            char* name;
        }function_not_invocable;
    }data;
};

struct tsdef_def_error
{
    int          error;
    char*        unit_name;
    unsigned int location;
    unsigned int flags;

    struct tsdef_def_error_info info;

    struct tsdef_def_error* next_error;
};

struct tsdef_def_error_list
{
    struct tsdef_def_error* encountered_errors;
    struct tsdef_def_error* last_error;

    unsigned int error_count;
    unsigned int warning_count;
};


extern void TSDef_InitializeDefErrorList (struct tsdef_def_error_list*);
extern int  TSDef_AppendDefErrorList     (
                                          int,
                                          char*,
                                          unsigned int,
                                          unsigned int,
                                          struct tsdef_def_error_info*,
                                          struct tsdef_def_error_list*
                                         );
extern void TSDef_DestroyDefErrorList    (struct tsdef_def_error_list*);


#endif

