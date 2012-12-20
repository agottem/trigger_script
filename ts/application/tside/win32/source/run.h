/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_RUN_H_
#define _TSIDE_RUN_H_


#include <windows.h>


extern int  TSIDE_InitializeRun (void);
extern void TSIDE_ShutdownRun   (void);

extern void TSIDE_SetRunData (WCHAR*, unsigned int);

extern void TSIDE_SetRunControllerMode (unsigned int);

extern void TSIDE_PrintVariables (void);
extern void TSIDE_AbortRun       (void);

extern INT_PTR CALLBACK TSIDE_RunMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

