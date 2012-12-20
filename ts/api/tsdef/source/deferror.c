/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsdef/deferror.h>
#include <tsdef/error.h>

#include <stdlib.h>
#include <malloc.h>
#include <string.h>


void TSDef_InitializeDefErrorList (struct tsdef_def_error_list* error_list)
{
    error_list->encountered_errors = NULL;
    error_list->encountered_errors = NULL;
    error_list->error_count        = 0;
    error_list->warning_count      = 0;
}

int TSDef_AppendDefErrorList (
                              int                          error,
                              char*                        unit_name,
                              unsigned int                 location,
                              unsigned int                 flags,
                              struct tsdef_def_error_info* info,
                              struct tsdef_def_error_list* error_list
                             )
{
    struct tsdef_def_error* error_report;

    error_report = malloc(sizeof(struct tsdef_def_error));
    if(error_report == NULL)
        goto allocate_def_error_failed;

    error_report->error     = error;
    error_report->unit_name = strdup(unit_name);
    if(error_report->unit_name == NULL)
        goto duplicate_unit_name_failed;

    error_report->location = location;
    error_report->flags    = flags;

    if(info != NULL)
    {
        error_report->flags |= TSDEF_DEF_ERROR_FLAG_INFO_VALID;
        error_report->info   = *info;
    }

    error_report->next_error = NULL;

    if(error_list->encountered_errors != NULL)
        error_list->last_error->next_error = error_report;
    else
        error_list->encountered_errors = error_report;

    error_list->last_error = error_report;

    if(flags&TSDEF_DEF_ERROR_FLAG_WARNING)
        error_list->warning_count++;
    else
        error_list->error_count++;

    return TSDEF_ERROR_NONE;

duplicate_unit_name_failed:
    free(error_report);

allocate_def_error_failed:
    return TSDEF_ERROR_MEMORY;
}

void TSDef_DestroyDefErrorList (struct tsdef_def_error_list* error_list)
{
    struct tsdef_def_error* free_error;
    struct tsdef_def_error* errors;

    errors = error_list->encountered_errors;
    while(errors != NULL)
    {
        free(errors->unit_name);

        if(errors->flags&TSDEF_DEF_ERROR_FLAG_INFO_VALID)
        {
            switch(errors->error)
            {
            case TSDEF_DEF_ERROR_INCOMPATIBLE_TYPES:
                free(errors->info.data.incompatible_types.convert_to_name);

                break;

            case TSDEF_DEF_ERROR_UNDEFINED_VARIABLE:
                free(errors->info.data.undefined_variable.name);

                break;

            case TSDEF_DEF_ERROR_WRONG_ARGUMENT_COUNT:
                free(errors->info.data.wrong_argument_count.name);

                break;

            case TSDEF_DEF_ERROR_VARIABLE_REDEFINITION:
                free(errors->info.data.variable_redefinition.name);

                break;

            case TSDEF_DEF_ERROR_UNDEFINED_FUNCTION:
                free(errors->info.data.undefined_function.name);

                break;

            case TSDEF_DEF_ERROR_FUNCTION_REDEFINITION:
                free(errors->info.data.function_redefinition.name);

                break;

            case TSDEF_DEF_ERROR_FUNCTION_NOT_ACTIONABLE:
                free(errors->info.data.function_not_actionable.name);

                break;

            case TSDEF_DEF_ERROR_FUNCTION_NOT_INVOCABLE:
                free(errors->info.data.function_not_invocable.name);

                break;

            default:
                break;
            }
        }

        free_error = errors;
        errors     = errors->next_error;

        free(free_error);
    }
}

