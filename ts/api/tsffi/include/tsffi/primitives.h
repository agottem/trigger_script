/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSFFI_PRIMITIVES_H_
#define _TSFFI_PRIMITIVES_H_


#define TSFFI_PRIMITIVE_TYPE_VOID    0
#define TSFFI_PRIMITIVE_TYPE_BOOL    1
#define TSFFI_PRIMITIVE_TYPE_INT     2
#define TSFFI_PRIMITIVE_TYPE_REAL    3
#define TSFFI_PRIMITIVE_TYPE_STRING  4

#define TSFFI_BOOL_FALSE 0
#define TSFFI_BOOL_TRUE  1


typedef char   tsffi_bool;
typedef int    tsffi_int;
typedef double tsffi_real;
typedef char*  tsffi_string;


#endif

