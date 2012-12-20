/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _GRAPH_RENDER_H_
#define _GRAPH_RENDER_H_


#include <tsffi/function.h>


extern int Graph_SetDefaultBackground (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetDefaultDomain     (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetDefaultRange      (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Graph_SetBackground (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetDomain     (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetRange      (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_ClearPlot     (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Graph_HSetBackground (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_HSetDomain     (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_HSetRange      (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_HClearPlot     (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Graph_SetDefaultWeight (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetDefaultMode   (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetDefaultColor  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Graph_SetWeight (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetMode   (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_SetColor  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Graph_HSetWeight (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_HSetMode   (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);
extern int Graph_HSetColor  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Graph_Plot  (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);

extern int Graph_HPlot (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);


#endif

