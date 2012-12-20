/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "main.h"
#include "error.h"
#include "controller.h"
#include "execif.h"
#include "register.h"

#include <tsdef/def.h>
#include <tsdef/deferror.h>
#include <tsdef/module.h>
#include <tsdef/error.h>
#include <tsutil/compile.h>
#include <tsutil/error.h>
#include <tsint/error.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <signal.h>


static int ProcessCommandLine (int argument_count, char* arguments[]);

static int  AddVariable      (char*);
static void DestroyVariables (void);

static void NotifyLookup (char*);

static void NullSignalHandler  (int);
static void AbortSignalHandler (int);


static struct tsint_module_abort_signal* abort_signal;

struct tsdef_module           tsi_module;
struct tsutil_path_collection tsi_search_paths;
char*                         tsi_unit_invocation;
struct tsi_variable*          tsi_set_variables;
unsigned int                  tsi_flags;


int main (int argument_count, char* argument_list[])
{
    struct tsdef_def_error_list def_errors;
    char*                       program_directory;
    char*                       program_path;
    int                         error;
    errno_t                     std_error;

    signal(SIGINT, NullSignalHandler);

    printf(
           "\n"
           "Launching Trigger Script Interpreter\n"
           "------------------------------------\n"
          );

    TSUtil_InitializePathCollection(&tsi_search_paths);

    error = TSUtil_AppendPath(".", &tsi_search_paths);
    if(error != TSUTIL_ERROR_NONE)
    {
        printf("\nAn unexpected error has occurred and the interpretter must now exit\n");

        goto append_path_failed;
    }

    TSDef_InitializeModule(&tsi_module);

    tsi_unit_invocation = NULL;
    tsi_set_variables   = NULL;
    tsi_flags           = 0;

    error = ProcessCommandLine(argument_count, argument_list);
    if(error < 0)
        goto process_command_line_failed;
    else if(error > 0)
        goto exit_gracefully;

    if(tsi_unit_invocation == NULL)
    {
        printf("\nNo function was specified, please invoke the interpretter with a function to be executed\n");

        goto no_unit_specified;
    }

#ifdef PLATFORM_WIN32
    std_error = _get_pgmptr(&program_path);
    if(std_error != 0)
        goto get_program_path_failed;

    program_directory = strdup(program_path);
    if(program_directory == NULL)
    {
        printf("\nAn unexpected error has occurred and the interpretter must now exit\n");

        goto allocate_program_directory_failed;
    }

    program_path = strrchr(program_directory, '\\');
    if(program_path != NULL)
        *program_path = 0;
#else
    #error "Unsupported platform"
#endif

    error = TSI_RegisterFFI(program_directory, &tsi_module);

    free(program_directory);

    if(error != TSI_ERROR_NONE)
    {
        printf("\nAn unexpected error has occurred and the interpretter must now exit\n");

        goto register_ffi_failed;
    }

    printf("\n");

    TSDef_InitializeDefErrorList(&def_errors);

    printf("Compiling trigger script...\n");

    error = TSUtil_CompileUnit(
                               tsi_unit_invocation,
                               0,
                               &tsi_search_paths,
                               TSI_SOURCE_EXTENSION,
                               &NotifyLookup,
                               &def_errors,
                               &tsi_module
                              );
    if(error != TSDEF_ERROR_NONE)
    {
        if(error == TSUTIL_ERROR_COMPILATION_ERROR || error == TSUTIL_ERROR_COMPILATION_WARNING)
            TSI_ReportDefErrors(&def_errors);
        else
            printf("An unexpected error has while trying to parse the unit\n");

        if(error != TSUTIL_ERROR_COMPILATION_WARNING)
            goto compilation_failed;
    }

    TSDef_DestroyDefErrorList(&def_errors);

    printf("Compilation successful\n");

    if(!(tsi_flags&TSI_FLAG_COMPILE_ONLY))
    {
        printf("\n");

        if(error == TSUTIL_ERROR_COMPILATION_WARNING)
        {
            int pressed_char;

            printf("Warnings were encountered, run anyways? (y/n)> ");

            do
            {
                pressed_char = getch();
                pressed_char = tolower(pressed_char);

                if(isalpha(pressed_char))
                    putchar(pressed_char);
            }while(pressed_char != 'y' && pressed_char != 'n');

            printf("\n");

            if(pressed_char == 'y')
                error = TSUTIL_ERROR_NONE;
        }

        if(error == TSUTIL_ERROR_NONE)
        {
            struct tsint_controller_data controller;
            struct tsint_execif_data     execif;

            controller.function  = &TSI_Controller;
            controller.user_data = NULL;

            execif.execif    = &tsi_execif;
            execif.user_data = NULL;

            printf("Executing trigger script...\n");

            error = TSInt_AllocAbortSignal(&abort_signal);
            if(error != TSINT_ERROR_NONE)
            {
                printf("\nAn unexpected error has occurred and the interpretter must now exit\n");

                goto alloc_abort_signal_failed;
            }

            signal(SIGINT, AbortSignalHandler);

            error = TSInt_InterpretModule(
                                          &tsi_module,
                                          NULL,
                                          NULL,
                                          &controller,
                                          &execif,
                                          abort_signal
                                         );
            if(error != TSINT_ERROR_NONE)
                printf("Trigger script halted due to exception\n");

            signal(SIGINT, NullSignalHandler);

            TSInt_FreeAbortSignal(abort_signal);
        }
    }

exit_gracefully:
    DestroyVariables();
    TSDef_DestroyModule(&tsi_module);
    TSUtil_DestroyPathCollection(&tsi_search_paths);

    return 0;

compilation_failed:
    TSDef_DestroyDefErrorList(&def_errors);
alloc_abort_signal_failed:
register_ffi_failed:
allocate_program_directory_failed:
get_program_path_failed:
process_command_line_failed:
    DestroyVariables();
no_unit_specified:
    TSDef_DestroyModule(&tsi_module);
append_path_failed:
    TSUtil_DestroyPathCollection(&tsi_search_paths);

    return -1;
}


