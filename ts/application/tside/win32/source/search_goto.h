/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_SEARCH_GOTO_H_
#define _TSIDE_SEARCH_GOTO_H_


#include <windows.h>


extern int  TSIDE_InitializeSearchGoto (void);
extern void TSIDE_ShutdownSearchGoto   (void);

extern int TSIDE_GetGotoLineNumber (HWND);

extern INT_PTR CALLBACK TSIDE_SearchGotoMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

