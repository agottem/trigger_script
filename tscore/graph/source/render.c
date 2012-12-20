/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <graph/definition.h>

#include <tsffi/error.h>
#include <ffilib/thread.h>
#include <ffilib/module.h>
#include <ffilib/error.h>

#include <string.h>
#include <stdio.h>
#include <windows.h>


#define MODE_TEXT_POINTS "points"
#define MODE_TEXT_LINES  "lines"

#define MAX_ALERT_LENGTH 512


struct set_weight_data
{
    double weight;

    struct graph_module_data* module_data;
    unsigned int              id;
};

struct set_mode_data
{
    unsigned int mode;

    struct graph_module_data* module_data;
    unsigned int              id;
};

struct set_color_data
{
    COLORREF color;

    struct graph_module_data* module_data;
    unsigned int              id;
};

struct set_range_data
{
    struct graph_range range;

    struct graph_module_data* module_data;
    unsigned int              id;
};

struct plot_data
{
    double x;
    double y;

    struct graph_module_data* module_data;
    unsigned int              id;
};

struct clear_plot_data
{
    struct graph_module_data* module_data;
    unsigned int              id;
};


static void AlertInvalidMode (struct tsffi_invocation_data*);

static int DetermineMode (char*, unsigned int*);

static int SetDefaultBackground (void*);
static int SetDefaultDomain     (void*);
static int SetDefaultRange      (void*);

static int SetBackground (void*);
static int SetDomain     (void*);
static int SetRange      (void*);
static int ClearPlot     (void*);

static int HSetBackground (void*);
static int HSetDomain     (void*);
static int HSetRange      (void*);
static int HClearPlot     (void*);

static int SetDefaultWeight (void*);
static int SetDefaultMode   (void*);
static int SetDefaultColor  (void*);

static int SetWeight (void*);
static int SetMode   (void*);
static int SetColor  (void*);

static int HSetWeight (void*);
static int HSetMode   (void*);
static int HSetColor  (void*);

static int Plot  (void*);

static int HPlot (void*);


static void AlertInvalidMode (struct tsffi_invocation_data* invocation_data)
{
    char text[MAX_ALERT_LENGTH];

    _snprintf(
              text,
              MAX_ALERT_LENGTH,
              "function=%s line=%d: Invalid plotting mode selected, valid options are \"" MODE_TEXT_LINES "\", \"" MODE_TEXT_POINTS "\"",
              invocation_data->unit_name,
              invocation_data->unit_location
             );

    text[MAX_ALERT_LENGTH-1] = 0;

    invocation_data->execif->set_exception_text(invocation_data->execif_data, text);
}

static void AlertInvalidHandle (struct tsffi_invocation_data* invocation_data)
{
    char text[MAX_ALERT_LENGTH];

    _snprintf(
              text,
              MAX_ALERT_LENGTH,
              "function=%s line=%d: Invalid handle specified for graphing function, use 'graph()' to create a handle",
              invocation_data->unit_name,
              invocation_data->unit_location
             );

    text[MAX_ALERT_LENGTH-1] = 0;

    invocation_data->execif->set_exception_text(invocation_data->execif_data, text);
}

static int DetermineMode (char* mode_text, unsigned int* mode)
{
    if(_stricmp(MODE_TEXT_POINTS, mode_text) == 0)
        *mode = GRAPH_PLOT_MODE_POINTS;
    else if(_stricmp(MODE_TEXT_LINES, mode_text) == 0)
        *mode = GRAPH_PLOT_MODE_LINES;
    else
        return -1;

    return 0;
}

static int SetDefaultBackground (void* user_data)
{
    struct graph_module_data* module_data;
    struct set_color_data*    color_data;

    color_data  = user_data;
    module_data = color_data->module_data;

    module_data->default_grid_info.background_color = color_data->color;

    free(color_data);

    return 0;
}

