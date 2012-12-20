/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_GUI_H_
#define _TSIDE_GUI_H_


#include <windows.h>


extern HWND tside_ide_window;
extern HWND tside_run_window;
extern HWND tside_reference_window;


extern int  TSIDE_InitializeGUI (void);
extern void TSIDE_ShutdownGUI   (void);

extern int TSIDE_GUIMain (void);


#endif

