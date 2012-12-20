/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_EDITOR_H_
#define _TSIDE_EDITOR_H_


#include <windows.h>

#include "alerts.h"


#define TSIDE_NO_USER_TABS  0
#define TSIDE_HAS_USER_TABS 1

#define TSIDE_FILES_UNSAVED    0
#define TSIDE_FILES_UP_TO_DATE 1


extern int  TSIDE_InitializeEditor (void);
extern void TSIDE_ShutdownEditor   (void);

extern unsigned int TSIDE_HasUserTabs     (HWND);
extern int          TSIDE_CheckFilesSaved (HWND);
extern unsigned int TSIDE_HasUnsavedFiles (HWND);

extern void TSIDE_GetRunnableSelection (WCHAR*, HWND);

extern void TSIDE_OpenNewFile       (HWND);
extern void TSIDE_OpenExistingFile  (HWND);
extern void TSIDE_SaveCurrentFile   (HWND);
extern void TSIDE_SaveCurrentFileAs (HWND);
extern void TSIDE_SaveAll           (HWND);
extern void TSIDE_CloseCurrentFile  (HWND);
extern void TSIDE_CloseAll          (HWND);

extern void TSIDE_ToggleSearchText (HWND);
extern void TSIDE_ToggleSearchGoto (HWND);

extern void TSIDE_GotoLineNumber     (HWND);
extern void TSIDE_SearchFindNext     (HWND);
extern void TSIDE_SearchFindPrevious (HWND);
extern void TSIDE_SearchReplaceNext  (HWND);
extern void TSIDE_SearchReplaceAll   (HWND);

extern void TSIDE_EditUndo      (HWND);
extern void TSIDE_EditRedo      (HWND);
extern void TSIDE_EditCut       (HWND);
extern void TSIDE_EditCopy      (HWND);
extern void TSIDE_EditPaste     (HWND);
extern void TSIDE_EditSelectAll (HWND);

extern void TSIDE_InsertText  (char*, HWND);
extern void TSIDE_UseTemplate (char*, HWND);

extern void TSIDE_AnnotateAlert    (struct tside_alert*, HWND);
extern void TSIDE_GotoAlert        (struct tside_alert*, HWND);
extern void TSIDE_ClearAnnotations (HWND);

extern void TSIDE_HighlightLocation (WCHAR*, unsigned int, unsigned int, HWND);

extern void TSIDE_SetReadOnlyMode (unsigned int, HWND);

extern INT_PTR CALLBACK TSIDE_EditorMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

