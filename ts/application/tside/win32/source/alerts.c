/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "alerts.h"
#include "paths.h"

#include <tsutil/path.h>
#include <tsutil/error.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>


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


struct tside_alert* TSIDE_AlertFromDefError (
                                             struct tsdef_def_error* def_error,
                                             char*                   invocation_text
                                            )
{
    struct tside_alert* alert;
    char*               type_text;
    char*               file_name;
    unsigned int        type;
    int                 error;

    alert = malloc(sizeof(struct tside_alert));
    if(alert == NULL)
        return NULL;

    if(def_error->flags&TSDEF_DEF_ERROR_FLAG_WARNING)
    {
        type_text = "Warning";
        type      = TSIDE_ALERT_TYPE_COMPILE_WARNING;
    }
    else
    {
        type_text = "Error";
        type      = TSIDE_ALERT_TYPE_COMPILE_ERROR;
    }

    error = TSUtil_FindUnitFile(def_error->unit_name, ".ts", &tside_unit_paths, &file_name);
    if(error != TSUTIL_ERROR_NONE)
        alert->file_name[0] = 0;
    else
    {
        char* separators;

        for(separators = file_name; *separators != 0; separators++)
        {
            if(*separators == '/')
                *separators = '\\';
        }

        mbstowcs(alert->file_name, file_name, MAX_PATH);

        free(file_name);
    }

    alert->location = def_error->location;
    alert->type     = type;

    switch(def_error->error)
    {
    case TSDEF_DEF_ERROR_INTERNAL:
        _snprintf(
                  alert->alert_text,
                  TSIDE_MAX_ALERT_LENGTH-1,
                  "%s[%d] Unrecoverable internal error",
                  type_text,
                  def_error->error
                 );

        break;

    case TSDEF_DEF_ERROR_SYNTAX:
        if(strcmp(def_error->unit_name, "_module_main") == 0)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Verify the invocation parameters '%s' are correct, and the the unit specified exists",
                      type_text,
                      def_error->error,
                      invocation_text
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Syntax error, verify the code typed at this location is properly formatted",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Incompatible types, cannot convert expression of type '%s' to variable '%s' of type '%s'",
                      type_text,
                      def_error->error,
                      type_names[def_error->info.data.incompatible_types.from_type],
                      def_error->info.data.incompatible_types.convert_to_name,
                      type_names[def_error->info.data.incompatible_types.to_type]
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Incompatible types",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_INVALID_USE_OF_OPERATOR:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Invalid use of operator '%s' with type '%s'",
                      type_text,
                      def_error->error,
                      operator_names[def_error->info.data.invalid_use_of_operator.operator],
                      type_names[def_error->info.data.invalid_use_of_operator.type]
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Invalid use of operator",
                      type_text,
                      def_error->error,
                      operator_names[def_error->info.data.invalid_use_of_operator.operator],
                      type_names[def_error->info.data.invalid_use_of_operator.type]
                     );
        }

        break;

    case TSDEF_DEF_ERROR_UNDEFINED_VARIABLE:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Use of undefined variable '%s'",
                      type_text,
                      def_error->error,
                      def_error->info.data.undefined_variable.name
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Use of undefined variable",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_FLOW_CONTROL_OUTSIDE_LOOP:
        _snprintf(
                  alert->alert_text,
                  TSIDE_MAX_ALERT_LENGTH-1,
                  "%s[%d] Flow control statement 'break' or 'continue' being used outside of loop",
                  type_text,
                  def_error->error
                 );

        break;

    case TSDEF_DEF_ERROR_WRONG_ARGUMENT_COUNT:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Function '%s' was passed %d arguments when %d were expected",
                      type_text,
                      def_error->error,
                      def_error->info.data.wrong_argument_count.name,
                      def_error->info.data.wrong_argument_count.passed_count,
                      def_error->info.data.wrong_argument_count.required_count
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Wrong number of arguments passed to function",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_VARIABLE_REDEFINITION:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Redefinition of variable '%s'",
                      type_text,
                      def_error->error,
                      def_error->info.data.variable_redefinition.name
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Variable redefinition",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_UNDEFINED_FUNCTION:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Function with name '%s' could not be found",
                      type_text,
                      def_error->error,
                      def_error->info.data.undefined_function.name
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Undefined function",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_USING_VOID_TYPE:
        _snprintf(
                  alert->alert_text,
                  TSIDE_MAX_ALERT_LENGTH-1,
                  "%s[%d] Functions which have no output cannot be used in an expression",
                  type_text,
                  def_error->error
                 );

        break;

    case TSDEF_DEF_ERROR_USING_DELAYED_TYPE:
        _snprintf(
                  alert->alert_text,
                  TSIDE_MAX_ALERT_LENGTH-1,
                  "%s[%d] Expression type could not be decided",
                  type_text,
                  def_error->error
                 );

        break;

    case TSDEF_DEF_ERROR_FUNCTION_REDEFINITION:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Function '%s' already defined",
                      type_text,
                      def_error->error,
                      def_error->info.data.function_redefinition.name
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Function redefinition",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_TYPE_NOT_STEPPABLE:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Unsteppable type '%s' used in for loop",
                      type_text,
                      def_error->error,
                      type_names[def_error->info.data.type_not_steppable.type]
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Unsteppable type used in for loop",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_FUNCTION_NOT_ACTIONABLE:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Function '%s' used in action but function is not actionable",
                      type_text,
                      def_error->error,
                      def_error->info.data.function_not_actionable.name
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Unactionable function used",
                      type_text,
                      def_error->error
                     );
        }

        break;

    case TSDEF_DEF_ERROR_FUNCTION_NOT_INVOCABLE:
        if(def_error->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] Function '%s' can only be used as part of an action",
                      type_text,
                      def_error->error,
                      def_error->info.data.function_not_invocable.name
                     );
        }
        else
        {
            _snprintf(
                      alert->alert_text,
                      TSIDE_MAX_ALERT_LENGTH-1,
                      "%s[%d] This function can only be used as part of an action",
                      type_text,
                      def_error->error
                     );
        }

        break;

    default:
        _snprintf(
                  alert->alert_text,
                  TSIDE_MAX_ALERT_LENGTH-1,
                  "%s[%d] Unknown error",
                  type_text,
                  def_error->error
                 );
    }

    alert->alert_text[TSIDE_MAX_ALERT_LENGTH-1] = 0;

    mbstowcs(alert->alert_text_w, alert->alert_text, TSIDE_MAX_ALERT_LENGTH);

    return alert;
}

