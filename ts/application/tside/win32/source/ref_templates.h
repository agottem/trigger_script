/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_REF_TEMPLATES_H_
#define _TSIDE_REF_TEMPLATES_H_


#include <windows.h>


struct tside_template_data
{
    char* content;

    WCHAR category_name[MAX_PATH];
    WCHAR template_name[MAX_PATH];

    struct tside_template_data* next_category_template;
    struct tside_template_data* next_template;
};


extern struct tside_template_data* tside_found_templates;


extern int  TSIDE_InitializeRefTemplates (void);
extern void TSIDE_ShutdownRefTemplates   (void);

extern INT_PTR CALLBACK TSIDE_RefTemplatesMessageProc (HWND, UINT, WPARAM, LPARAM);


#endif

