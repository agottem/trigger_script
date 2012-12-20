/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSI_ERROR_H_
#define _TSI_ERROR_H_


#include <tsdef/deferror.h>


#define TSI_ERROR_NONE     0
#define TSI_ERROR_FAILURE -1


extern void TSI_ReportDefErrors (struct tsdef_def_error_list*);


#endif