static int ProcessCommandLine (int argument_count, char* argument_list[])
{
    char** scan_arguments;
    char*  constant_env_data;
    char*  env_data;
    int    remaining_argument_count;

    argument_count--;
    argument_list++;

    if(argument_count == 0)
        goto print_help;

    remaining_argument_count = argument_count;
    scan_arguments           = argument_list;

    while(remaining_argument_count--)
    {
        char* argument;

        argument = *scan_arguments;
        scan_arguments++;

        if(strncmp(argument, "-P", sizeof("-P")-1) == 0)
            continue;
        else if(strncmp(argument, "-I", sizeof("-I")-1) == 0)
        {
            int error;

            error = TSUtil_AppendPath(&argument[sizeof("-I")-1], &tsi_search_paths);
            if(error != TSUTIL_ERROR_NONE)
                goto append_path_failed;
        }
        else if(strncmp(argument, "-v", sizeof("-v")-1) == 0)
        {
            int error;

            error = AddVariable(&argument[sizeof("-v")-1]);
            if(error != 0)
                goto add_variable_failed;
        }
        else if(strncmp(argument, "-c", sizeof("-c")-1) == 0)
            tsi_flags |= TSI_FLAG_COMPILE_ONLY;
        else if(strncmp(argument, "-d", sizeof("-d")-1) == 0)
            tsi_flags |= TSI_FLAG_DEBUG;
        else if(strcmp(argument, "--help") == 0)
            goto print_help;
        else
        {
            if(isalpha(*argument) == 0)
            {
                printf("\nInvalid command line option '%s', proper usage described below\n", argument);

                goto print_help;
            }

            tsi_unit_invocation = argument;
        }
    }

    constant_env_data = getenv("ts_include");
    if(constant_env_data != NULL)
    {
        char* scan_env;
        char* start_env;
        int   error;

        env_data = strdup(constant_env_data);
        if(env_data == NULL)
            goto duplicate_env_failed;

        start_env = env_data;

        do
        {
            scan_env = strchr(start_env, ';');
            if(scan_env != NULL)
                *scan_env = 0;

            error = TSUtil_AppendPath(start_env, &tsi_search_paths);
            if(error != TSUTIL_ERROR_NONE)
                goto env_append_path_failed;

            if(scan_env == NULL)
                break;

            scan_env++;
            start_env = scan_env;
        }while(*start_env != 0);

        free(env_data);
    }

    constant_env_data = getenv("ts_variable");
    if(constant_env_data != NULL)
    {
        char* scan_env;
        char* start_env;
        int   error;

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
            if(error != 0)
                goto env_add_variable_failed;

            if(scan_env == NULL)
                break;

            scan_env++;
            start_env = scan_env;
        }while(*start_env != 0);

        free(env_data);
    }

    remaining_argument_count = argument_count;
    scan_arguments           = argument_list;

    while(remaining_argument_count--)
    {
        char* argument;

        argument = *scan_arguments;
        scan_arguments++;

        if(strncmp(argument, "-P", sizeof("-P")-1) == 0)
        {
            int error;

            error = TSI_RegisterFFI(&argument[sizeof("-P")-1], &tsi_module);
            if(error != TSI_ERROR_NONE)
                goto register_ffi_failed;
        }
    }

    constant_env_data = getenv("ts_plugin");
    if(constant_env_data != NULL)
    {
        char* scan_env;
        char* start_env;
        int   error;

        env_data = strdup(constant_env_data);
        if(env_data == NULL)
            goto duplicate_env_failed;

        start_env = env_data;

        do
        {
            scan_env = strchr(start_env, ';');
            if(scan_env != NULL)
                *scan_env = 0;

            error = TSI_RegisterFFI(start_env, &tsi_module);
            if(error != TSI_ERROR_NONE)
                goto env_register_ffi_failed;

            if(scan_env == NULL)
                break;

            scan_env++;
            start_env = scan_env;
        }while(*start_env != 0);

        free(env_data);
    }

    return 0;

