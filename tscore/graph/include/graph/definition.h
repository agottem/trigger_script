/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _GRAPH_DEFINITION_H_
#define _GRAPH_DEFINITION_H_


#include <ffilib/idhash.h>

#include <tsffi/register.h>

#include <windows.h>


#define GRAPH_PLOT_MODE_POINTS 0
#define GRAPH_PLOT_MODE_LINES  1


struct graph_plot_info
{
    unsigned int mode;
    double       weight;
    COLORREF     color;
};

struct graph_grid_info
{
    COLORREF background_color;
};

struct graph_point
{
    struct graph_plot_info plot_info;

    double x;
    double y;

    struct graph_point* next_point;
};

struct graph_range
{
    double min_p;
    double max_p;
};

struct graph_definition
{
    HWND         window_handle;
    unsigned int unit_invocation_id;
    unsigned int graph_id;

    struct graph_module_data* module_data;

    char* x_axis_title;
    char* y_axis_title;

    struct graph_plot_info selected_plot_info;
    struct graph_grid_info selected_grid_info;

    struct graph_range plot_domain;
    struct graph_range view_domain;

    struct graph_range plot_range;
    struct graph_range view_range;

    struct graph_point* points;
};

struct graph_module_data
{
    unsigned int next_graph_id;
    unsigned int active_graph_count;

    struct graph_plot_info default_plot_info;
    struct graph_grid_info default_grid_info;
    struct graph_range     default_view_domain;
    struct graph_range     default_view_range;

    HANDLE graphs_closed_signal;

    struct ffilib_id_hash graph_hash;
    struct ffilib_id_hash unit_hash;
};


extern int  Graph_BeginModule (
                               struct tsffi_execif*,
                               void*,
                               struct tsffi_registration_group*,
                               void**
                              );
extern void Graph_EndModule   (
                               struct tsffi_execif*,
                               void*,
                               int,
                               struct tsffi_registration_group*,
                               void*
                              );

extern int  Graph_InitializeDefinition (
                                        HWND,
                                        unsigned int,
                                        struct graph_module_data*,
                                        struct graph_definition*
                                       );
extern void Graph_DestroyDefinition    (struct graph_definition*);

extern int  Graph_AddPoint    (double, double, struct graph_definition*);
extern void Graph_ClearPoints (struct graph_definition*);


#endif

