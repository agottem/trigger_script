/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSDEF_RESOLVE_H_
#define _TSDEF_RESOLVE_H_


#include <tsdef/def.h>
#include <tsdef/deferror.h>
#include <tsdef/module.h>
#include <tsdef/arguments.h>


typedef int (*tsdef_module_object_lookup) (
                                           char*,
                                           struct tsdef_argument_types*,
                                           void*,
                                           struct tsdef_module_object**
                                          );


extern int TSDef_ResolveUnit (
                              struct tsdef_unit*,
                              struct tsdef_argument_types*,
                              unsigned int,
                              struct tsdef_module*,
                              tsdef_module_object_lookup,
                              void*,
                              struct tsdef_def_error_list*
                             );


#endif

