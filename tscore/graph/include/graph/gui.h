/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _GRAPH_WINDOW_H_
#define _GRAPH_WINDOW_H_


#include <tsffi/function.h>

#include <windows.h>


extern int  Graph_InitializeGUI (HINSTANCE);
extern void Graph_ShutdownGUI   (void);

extern int Graph_Graph (struct tsffi_invocation_data*, void*, union tsffi_value*, union tsffi_value*);


#endif

