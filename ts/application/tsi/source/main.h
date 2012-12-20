/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSI_MAIN_H_
#define _TSI_MAIN_H_


#include <tsint/module.h>
#include <tsutil/path.h>


#define TSI_SOURCE_EXTENSION ".ts"

#define TSI_FLAG_COMPILE_ONLY 0x01
#define TSI_FLAG_DEBUG        0x02


struct tsi_variable
{
    char* name;
    char* value;

    struct tsi_variable* next_variable;
};


extern struct tsdef_module           tsi_module;
extern struct tsutil_path_collection tsi_search_paths;
extern char*                         tsi_unit_invocation;
extern struct tsi_variable*          tsi_set_variables;
extern unsigned int                  tsi_flags;


#endif

