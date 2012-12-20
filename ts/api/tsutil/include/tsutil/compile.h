/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSUTIL_COMPILE_H_
#define _TSUTIL_COMPILE_H_


#include <tsdef/module.h>
#include <tsdef/deferror.h>
#include <tsutil/path.h>


#define TSUTIL_COMPILE_FLAG_CAPTURE_OUTPUT 0x01


typedef void (*notify_lookup_function) (char*);


extern int TSUtil_CompileUnit (
                               char*,
                               unsigned int,
                               struct tsutil_path_collection*,
                               char*,
                               notify_lookup_function,
                               struct tsdef_def_error_list*,
                               struct tsdef_module*
                              );


#endif

