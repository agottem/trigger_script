/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_SELECT_H_
#define _TSIDE_SELECT_H_


#include <windows.h>


extern int  TSIDE_InitializeSelect (void);
extern void TSIDE_ShutdownSelect   (void);

extern INT_PTR CALLBACK TSIDE_SelectMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

