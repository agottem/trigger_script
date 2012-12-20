/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <graph/definition.h>

#include <ffilib/module.h>
#include <ffilib/error.h>
#include <tsffi/error.h>


#define DEFAULT_PLOT_MODE             GRAPH_PLOT_MODE_LINES
#define DEFAULT_PLOT_WEIGHT           (double)1.0
#define DEFAULT_PLOT_COLOR            RGB(0, 0, 255)
#define DEFAULT_GRID_BACKGROUND_COLOR RGB(255, 255, 255)


static void ClearPoints (struct graph_definition*);


static void ClearPoints (struct graph_definition* definition)
{
    struct graph_point* point;

    point = definition->points;
    while(point != NULL)
    {
        struct graph_point* free_point;

        free_point = point;
        point      = point->next_point;

        free(free_point);
    }
}


int Graph_BeginModule (
                       struct tsffi_execif*             execif,
                       void*                            execif_data,
                       struct tsffi_registration_group* group,
                       void**                           group_data
                      )
{
    struct graph_module_data* module_data;
    int                       error;

    error = FFILib_BeginModule(execif, execif_data, group, NULL);
    if(error != TSFFI_ERROR_NONE)
        goto begin_ffilib_failed;

    module_data = malloc(sizeof(struct graph_module_data));
    if(module_data == NULL)
        goto allocate_module_data_failed;

    module_data->graphs_closed_signal = CreateEvent(NULL, TRUE, TRUE, NULL);
    if(module_data->graphs_closed_signal == NULL)
        goto create_closed_signal_failed;

    module_data->next_graph_id      = 1;
    module_data->active_graph_count = 0;

    module_data->default_plot_info.mode   = DEFAULT_PLOT_MODE;
    module_data->default_plot_info.weight = DEFAULT_PLOT_WEIGHT;
    module_data->default_plot_info.color  = DEFAULT_PLOT_COLOR;

    module_data->default_grid_info.background_color = DEFAULT_GRID_BACKGROUND_COLOR;

    module_data->default_view_domain.max_p = (double)0.0;
    module_data->default_view_domain.min_p = (double)0.0;
    module_data->default_view_range.max_p  = (double)0.0;
    module_data->default_view_range.min_p  = (double)0.0;

    FFILib_InitializeHash(&module_data->graph_hash);
    FFILib_InitializeHash(&module_data->unit_hash);

    *group_data = module_data;

    return TSFFI_ERROR_NONE;

create_closed_signal_failed:
    free(module_data);
allocate_module_data_failed:
    FFILib_EndModule(execif, execif_data, TSFFI_ERROR_EXCEPTION, group, NULL);

begin_ffilib_failed:
    return TSFFI_ERROR_EXCEPTION;
}

void Graph_EndModule (
                      struct tsffi_execif*             execif,
                      void*                            execif_data,
                      int                              error,
                      struct tsffi_registration_group* group,
                      void*                            group_data
                     )
{
    struct graph_module_data* module_data;

    module_data = group_data;

    WaitForSingleObject(module_data->graphs_closed_signal, INFINITE);

    FFILib_DestroyHash(&module_data->unit_hash);
    FFILib_DestroyHash(&module_data->graph_hash);
    CloseHandle(module_data->graphs_closed_signal);

    free(module_data);

    FFILib_EndModule(execif, execif_data, TSFFI_ERROR_EXCEPTION, group, NULL);
}

int Graph_InitializeDefinition (
                                HWND                      window_handle,
                                unsigned int              unit_invocation_id,
                                struct graph_module_data* module_data,
                                struct graph_definition*  definition
                               )
{
    int          error;
    unsigned int graph_id;

    if(module_data->active_graph_count == 0)
        ResetEvent(module_data->graphs_closed_signal);

    graph_id = module_data->next_graph_id;

    definition->window_handle      = window_handle;
    definition->unit_invocation_id = unit_invocation_id;
    definition->graph_id           = graph_id;
    definition->module_data        = module_data;
    definition->x_axis_title       = NULL;
    definition->y_axis_title       = NULL;
    definition->selected_plot_info = module_data->default_plot_info;
    definition->selected_grid_info = module_data->default_grid_info;
    definition->plot_domain.max_p  = (double)0.0;
    definition->plot_domain.min_p  = (double)0.0;
    definition->view_domain        = module_data->default_view_domain;
    definition->plot_range.max_p   = (double)0.0;
    definition->plot_range.min_p   = (double)0.0;
    definition->view_range         = module_data->default_view_range;
    definition->points             = NULL;

    error = FFILib_AddID(unit_invocation_id, definition, &module_data->unit_hash);
    if(error != FFILIB_ERROR_NONE)
        goto add_unit_invocation_id_failed;

    error = FFILib_AddID(graph_id, definition, &module_data->graph_hash);
    if(error != FFILIB_ERROR_NONE)
        goto add_graph_id_failed;

    module_data->active_graph_count++;
    module_data->next_graph_id++;

    return FFILIB_ERROR_NONE;

add_graph_id_failed:
    FFILib_RemoveID(unit_invocation_id, &module_data->unit_hash);

add_unit_invocation_id_failed:
    return error;
}

void Graph_DestroyDefinition (struct graph_definition* definition)
{
    struct graph_module_data* module_data;

    module_data = definition->module_data;

    FFILib_RemoveID(definition->unit_invocation_id, &module_data->unit_hash);
    FFILib_RemoveID(definition->graph_id, &module_data->graph_hash);

    ClearPoints(definition);

    if(definition->x_axis_title != NULL)
        free(definition->x_axis_title);
    if(definition->y_axis_title != NULL)
        free(definition->y_axis_title);

    module_data->active_graph_count--;
    if(module_data->active_graph_count == 0)
        SetEvent(module_data->graphs_closed_signal);
}

int Graph_AddPoint (double x, double y, struct graph_definition* definition)
{
    struct graph_point* point;

    point = malloc(sizeof(struct graph_point));
    if(point == NULL)
        return FFILIB_ERROR_MEMORY;

    point->plot_info = definition->selected_plot_info;
    point->x         = x;
    point->y         = y;

    point->next_point  = definition->points;
    definition->points = point;

    if(point->next_point == NULL)
    {
        definition->plot_domain.max_p = x;
        definition->plot_domain.min_p = x;

        definition->plot_range.max_p = y;
        definition->plot_range.min_p = y;
    }
    else
    {
        if(x > definition->plot_domain.max_p)
            definition->plot_domain.max_p = x;
        else if(x < definition->plot_domain.min_p)
            definition->plot_domain.min_p = x;

        if(y > definition->plot_range.max_p)
            definition->plot_range.max_p = y;
        else if(y < definition->plot_range.min_p)
            definition->plot_range.min_p = y;
    }

    return FFILIB_ERROR_NONE;
}

void Graph_ClearPoints (struct graph_definition* definition)
{
    ClearPoints(definition);

    definition->plot_domain.max_p  = (double)0.0;
    definition->plot_domain.min_p  = (double)0.0;
    definition->plot_range.max_p   = (double)0.0;
    definition->plot_range.min_p   = (double)0.0;

    definition->points = NULL;
}

