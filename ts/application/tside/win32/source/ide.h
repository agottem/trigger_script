/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_IDE_H_
#define _TSIDE_IDE_H_


#include <windows.h>


#define TSIDE_NOTIFY_DOCUMENT_MODIFIED 1
#define TSIDE_NOTIFY_RUN_FINISHED      2
#define TSIDE_NOTIFY_RUN_COMPILING     3
#define TSIDE_NOTIFY_RUN_EXECUTING     4
#define TSIDE_NOTIFY_RUN_CLOSED        5

#define TSIDE_MESSAGE_RUN_BEGINNING    WM_USER+100
#define TSIDE_MESSAGE_RUN_ALERT        WM_USER+101
#define TSIDE_MESSAGE_RUN_CLEAR_ALERTS WM_USER+102
#define TSIDE_MESSAGE_RUN_GOTO_ALERT   WM_USER+103
#define TSIDE_MESSAGE_RUN_BREAK        WM_USER+104
#define TSIDE_MESSAGE_INSERT_TEXT      WM_USER+105
#define TSIDE_MESSAGE_USE_TEMPLATE     WM_USER+106


extern int  TSIDE_InitializeIDE (void);
extern void TSIDE_ShutdownIDE   (void);

extern void TSIDE_PresentIDE (void);

extern INT_PTR CALLBACK TSIDE_IDEMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