env_register_ffi_failed:
env_add_variable_failed:
env_append_path_failed:
    free(env_data);
duplicate_env_failed:
register_ffi_failed:
add_variable_failed:
append_path_failed:
    printf("\nAn error has occurred while processing the supplied command line, verify the syntax is correct\n");

    return -1;

print_help:
    printf(
           "\n"
           "Usage: tsi [options] function(...)\n"
           "Options:\n"
           "    --help\t\tDisplay this information\n"
           "    -I<path>\t\tSpecify a path to search when resolving functions\n"
           "    -P<path>\t\tSpeciy a path containg TS plugins to be loaded\n"
           "    -v<name>=<value>\tSpecify a variable to be communicated to all loaded TS plugins\n"
           "    -c\t\t\tCompile but don't execute\n"
           "    -d\t\t\tStep into source and debug upon beginning execution \n"
           "\n"
           "Options specifying search paths are listed in priority order.  Paths listed first will\n"
           " be searched first.  If multiple functions are specified, the last specified function\n"
           " is chosen.\n"
           "\n"
           "Search paths and plugin variables can be set using an environment variable and will be\n"
           " inheritted by tsi upon startup.  Supported environment variables are listed below.\n"
           "    ts_include\t\tA semicolon seperated list of paths used to resolve functions\n"
           "    ts_plugin\t\tA semicolon seperated list of paths containing plugins to be loaded\n"
           "    ts_variable\t\tA semicolon seperated list of variable assignments to be\n"
           "               \t\tcommunicated to TS plugins\n"
           "\n"
           "About:\n"
           "    The TS language, compiler, and interpreter, along with the\n"
           "    corresponding TS command line interface and TS IDE, were\n"
           "    architected and implemented by Andrew Gottemoller between\n"
           "    late March and early June of 2011.\n"
           "\n"
           "License:\n"
           "    Copyright 2011 Andrew Gottemoller.\n"
           "\n"
           "    Trigger Script is free software: you can redistribute it and/or modify\n"
           "    it under the terms of the GNU General Public License as published by\n"
           "    the Free Software Foundation, either version 3 of the License, or\n"
           "    (at your option) any later version.\n"
           "\n"
           "    Trigger Script is distributed in the hope that it will be useful,\n"
           "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
           "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
           "    GNU General Public License for more details.\n"
           "\n"
           "    You should have received a copy of the GNU General Public License\n"
           "    along with Trigger Script.  If not, see <http://www.gnu.org/licenses/>.\n"
           "\n"
           "Examples:\n"
           "    tsi -Imy_funcs/ -Iothers_funcs/ -Pextra_plugins/ -vusername=agottem my_function()\n"
           "    tsi -d -vmode=xt \"my_function(5, 9.0)\"\n"
           "    tsi -c \"my_function(\\\"hello world\\\")\"\n"
           "\n"
          );

    return 1;
}

static int AddVariable (char* assignment)
{
    struct tsi_variable* variable;
    char*                assignment_character;

    variable = malloc(sizeof(struct tsi_variable));
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

    variable->next_variable = tsi_set_variables;
    tsi_set_variables       = variable;

    return 0;

invalid_variable_text:
    free(variable->name);
allocate_variable_content_failed:
    free(variable);
allocate_variable_failed:
    return -1;
}

static void DestroyVariables (void)
{
    struct tsi_variable* variable;

    variable = tsi_set_variables;
    while(variable != NULL)
    {
        struct tsi_variable* free_variable;

        free_variable = variable;
        variable      = variable->next_variable;

        free(free_variable->name);
        free(free_variable);
    }
}


static void NotifyLookup (char* lookup_name)
{
    printf("Looking up dependency: '%s(...)'\n", lookup_name);
}

static void NullSignalHandler (int event)
{
    printf("Ctrl-C detected, module not yet executing so signal will be ignored\n");
}

static void AbortSignalHandler (int event)
{
    printf("Ctrl-C detected, aborting module execution...\n");

    TSInt_SignalAbort(abort_signal);
}

