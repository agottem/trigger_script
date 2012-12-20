/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSDEF_CONSTRUCT_H_
#define _TSDEF_CONSTRUCT_H_


#include <tsdef/def.h>
#include <tsdef/deferror.h>


extern int TSDef_ConstructUnitFromFile (
                                        char*,
                                        char*,
                                        struct tsdef_unit*,
                                        struct tsdef_def_error_list*
                                       );

extern int TSDef_ConstructUnitFromString (
                                          char*,
                                          char*,
                                          struct tsdef_unit*,
                                          struct tsdef_def_error_list*
                                         );


#endif

