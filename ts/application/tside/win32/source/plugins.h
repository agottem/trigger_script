/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_PLUGINS_H_
#define _TSIDE_PLUGINS_H_


#include <tsffi/register.h>


struct tside_registered_plugin
{
    char* path;

    tsffi_register  register_function;
    tsffi_configure configure_function;

    struct tsffi_registration_group* groups;
    unsigned int                     count;

    struct tside_registered_plugin* next_plugin;
};


extern struct tside_registered_plugin* tside_available_plugins;


extern int  TSIDE_InitializePlugins (void);
extern void TSIDE_ShutdownPlugins   (void);


#endif

