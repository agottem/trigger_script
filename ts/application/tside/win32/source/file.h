/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_FILE_H_
#define _TSIDE_FILE_H_


#include <windows.h>


extern int TSIDE_GetFileText  (WCHAR*, char**);
extern int TSIDE_SaveFileText (WCHAR*, char*);


#endif

