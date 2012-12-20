/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_REF_PLUGINS_H_
#define _TSIDE_REF_PLUGINS_H_


#include <windows.h>


extern int  TSIDE_InitializeRefPlugins (void);
extern void TSIDE_ShutdownRefPlugins   (void);

extern INT_PTR CALLBACK TSIDE_RefPluginsMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

