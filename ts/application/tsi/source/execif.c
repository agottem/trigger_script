/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "execif.h"

#include <stdio.h>
#include <stdlib.h>


static void Alert            (void*, unsigned int, char*);
static void SetExceptionText (void*, char*);


struct tsffi_execif tsi_execif = {NULL, &Alert, &SetExceptionText, NULL, NULL};


static void Alert (void* execif_data, unsigned int severity, char* text)
{
    switch(severity)
    {
    case TSFFI_ALERT_MESSAGE:
        printf("TS plugin message: ");

        break;

    case TSFFI_ALERT_WARNING:
        printf("TS plugin warning: ");

        break;

    case TSFFI_ALERT_ERROR:
        printf("TS plugin error: ");

        break;
    }

    printf("%s\n", text);
}

static void SetExceptionText (void* execif_data, char* text)
{
    printf("TS plugin exception text: %s\n", text);
}