struct tside_alert* TSIDE_AlertFromString (char* string, unsigned int type)
{
    char                unit_name[MAX_PATH];
    struct tside_alert* alert;
    unsigned int        location;
    int                 matched_parameters;

    alert = malloc(sizeof(struct tside_alert));
    if(alert == NULL)
        return NULL;

    matched_parameters = sscanf(string, "function=%s line=%d: ", unit_name, &location);

    if(matched_parameters == 2)
    {
        char* file_name;
        int   error;

        error = TSUtil_FindUnitFile(unit_name, ".ts", &tside_unit_paths, &file_name);
        if(error != TSUTIL_ERROR_NONE)
            alert->file_name[0] = 0;
        else
        {
            char* separators;

            for(separators = file_name; *separators != 0; separators++)
            {
                if(*separators == '/')
                    *separators = '\\';
            }

            mbstowcs(alert->file_name, file_name, MAX_PATH);

            free(file_name);

            alert->location = location;
        }

        string = strchr(string, ':');
        string += 2;
    }
    else
    {
        alert->file_name[0] = 0;
        alert->location     = 0;
    }

    alert->type = type;

    strncpy(alert->alert_text, string, TSIDE_MAX_ALERT_LENGTH);

    alert->alert_text[TSIDE_MAX_ALERT_LENGTH-1] = 0;

    mbstowcs(alert->alert_text_w, alert->alert_text, TSIDE_MAX_ALERT_LENGTH);

    return alert;
}

