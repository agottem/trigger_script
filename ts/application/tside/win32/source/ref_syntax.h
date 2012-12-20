/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_REF_SYNTAX_H_
#define _TSIDE_REF_SYNTAX_H_


#include <windows.h>


struct tside_syntax_data
{
    char* content;

    WCHAR syntax_name[MAX_PATH];

    struct tside_syntax_data* next_syntax;
};


extern struct tside_syntax_data* tside_found_syntax;


extern int  TSIDE_InitializeRefSyntax (void);
extern void TSIDE_ShutdownRefSyntax   (void);

extern INT_PTR CALLBACK TSIDE_RefSyntaxMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