static int SetDefaultDomain (void* user_data)
{
    struct graph_module_data* module_data;
    struct set_range_data*    range_data;

    range_data  = user_data;
    module_data = range_data->module_data;

    module_data->default_view_domain = range_data->range;

    free(range_data);

    return 0;
}

static int SetDefaultRange (void* user_data)
{
    struct graph_module_data* module_data;
    struct set_range_data*    range_data;

    range_data  = user_data;
    module_data = range_data->module_data;

    module_data->default_view_range = range_data->range;

    free(range_data);

    return 0;
}

static int SetBackground (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_color_data*    color_data;

    color_data  = user_data;
    module_data = color_data->module_data;

    definition = FFILib_GetIDData(
                                  color_data->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
    {
        definition->selected_grid_info.background_color = color_data->color;

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(color_data);

    return 0;
}

static int SetDomain (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_range_data*    range_data;

    range_data  = user_data;
    module_data = range_data->module_data;

    definition = FFILib_GetIDData(
                                  range_data->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
    {
        definition->view_domain = range_data->range;

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(range_data);

    return 0;
}

static int SetRange (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_range_data*    range_data;

    range_data  = user_data;
    module_data = range_data->module_data;

    definition = FFILib_GetIDData(
                                  range_data->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
    {
        definition->view_range = range_data->range;

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(range_data);

    return 0;
}

static int ClearPlot (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct clear_plot_data*   clear_data;

    clear_data  = user_data;
    module_data = clear_data->module_data;

    definition = FFILib_GetIDData(
                                  clear_data->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
    {
        Graph_ClearPoints(definition);

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(clear_data);

    return 0;
}

static int HSetBackground (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_color_data*    color_data;

    color_data  = user_data;
    module_data = color_data->module_data;

    definition = FFILib_GetIDData(
                                  color_data->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
    {
        definition->selected_grid_info.background_color = color_data->color;

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(color_data);

    return 0;
}

static int HSetDomain (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_range_data*    range_data;

    range_data  = user_data;
    module_data = range_data->module_data;

    definition = FFILib_GetIDData(
                                  range_data->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
    {
        definition->view_domain = range_data->range;

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(range_data);

    return 0;
}

static int HSetRange (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_range_data*    range_data;

    range_data  = user_data;
    module_data = range_data->module_data;

    definition = FFILib_GetIDData(
                                  range_data->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
    {
        definition->view_range = range_data->range;

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(range_data);

    return 0;
}

static int HClearPlot (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct clear_plot_data*   clear_data;

    clear_data  = user_data;
    module_data = clear_data->module_data;

    definition = FFILib_GetIDData(
                                  clear_data->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
    {
        Graph_ClearPoints(definition);

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(clear_data);

    return 0;
}

static int SetDefaultWeight (void* user_data)
{
    struct graph_module_data* module_data;
    struct set_weight_data*   weight_data;

    weight_data = user_data;
    module_data = weight_data->module_data;

    module_data->default_plot_info.weight = weight_data->weight;

    free(weight_data);

    return 0;
}

static int SetDefaultMode (void* user_data)
{
    struct graph_module_data* module_data;
    struct set_mode_data*     mode_data;

    mode_data   = user_data;
    module_data = mode_data->module_data;

    module_data->default_plot_info.mode = mode_data->mode;

    free(mode_data);

    return 0;
}

static int SetDefaultColor (void* user_data)
{
    struct graph_module_data* module_data;
    struct set_color_data*    color_data;

    color_data  = user_data;
    module_data = color_data->module_data;

    module_data->default_plot_info.color = color_data->color;

    free(color_data);

    return 0;
}

static int SetWeight (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_weight_data*   weight_data;

    weight_data = user_data;
    module_data = weight_data->module_data;

    definition = FFILib_GetIDData(
                                  weight_data->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
        definition->selected_plot_info.weight = weight_data->weight;

    free(weight_data);

    return 0;
}

static int SetMode (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_mode_data*     mode_data;

    mode_data   = user_data;
    module_data = mode_data->module_data;

    definition = FFILib_GetIDData(
                                  mode_data->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
        definition->selected_plot_info.mode = mode_data->mode;

    free(mode_data);

    return 0;
}

static int SetColor (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_color_data*    color_data;

    color_data  = user_data;
    module_data = color_data->module_data;

    definition = FFILib_GetIDData(
                                  color_data->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
        definition->selected_plot_info.color = color_data->color;

    free(color_data);

    return 0;
}

static int HSetWeight (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_weight_data*   weight_data;

    weight_data = user_data;
    module_data = weight_data->module_data;

    definition = FFILib_GetIDData(
                                  weight_data->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
        definition->selected_plot_info.weight = weight_data->weight;

    free(weight_data);

    return 0;
}

static int HSetMode (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_mode_data*     mode_data;

    mode_data   = user_data;
    module_data = mode_data->module_data;

    definition = FFILib_GetIDData(
                                  mode_data->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
        definition->selected_plot_info.mode = mode_data->mode;

    free(mode_data);

    return 0;
}

static int HSetColor (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct set_color_data*    color_data;

    color_data  = user_data;
    module_data = color_data->module_data;

    definition = FFILib_GetIDData(
                                  color_data->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
        definition->selected_plot_info.color = color_data->color;

    free(color_data);

    return 0;
}

static int Plot (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct plot_data*         plot;

    plot        = user_data;
    module_data = plot->module_data;

    definition = FFILib_GetIDData(
                                  plot->id,
                                  &module_data->unit_hash
                                 );
    if(definition != NULL)
    {
        Graph_AddPoint(plot->x, plot->y, definition);

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(plot);

    return 0;
}

static int HPlot (void* user_data)
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    struct plot_data*         plot;

    plot        = user_data;
    module_data = plot->module_data;

    definition = FFILib_GetIDData(
                                  plot->id,
                                  &module_data->graph_hash
                                 );
    if(definition != NULL)
    {
        Graph_AddPoint(plot->x, plot->y, definition);

        RedrawWindow(definition->window_handle, NULL, NULL, RDW_INVALIDATE);
    }

    free(plot);

    return 0;
}


int Graph_SetDefaultBackground (
                                struct tsffi_invocation_data* invocation_data,
                                void*                         group_data,
                                union tsffi_value*            output,
                                union tsffi_value*            input
                               )
{
    struct set_color_data* color_data;
    char*                  stop_position;

    color_data = malloc(sizeof(struct set_color_data));
    if(color_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    color_data->color       = (COLORREF)strtol((char*)input->string_data, &stop_position, 16);
    color_data->module_data = group_data;

    FFILib_AsynchronousFunction(&SetDefaultBackground, color_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetDefaultDomain (
                            struct tsffi_invocation_data* invocation_data,
                            void*                         group_data,
                            union tsffi_value*            output,
                            union tsffi_value*            input
                           )
{
    struct set_range_data* range_data;

    range_data = malloc(sizeof(struct set_range_data));
    if(range_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    if(input[0].real_data > input[1].real_data)
    {
        range_data->range.min_p = (double)input[1].real_data;
        range_data->range.max_p = (double)input[0].real_data;
    }
    else
    {
        range_data->range.min_p = (double)input[0].real_data;
        range_data->range.max_p = (double)input[1].real_data;
    }

    range_data->module_data = group_data;

    FFILib_AsynchronousFunction(&SetDefaultDomain, range_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetDefaultRange (
                           struct tsffi_invocation_data* invocation_data,
                           void*                         group_data,
                           union tsffi_value*            output,
                           union tsffi_value*            input
                          )
{
    struct set_range_data* range_data;

    range_data = malloc(sizeof(struct set_range_data));
    if(range_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    if(input[0].real_data > input[1].real_data)
    {
        range_data->range.min_p = (double)input[1].real_data;
        range_data->range.max_p = (double)input[0].real_data;
    }
    else
    {
        range_data->range.min_p = (double)input[0].real_data;
        range_data->range.max_p = (double)input[1].real_data;
    }

    range_data->module_data = group_data;

    FFILib_AsynchronousFunction(&SetDefaultRange, range_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetBackground (
                         struct tsffi_invocation_data* invocation_data,
                         void*                         group_data,
                         union tsffi_value*            output,
                         union tsffi_value*            input
                        )
{
    struct set_color_data* color_data;
    char*                  stop_position;

    color_data = malloc(sizeof(struct set_color_data));
    if(color_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    color_data->color       = (COLORREF)strtol((char*)input->string_data, &stop_position, 16);
    color_data->module_data = group_data;
    color_data->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&SetBackground, color_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetDomain (
                     struct tsffi_invocation_data* invocation_data,
                     void*                         group_data,
                     union tsffi_value*            output,
                     union tsffi_value*            input
                    )
{
    struct set_range_data* range_data;

    range_data = malloc(sizeof(struct set_range_data));
    if(range_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    if(input[0].real_data > input[1].real_data)
    {
        range_data->range.min_p = (double)input[1].real_data;
        range_data->range.max_p = (double)input[0].real_data;
    }
    else
    {
        range_data->range.min_p = (double)input[0].real_data;
        range_data->range.max_p = (double)input[1].real_data;
    }

    range_data->module_data = group_data;
    range_data->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&SetDomain, range_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetRange (
                    struct tsffi_invocation_data* invocation_data,
                    void*                         group_data,
                    union tsffi_value*            output,
                    union tsffi_value*            input
                   )
{
    struct set_range_data* range_data;

    range_data = malloc(sizeof(struct set_range_data));
    if(range_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    if(input[0].real_data > input[1].real_data)
    {
        range_data->range.min_p = (double)input[1].real_data;
        range_data->range.max_p = (double)input[0].real_data;
    }
    else
    {
        range_data->range.min_p = (double)input[0].real_data;
        range_data->range.max_p = (double)input[1].real_data;
    }

    range_data->module_data = group_data;
    range_data->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&SetRange, range_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_ClearPlot (
                     struct tsffi_invocation_data* invocation_data,
                     void*                         group_data,
                     union tsffi_value*            output,
                     union tsffi_value*            input
                    )
{
    struct clear_plot_data* clear_data;

    clear_data = malloc(sizeof(struct clear_plot_data));
    if(clear_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    clear_data->module_data = group_data;
    clear_data->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&ClearPlot, clear_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HSetBackground (
                          struct tsffi_invocation_data* invocation_data,
                          void*                         group_data,
                          union tsffi_value*            output,
                          union tsffi_value*            input
                         )
{
    struct set_color_data* color_data;
    char*                  stop_position;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    color_data = malloc(sizeof(struct set_color_data));
    if(color_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    color_data->color       = (COLORREF)strtol((char*)input[1].string_data, &stop_position, 16);
    color_data->module_data = group_data;
    color_data->id          = (unsigned int)input[0].int_data;

    FFILib_AsynchronousFunction(&HSetBackground, color_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HSetDomain (
                      struct tsffi_invocation_data* invocation_data,
                      void*                         group_data,
                      union tsffi_value*            output,
                      union tsffi_value*            input
                     )
{
    struct set_range_data* range_data;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    range_data = malloc(sizeof(struct set_range_data));
    if(range_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    if(input[1].real_data > input[2].real_data)
    {
        range_data->range.min_p = (double)input[2].real_data;
        range_data->range.max_p = (double)input[1].real_data;
    }
    else
    {
        range_data->range.min_p = (double)input[1].real_data;
        range_data->range.max_p = (double)input[2].real_data;
    }

    range_data->module_data = group_data;
    range_data->id          = (unsigned int)input[0].int_data;

    FFILib_AsynchronousFunction(&HSetDomain, range_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HSetRange (
                     struct tsffi_invocation_data* invocation_data,
                     void*                         group_data,
                     union tsffi_value*            output,
                     union tsffi_value*            input
                    )
{
    struct set_range_data* range_data;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    range_data = malloc(sizeof(struct set_range_data));
    if(range_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    if(input[1].real_data > input[2].real_data)
    {
        range_data->range.min_p = (double)input[2].real_data;
        range_data->range.max_p = (double)input[1].real_data;
    }
    else
    {
        range_data->range.min_p = (double)input[1].real_data;
        range_data->range.max_p = (double)input[2].real_data;
    }

    range_data->module_data = group_data;
    range_data->id          = (unsigned int)input[0].int_data;

    FFILib_AsynchronousFunction(&HSetRange, range_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HClearPlot (
                      struct tsffi_invocation_data* invocation_data,
                      void*                         group_data,
                      union tsffi_value*            output,
                      union tsffi_value*            input
                     )
{
    struct clear_plot_data* clear_data;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    clear_data = malloc(sizeof(struct clear_plot_data));
    if(clear_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    clear_data->module_data = group_data;
    clear_data->id          = (unsigned int)input->int_data;

    FFILib_AsynchronousFunction(&HClearPlot, clear_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetDefaultWeight (
                            struct tsffi_invocation_data* invocation_data,
                            void*                         group_data,
                            union tsffi_value*            output,
                            union tsffi_value*            input
                           )
{
    struct set_weight_data* weight_data;

    weight_data = malloc(sizeof(struct set_weight_data));
    if(weight_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    weight_data->weight      = (double)input->real_data;
    weight_data->module_data = group_data;

    FFILib_AsynchronousFunction(&SetDefaultWeight, weight_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetDefaultMode (
                          struct tsffi_invocation_data* invocation_data,
                          void*                         group_data,
                          union tsffi_value*            output,
                          union tsffi_value*            input
                         )
{
    struct set_mode_data* mode_data;
    char*                 mode_text;
    unsigned int          mode;
    int                   error;

    mode_text = (char*)input->string_data;

    error = DetermineMode(mode_text, &mode);
    if(error != 0)
    {
        AlertInvalidMode(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    mode_data = malloc(sizeof(struct set_mode_data));
    if(mode_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    mode_data->mode        = mode;
    mode_data->module_data = group_data;

    FFILib_AsynchronousFunction(&SetDefaultMode, mode_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetDefaultColor (
                           struct tsffi_invocation_data* invocation_data,
                           void*                         group_data,
                           union tsffi_value*            output,
                           union tsffi_value*            input
                          )
{
    struct set_color_data* color_data;
    char*                  stop_position;

    color_data = malloc(sizeof(struct set_color_data));
    if(color_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    color_data->color       = (COLORREF)strtol((char*)input->string_data, &stop_position, 16);
    color_data->module_data = group_data;

    FFILib_AsynchronousFunction(&SetDefaultColor, color_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetWeight (
                     struct tsffi_invocation_data* invocation_data,
                     void*                         group_data,
                     union tsffi_value*            output,
                     union tsffi_value*            input
                    )
{
    struct set_weight_data* weight_data;

    weight_data = malloc(sizeof(struct set_weight_data));
    if(weight_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    weight_data->weight      = (double)input->real_data;
    weight_data->module_data = group_data;
    weight_data->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&SetWeight, weight_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetMode (
                   struct tsffi_invocation_data* invocation_data,
                   void*                         group_data,
                   union tsffi_value*            output,
                   union tsffi_value*            input
                  )
{
    struct set_mode_data* mode_data;
    char*                 mode_text;
    unsigned int          mode;
    int                   error;

    mode_text = (char*)input->string_data;

    error = DetermineMode(mode_text, &mode);
    if(error != 0)
    {
        AlertInvalidMode(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    mode_data = malloc(sizeof(struct set_mode_data));
    if(mode_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    mode_data->mode        = mode;
    mode_data->module_data = group_data;
    mode_data->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&SetMode, mode_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_SetColor (
                    struct tsffi_invocation_data* invocation_data,
                    void*                         group_data,
                    union tsffi_value*            output,
                    union tsffi_value*            input
                   )
{
    struct set_color_data* color_data;
    char*                  stop_position;

    color_data = malloc(sizeof(struct set_color_data));
    if(color_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    color_data->color       = (COLORREF)strtol((char*)input->string_data, &stop_position, 16);
    color_data->module_data = group_data;
    color_data->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&SetColor, color_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HSetWeight (
                      struct tsffi_invocation_data* invocation_data,
                      void*                         group_data,
                      union tsffi_value*            output,
                      union tsffi_value*            input
                     )
{
    struct set_weight_data* weight_data;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    weight_data = malloc(sizeof(struct set_weight_data));
    if(weight_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    weight_data->weight      = (double)input[1].real_data;
    weight_data->module_data = group_data;
    weight_data->id          = (unsigned int)input[0].int_data;

    FFILib_AsynchronousFunction(&HSetWeight, weight_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HSetMode (
                    struct tsffi_invocation_data* invocation_data,
                    void*                         group_data,
                    union tsffi_value*            output,
                    union tsffi_value*            input
                   )
{
    struct set_mode_data* mode_data;
    char*                 mode_text;
    unsigned int          mode;
    int                   error;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    mode_text = (char*)input[1].string_data;

    error = DetermineMode(mode_text, &mode);
    if(error != 0)
    {
        AlertInvalidMode(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    mode_data = malloc(sizeof(struct set_mode_data));
    if(mode_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    mode_data->mode        = mode;
    mode_data->module_data = group_data;
    mode_data->id          = (unsigned int)input[0].int_data;

    FFILib_AsynchronousFunction(&HSetMode, mode_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HSetColor (
                     struct tsffi_invocation_data* invocation_data,
                     void*                         group_data,
                     union tsffi_value*            output,
                     union tsffi_value*            input
                    )
{
    struct set_color_data* color_data;
    char*                  stop_position;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    color_data = malloc(sizeof(struct set_color_data));
    if(color_data == NULL)
        return TSFFI_ERROR_EXCEPTION;

    color_data->color       = (COLORREF)strtol((char*)input[1].string_data, &stop_position, 16);
    color_data->module_data = group_data;
    color_data->id          = (unsigned int)input[0].int_data;

    FFILib_AsynchronousFunction(&HSetColor, color_data, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_Plot (
                struct tsffi_invocation_data* invocation_data,
                void*                         group_data,
                union tsffi_value*            output,
                union tsffi_value*            input
               )
{
    struct plot_data* plot;

    plot = malloc(sizeof(struct plot_data));
    if(plot == NULL)
        return TSFFI_ERROR_EXCEPTION;

    plot->x           = (double)input[0].real_data;
    plot->y           = (double)input[1].real_data;
    plot->module_data = group_data;
    plot->id          = invocation_data->unit_invocation_id;

    FFILib_AsynchronousFunction(&Plot, plot, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

int Graph_HPlot (
                 struct tsffi_invocation_data* invocation_data,
                 void*                         group_data,
                 union tsffi_value*            output,
                 union tsffi_value*            input
                )
{
    struct plot_data* plot;

    if(input[0].int_data == 0)
    {
        AlertInvalidHandle(invocation_data);

        return TSFFI_ERROR_EXCEPTION;
    }

    plot = malloc(sizeof(struct plot_data));
    if(plot == NULL)
        return TSFFI_ERROR_EXCEPTION;

    plot->x           = (double)input[1].real_data;
    plot->y           = (double)input[2].real_data;
    plot->module_data = group_data;
    plot->id          = (unsigned int)input[0].int_data;

    FFILib_AsynchronousFunction(&HPlot, plot, &ffilib_gui_thread);

    return TSFFI_ERROR_NONE;
}

