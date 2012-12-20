/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_SEARCH_TEXT_H_
#define _TSIDE_SEARCH_TEXT_H_


#include <windows.h>


#define TSIDE_MAX_SEARCH_LENGTH 4096


extern int  TSIDE_InitializeSearchText (void);
extern void TSIDE_ShutdownSearchText   (void);

extern void TSIDE_GetFindText    (HWND, char*);
extern void TSIDE_GetReplaceText (HWND, char*);

extern INT_PTR CALLBACK TSIDE_SearchTextMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

