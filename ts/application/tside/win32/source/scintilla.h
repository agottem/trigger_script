/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_SCINTILLA_H_
#define _TSIDE_SCINTILLA_H_


#include <windows.h>


#define TSIDE_SCINTILLA_STYLE_BOLD      0x01
#define TSIDE_SCINTILLA_STYLE_ITALIC    0x02
#define TSIDE_SCINTILLA_STYLE_UNDERLINE 0x04

#define TSIDE_SCINTILLA_CONFIG_SHOW_LINE_NUMBERS 0x01
#define TSIDE_SCINTILLA_CONFIG_TABS_AS_SPACES    0x02

#define TSIDE_TEXT_UNMODIFIED 0
#define TSIDE_TEXT_MODIFIED   1

#define TSIDE_TEXT_NOT_FOUND 0
#define TSIDE_TEXT_FOUND     1

#define TSIDE_HIGHLIGHT_NONE      0
#define TSIDE_HIGHLIGHT_EXECUTION 1

#define TSIDE_READONLY 0
#define TSIDE_WRITABLE 1


struct tside_scintilla_style
{
    char*        font;
    int          point_size;
    unsigned int flags;
    COLORREF     background;
    COLORREF     foreground;
};

struct tside_scintilla_configuration
{
    unsigned int flags;

    unsigned int indent_size;

    COLORREF execution_foreground_color;
    COLORREF execution_background_color;

    struct tside_scintilla_style default_style;
    struct tside_scintilla_style line_number_style;
    struct tside_scintilla_style brace_highlight_style;
    struct tside_scintilla_style brace_mismatch_style;
    struct tside_scintilla_style comment_style;
    struct tside_scintilla_style number_style;
    struct tside_scintilla_style keyword_style;
    struct tside_scintilla_style string_style;
    struct tside_scintilla_style operator_style;
    struct tside_scintilla_style identifier_style;
    struct tside_scintilla_style plugin_function_style;
    struct tside_scintilla_style user_defined_function_style;
    struct tside_scintilla_style alert_compile_error_style;
    struct tside_scintilla_style alert_compile_warning_style;
    struct tside_scintilla_style alert_runtime_error_style;
    struct tside_scintilla_style alert_runtime_warning_style;
    struct tside_scintilla_style alert_runtime_message_style;
};


extern struct tside_scintilla_configuration tside_default_configuration;


extern int  TSIDE_InitializeScintilla (void);
extern void TSIDE_ShutdownScintilla   (void);

extern int TSIDE_ConfigureScintilla (struct tside_scintilla_configuration*, HWND);

extern void         TSIDE_SetScintillaText         (char*, HWND);
extern unsigned int TSIDE_ScintillaHasModifiedText (HWND);
extern int          TSIDE_GetScintillaText         (char**, HWND);
extern void         TSIDE_ResetModifiedState       (HWND);

extern void TSIDE_GotoScintillaLine (int, HWND);

extern unsigned int TSIDE_SearchNextScintillaText (int, char*, char*, HWND);
extern unsigned int TSIDE_SearchPrevScintillaText (int, char*, char*, HWND);

extern void TSIDE_ScintillaUndo      (HWND);
extern void TSIDE_ScintillaRedo      (HWND);
extern void TSIDE_ScintillaCut       (HWND);
extern void TSIDE_ScintillaCopy      (HWND);
extern void TSIDE_ScintillaPaste     (HWND);
extern void TSIDE_ScintillaSelectAll (HWND);

extern void TSIDE_ScintillaInsertText (char*, HWND);

extern int  TSIDE_AnnotateScintilla         (char*, unsigned int, unsigned int, HWND);
extern void TSIDE_ClearScintillaAnnotations (HWND);

extern void TSIDE_HighlightScintillaLine (unsigned int, unsigned int, HWND);

extern void TSIDE_SetScintillaReadOnly (unsigned int, HWND);


#endif

