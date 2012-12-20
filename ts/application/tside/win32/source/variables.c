/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "variables.h"
#include "error.h"

#include <stdlib.h>
#include <malloc.h>
#include <string.h>


static int AddVariable (char*);


struct tside_variable* tside_set_variables;


static int AddVariable (char* assignment)
{
    struct tside_variable* variable;
    char*                  assignment_character;

    variable = malloc(sizeof(struct tside_variable));
    if(variable == NULL)
        goto allocate_variable_failed;

    variable->name = strdup(assignment);
    if(variable->name == NULL)
        goto allocate_variable_content_failed;

    assignment_character = strchr(variable->name, '=');
    if(assignment_character == NULL)
        goto invalid_variable_text;

    *assignment_character = 0;
    assignment_character++;

    variable->value = assignment_character;

    variable->next_variable = tside_set_variables;
    tside_set_variables     = variable;

    return TSIDE_ERROR_NONE;

invalid_variable_text:
    free(variable->name);
allocate_variable_content_failed:
    free(variable);
allocate_variable_failed:
    return TSIDE_ERROR_MEMORY;
}


int TSIDE_InitializeVariables (void)
{
    char* constant_env_data;
    char* env_data;
    int   error;

    tside_set_variables = NULL;

    constant_env_data = getenv("ts_variable");
    if(constant_env_data != NULL)
    {
        char* scan_env;
        char* start_env;

        env_data = strdup(constant_env_data);
        if(env_data == NULL)
            goto duplicate_env_failed;

        start_env = env_data;

        do
        {
            scan_env = strchr(start_env, ';');
            if(scan_env != NULL)
                *scan_env = 0;

            error = AddVariable(start_env);
            if(error != TSIDE_ERROR_NONE)
                goto env_add_variable_failed;

            if(scan_env == NULL)
                break;

            scan_env++;
            start_env = scan_env;
        }while(*start_env != 0);

        free(env_data);
    }

    return TSIDE_ERROR_NONE;

env_add_variable_failed:
    TSIDE_ShutdownVariables();
duplicate_env_failed:
    return TSIDE_ERROR_SYSTEM_CALL_FAILED;
}

void TSIDE_ShutdownVariables (void)
{
    struct tside_variable* variable;

    variable = tside_set_variables;
    while(variable != NULL)
    {
        struct tside_variable* free_variable;

        free_variable = variable;
        variable      = variable->next_variable;

        free(free_variable->name);
        free(free_variable);
    }
}

