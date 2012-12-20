/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_ALERTS_H_
#define _TSIDE_ALERTS_H_


#include <tsdef/deferror.h>

#include <windows.h>


#define TSIDE_ALERT_TYPE_COMPILE_ERROR   1
#define TSIDE_ALERT_TYPE_COMPILE_WARNING 2
#define TSIDE_ALERT_TYPE_RUNTIME_ERROR   3
#define TSIDE_ALERT_TYPE_RUNTIME_WARNING 4
#define TSIDE_ALERT_TYPE_RUNTIME_MESSAGE 5

#define TSIDE_MAX_ALERT_LENGTH 4096


struct tside_alert
{
    WCHAR        file_name[MAX_PATH];
    unsigned int location;
    unsigned int type;

    WCHAR alert_text_w[TSIDE_MAX_ALERT_LENGTH];
    char  alert_text[TSIDE_MAX_ALERT_LENGTH];
};


struct tside_alert* TSIDE_AlertFromDefError (struct tsdef_def_error*, char*);
struct tside_alert* TSIDE_AlertFromString   (char*, unsigned int);


#endif

