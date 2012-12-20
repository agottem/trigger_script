/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSFFI_REGISTER_H_
#define _TSFFI_REGISTER_H_


#include <tsffi/function.h>
#include <tsffi/execif.h>


#define TSFFI_REGISTER_FUNCTION_NAME  "RegisterTSFFI"
#define TSFFI_CONFIGURE_FUNCTION_NAME "ConfigureTSFFI"

#define TSFFI_MODULE_SLEEPING 0
#define TSFFI_MODULE_RUNNING  1


struct tsffi_registration_group;

typedef void (*tsffi_register)  (struct tsffi_registration_group**, unsigned int*);
typedef void (*tsffi_configure) (char*, char*);

typedef int  (*tsffi_begin_module) (
                                    struct tsffi_execif*,
                                    void*,
                                    struct tsffi_registration_group*,
                                    void**
                                   );
typedef int  (*tsffi_module_state) (
                                    struct tsffi_execif*,
                                    void*,
                                    unsigned int,
                                    struct tsffi_registration_group*,
                                    void*
                                   );
typedef void (*tsffi_end_module)   (
                                    struct tsffi_execif*,
                                    void*,
                                    int,
                                    struct tsffi_registration_group*,
                                    void*
                                   );

struct tsffi_function_definition
{
    char* name;
    char* documentation;

    tsffi_function          function;
    tsffi_action_controller action_controller;
    unsigned int            output_type;
    unsigned int            argument_count;
    unsigned int            argument_types[TSFFI_MAX_INPUT_ARGUMENTS];
};

struct tsffi_registration_group
{
    unsigned int                      function_count;
    struct tsffi_function_definition* functions;

    tsffi_begin_module begin_function;
    tsffi_module_state state_function;
    tsffi_end_module   end_function;

    void* user_data;
};


#endif

