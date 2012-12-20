/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_REFERENCE_H_
#define _TSIDE_REFERENCE_H_


#include <windows.h>


#define TSIDE_REFERENCE_SYNTAX    0
#define TSIDE_REFERENCE_PLUGINS   1
#define TSIDE_REFERENCE_TEMPLATES 2


extern int  TSIDE_InitializeReference (void);
extern void TSIDE_ShutdownReference   (void);

extern void TSIDE_ActivateReference (unsigned int);

extern INT_PTR CALLBACK TSIDE_ReferenceMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

