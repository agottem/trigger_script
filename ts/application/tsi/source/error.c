/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "error.h"
#include "context.h"
#include "main.h"

#include <tsdef/def.h>

#include <stdio.h>
#include <string.h>


#define LINES_OF_CONTEXT 3
#define PADDING_SIZE     5


static void PrintError (struct tsdef_def_error*);


static char* type_names[] = {
                             "void",
                             "delayed",
                             "bool",
                             "integer",
                             "real",
                             "string"
                            };

static char* operator_names[] = {
                                 "value",
                                 "+",
                                 "-",
                                 "*",
                                 "/",
                                 "%",
                                 "^"
                                };


void TSI_ReportDefErrors (struct tsdef_def_error_list* error_list)
{
    struct tsdef_def_error* def_error;

    printf("\n");

    for(
        def_error = error_list->encountered_errors;
        def_error != NULL;
        def_error = def_error->next_error
       )
    {
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_WARNING)
            printf("WARNING");
        else
            printf("ERROR");

        printf("[%d] function=%s line=%d: ", def_error->error, def_error->unit_name, def_error->location);

        switch(def_error->error)
        {
        case TSDEF_DEF_ERROR_INTERNAL:
            printf("Unrecoverable internal error");

            break;

        case TSDEF_DEF_ERROR_SYNTAX:
            if(strcmp(def_error->unit_name, "_module_main") == 0)
            {
                if(tsi_unit_invocation != NULL)
                {
                    printf(
                           "Verify the function '%s' is present in one of the specified search paths, and is invoked with the correct syntax",
                           tsi_unit_invocation
                          );
                }
                else
                    printf("Verify the function specified to be executed is present in one of the specified search paths, and is invoked with the correct syntax");
            }
            else
                printf("Syntax error");

            break;

        case TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Incompatible types, cannot convert expression of type '%s' to variable '%s' of type '%s'",
                       type_names[def_error->info.data.incompatible_types.from_type],
                       def_error->info.data.incompatible_types.convert_to_name,
                       type_names[def_error->info.data.incompatible_types.to_type]
                      );
            }
            else
                printf("Incompatible types");

            break;

        case TSDEF_DEF_ERROR_INVALID_USE_OF_OPERATOR:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Invalid use of operator '%s' with type '%s'",
                       operator_names[def_error->info.data.invalid_use_of_operator.operator],
                       type_names[def_error->info.data.invalid_use_of_operator.type]
                      );
            }
            else
                printf("Invalid use of operator");

            break;

        case TSDEF_DEF_ERROR_UNDEFINED_VARIABLE:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
                printf("Use of undefined variable '%s'", def_error->info.data.undefined_variable.name);
            else
                printf("Use of undefined variable");

            break;

        case TSDEF_DEF_ERROR_FLOW_CONTROL_OUTSIDE_LOOP:
            printf("Flow control statement 'break' or 'continue' being used outside of loop");

            break;

        case TSDEF_DEF_ERROR_WRONG_ARGUMENT_COUNT:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Function '%s' was passed %d arguments when %d were expected",
                       def_error->info.data.wrong_argument_count.name,
                       def_error->info.data.wrong_argument_count.passed_count,
                       def_error->info.data.wrong_argument_count.required_count
                      );
            }
            else
                printf("Wrong number of arguments passed to function");

            break;

        case TSDEF_DEF_ERROR_VARIABLE_REDEFINITION:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Redefinition of variable '%s'",
                       def_error->info.data.variable_redefinition.name
                      );
            }
            else
                printf("Variable redefinition");

            break;

        case  TSDEF_DEF_ERROR_UNDEFINED_FUNCTION:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Function with name '%s' could not be found",
                       def_error->info.data.undefined_function.name
                      );
            }
            else
                printf("Undefined function");

            break;

        case TSDEF_DEF_ERROR_USING_VOID_TYPE:
            printf("Functions which have no output cannot be used in an expression");

            break;

        case TSDEF_DEF_ERROR_USING_DELAYED_TYPE:
            printf("Expression type could not be decided");

            break;

        case TSDEF_DEF_ERROR_FUNCTION_REDEFINITION:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
                printf("Function '%s' already defined", def_error->info.data.function_redefinition.name);
            else
                printf("Function redefinition");

            break;

        case TSDEF_DEF_ERROR_TYPE_NOT_STEPPABLE:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Unsteppable type '%s' used in for loop",
                       type_names[def_error->info.data.type_not_steppable.type]
                      );
            }
            else
                printf("Unsteppable type used in for loop");

            break;

        case TSDEF_DEF_ERROR_FUNCTION_NOT_ACTIONABLE:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Function '%s' used in action but function is not actionable",
                       def_error->info.data.function_not_actionable.name
                      );
            }
            else
                printf("Unactionable function used in action");

            break;

        case TSDEF_DEF_ERROR_FUNCTION_NOT_INVOCABLE:
            if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
            {
                printf(
                       "Function '%s' is not invocable",
                       def_error->info.data.function_not_invocable.name
                      );
            }
            else
                printf("Uninvocable function used in action");

            break;

        default:
            printf("Unknown error");

            break;
        }

        printf(
               "\n"
               "Code containing error:\n"
              );

        TSI_DisplayContext(
                           def_error->unit_name,
                           def_error->location,
                           LINES_OF_CONTEXT,
                           PADDING_SIZE
                          );

        printf("\n");
    }
}

