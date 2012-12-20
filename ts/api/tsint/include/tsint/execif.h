/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_EXECIF_H_
#define _TSINT_EXECIF_H_


#include <tsffi/execif.h>


struct tsint_execif_data
{
    struct tsffi_execif* execif;
    void*                user_data;
};


#endif

