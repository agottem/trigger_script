/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSFFI_EXECIF_H_
#define _TSFFI_EXECIF_H_


#include <stdlib.h>


#define TSFFI_ALERT_MESSAGE 0
#define TSFFI_ALERT_WARNING 1
#define TSFFI_ALERT_ERROR   2


struct tsffi_execif
{
    void (*signal_action) (void*);

    void (*alert)              (void*, unsigned int, char*);
    void (*set_exception_text) (void*, char*);

    void* (*allocate_memory) (void*, size_t);
    void  (*free_memory)     (void*, void*);
};


#endif

