/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_PATHS_H_
#define _TSIDE_PATHS_H_


#include <windows.h>
#include <tsutil/path.h>


extern char  tside_application_path[];
extern WCHAR tside_application_path_w[];

extern char  tside_user_plugin_path[];
extern WCHAR tside_user_plugin_path_w[];

extern char tside_default_units_path[];
extern WCHAR tside_default_units_path_w[];

extern char  tside_user_units_path[];
extern WCHAR tside_user_units_path_w[];

extern char  tside_syntax_path[];
extern WCHAR tside_syntax_path_w[];

extern char  tside_templates_path[];
extern WCHAR tside_templates_path_w[];

extern struct tsutil_path_collection tside_plugin_paths;
extern struct tsutil_path_collection tside_unit_paths;


extern int  TSIDE_InitializePaths (void);
extern void TSIDE_ShutdownPaths   (void);


#endif

