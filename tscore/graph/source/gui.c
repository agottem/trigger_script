/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <graph/gui.h>
#include <graph/definition.h>

#include <tsffi/error.h>
#include <ffilib/thread.h>
#include <ffilib/module.h>
#include <ffilib/idhash.h>
#include <ffilib/error.h>

#include <windows.h>
#include <shellapi.h>
#include <float.h>
#include <stdio.h>


#define MAX_CONVERSION_STRING_LENGTH (3+DBL_MANT_DIG-DBL_MIN_EXP)

#define DEFAULT_GRAPH_WIDTH  600
#define DEFAULT_GRAPH_HEIGHT 500

#define GRAPH_AXIS_WEIGHT      2
#define GRAPH_AXIS_COLOR       RGB(0, 0, 0)
#define GRAPH_AXIS_TEXT_HEIGHT 12

#define GRAPH_TICK_WEIGHT 1
#define GRAPH_TICK_COLOR  RGB(200, 200, 200)

#define GRAPH_TICK_LABEL_RATIO 4


static LRESULT CALLBACK GraphWindowProc (HWND, UINT, WPARAM, LPARAM);

static LRESULT PaintGraph  (struct graph_definition*);
static void    PaintAxis   (
                            HDC,
                            struct graph_definition*,
                            RECT*,
                            double,
                            double,
                            unsigned int,
                            unsigned int,
                            struct graph_range*,
                            struct graph_range*
                           );
static void    PaintPoints (HDC, struct graph_definition*, RECT*, double, double, double, double);


static HINSTANCE gui_instance;
static HICON     process_icon;


static LRESULT CALLBACK GraphWindowProc (
                                         HWND   window_handle,
                                         UINT   message_id,
                                         WPARAM wparam,
                                         LPARAM lparam
                                        )
{
    struct graph_definition* definition;

    definition = (struct graph_definition*)GetWindowLongPtr(window_handle, GWLP_USERDATA);
    if(definition != NULL)
    {
        switch(message_id)
        {
        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT:
            return PaintGraph(definition);

        case WM_DESTROY:
            Graph_DestroyDefinition(definition);
            free(definition);

            break;
        }
    }

    return DefWindowProc(window_handle, message_id, wparam, lparam);
}

static LRESULT PaintGraph (struct graph_definition* definition)
{
    PAINTSTRUCT         paint_data;
    RECT                client_area;
    RECT                graph_area;
    struct graph_range* view_domain;
    struct graph_range* view_range;
    double              domain_coefficient;
    double              range_coefficient;
    double              x_offset;
    double              y_offset;
    double              plot_width;
    double              plot_height;
    unsigned int        graph_width;
    unsigned int        graph_height;
    int                 margin_size;
    HWND                window_handle;
    HBRUSH              brush;
    HDC                 dc;

    window_handle = definition->window_handle;

    GetClientRect(window_handle, &client_area);

    margin_size = GetSystemMetrics(SM_CYCAPTION);

    graph_area.left   = client_area.left+margin_size;
    graph_area.top    = client_area.top;
    graph_area.right  = client_area.right;
    graph_area.bottom = client_area.bottom-margin_size;

    if(graph_area.right < graph_area.left)
        graph_area.right = graph_area.left;
    if(graph_area.bottom < graph_area.top)
        graph_area.bottom = graph_area.top;

    if(definition->view_domain.max_p == 0.0 && definition->view_domain.min_p == 0.0)
        view_domain = &definition->plot_domain;
    else
        view_domain = &definition->view_domain;

    graph_width = graph_area.right-graph_area.left;
    plot_width  = view_domain->max_p-view_domain->min_p;
    if(plot_width == 0.0)
        domain_coefficient = (double)1.0;
    else
        domain_coefficient = (double)graph_width/plot_width;

    x_offset = -view_domain->min_p;

    if(definition->view_range.max_p == 0.0 && definition->view_range.min_p == 0.0)
        view_range = &definition->plot_range;
    else
        view_range = &definition->view_range;

    graph_height = graph_area.bottom-graph_area.top;
    plot_height  = view_range->max_p-view_range->min_p;
    if(plot_height == 0.0)
        range_coefficient = (double)1.0;
    else
        range_coefficient = (double)graph_height/plot_height;

    y_offset = -view_range->min_p;

    dc = BeginPaint(window_handle, &paint_data);

    brush = CreateSolidBrush(definition->selected_grid_info.background_color);
    if(brush == NULL)
        goto paint_failure;

    FillRect(dc, &client_area, brush);
    DeleteObject(brush);

    PaintAxis(
              dc, definition,
              &graph_area,
              plot_width,
              plot_height,
              graph_width,
              graph_height,
              view_domain,
              view_range
             );
    PaintPoints(
                dc,
                definition,
                &graph_area,
                domain_coefficient,
                range_coefficient,
                x_offset,
                y_offset
               );

paint_failure:
    EndPaint(window_handle, &paint_data);

    return 0;
}

static void PaintAxis  (
                        HDC                      dc,
                        struct graph_definition* definition,
                        RECT*                    graph_area,
                        double                   plot_width,
                        double                   plot_height,
                        unsigned int             graph_width,
                        unsigned int             graph_height,
                        struct graph_range*      view_domain,
                        struct graph_range*      view_range
                       )
{
    RECT         axis_area;
    double       real_y;
    double       real_y_tick_size;
    double       real_x;
    double       real_x_tick_size;
    unsigned int x_index;
    unsigned int y_index;
    unsigned int x_ticks;
    unsigned int y_ticks;
    unsigned int tick_x;
    unsigned int tick_y;
    int          tick_size;
    int          half_tick_size;
    HFONT        vertical_font;
    HFONT        horizontal_font;
    HGDIOBJ      original_pen;
    HGDIOBJ      original_font;
    HPEN         pen;

    vertical_font = CreateFont(
                               GRAPH_AXIS_TEXT_HEIGHT,
                               0,
                               900,
                               900,
                               FW_NORMAL,
                               FALSE,
                               FALSE,
                               FALSE,
                               DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY,
                               DEFAULT_PITCH|FF_DONTCARE,
                               NULL
                              );
    if(vertical_font == NULL)
        goto create_veritcal_font_failed;

    horizontal_font = CreateFont(
                                 GRAPH_AXIS_TEXT_HEIGHT,
                                 0,
                                 0,
                                 0,
                                 FW_NORMAL,
                                 FALSE,
                                 FALSE,
                                 FALSE,
                                 DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY,
                                 DEFAULT_PITCH|FF_DONTCARE,
                                 NULL
                                );
    if(horizontal_font == NULL)
        goto create_horizontal_font_failed;

    SetTextColor(dc, GRAPH_AXIS_COLOR);
    SetBkColor(dc, definition->selected_grid_info.background_color);

    tick_size      = GetSystemMetrics(SM_CYCAPTION);
    half_tick_size = tick_size/2;

    axis_area.left   = graph_area->left-GRAPH_AXIS_WEIGHT;
    axis_area.top    = graph_area->top;
    axis_area.right  = graph_area->right;
    axis_area.bottom = graph_area->bottom+GRAPH_AXIS_WEIGHT;

    tick_y  = axis_area.bottom-tick_size;
    tick_x  = axis_area.left+tick_size;
    x_ticks = graph_width/tick_size;
    y_ticks = graph_height/tick_size;

    real_y_tick_size = plot_height/y_ticks;
    real_y           = view_range->min_p+real_y_tick_size;

    real_x_tick_size = plot_width/x_ticks;
    real_x           = view_domain->min_p+real_x_tick_size;

    pen = CreatePen(PS_SOLID, GRAPH_TICK_WEIGHT, GRAPH_TICK_COLOR);
    if(pen == NULL)
        goto paint_failure;

    original_pen  = SelectObject(dc, pen);
    original_font = SelectObject(dc, horizontal_font);

    for(x_index = 0; x_index < x_ticks; x_index++)
    {
        if(x_index%GRAPH_TICK_LABEL_RATIO == 0)
        {
            char   text[MAX_CONVERSION_STRING_LENGTH+1];
            SIZE   text_size;
            size_t length;
            int    text_x;
            int    text_y;

            sprintf(text, "%.5g", real_x);

            length = strlen(text);

            GetTextExtentPoint32(dc, text, length, &text_size);

            text_x = tick_x-text_size.cx/2;
            text_y = axis_area.bottom+1;

            ExtTextOut(
                       dc,
                       text_x,
                       text_y,
                       0,
                       NULL,
                       text,
                       length,
                       NULL
                      );
        }

        MoveToEx(dc, tick_x, axis_area.top, NULL);
        LineTo(dc, tick_x, axis_area.bottom);

        real_x += real_x_tick_size;
        tick_x += tick_size;
    }

    SelectObject(dc, original_font);

    original_font = SelectObject(dc, vertical_font);

    for(y_index = 0; y_index < y_ticks; y_index++)
    {
        if(y_index%GRAPH_TICK_LABEL_RATIO == 0)
        {
            char   text[MAX_CONVERSION_STRING_LENGTH+1];
            SIZE   text_size;
            size_t length;
            int    text_x;
            int    text_y;

            sprintf(text, "%.5g", real_y);

            length = strlen(text);

            GetTextExtentPoint32(dc, text, length, &text_size);

            text_x = axis_area.left-text_size.cy-1;
            text_y = tick_y+text_size.cx/2;

            ExtTextOut(
                       dc,
                       text_x,
                       text_y,
                       0,
                       NULL,
                       text,
                       length,
                       NULL
                      );
        }

        MoveToEx(dc, axis_area.left, tick_y, NULL);
        LineTo(dc, axis_area.right, tick_y);

        real_y += real_y_tick_size;
        tick_y -= tick_size;
    }

    SelectObject(dc, original_font);
    SelectObject(dc, original_pen);
    DeleteObject(pen);

    pen = CreatePen(PS_SOLID, GRAPH_AXIS_WEIGHT, GRAPH_AXIS_COLOR);
    if(pen == NULL)
        goto paint_failure;

    original_pen = SelectObject(dc, pen);

    MoveToEx(dc, axis_area.left, axis_area.top, NULL);
    LineTo(dc, axis_area.left, axis_area.bottom);
    LineTo(dc, axis_area.right, axis_area.bottom);

    SelectObject(dc, original_pen);
    DeleteObject(pen);

paint_failure:
    DeleteObject(horizontal_font);
create_horizontal_font_failed:
    DeleteObject(vertical_font);

create_veritcal_font_failed:
    return;
}

void PaintPoints (
                  HDC                      dc,
                  struct graph_definition* definition,
                  RECT*                    graph_area,
                  double                   domain_coefficient,
                  double                   range_coefficient,
                  double                   x_offset,
                  double                   y_offset
                 )
{
    struct graph_point* point;
    HRGN                graph_region;

    graph_region = CreateRectRgn(
                                 graph_area->left,
                                 graph_area->top,
                                 graph_area->right,
                                 graph_area->bottom
                                );
    if(graph_region == NULL)
        return;

    SelectClipRgn(dc, graph_region);

    for(point = definition->points; point != NULL; point = point->next_point)
    {
        HPEN    pen;
        HBRUSH  brush;
        HGDIOBJ original_object;
        int     x;
        int     y;
        int     weight;

        x      = graph_area->left+(int)((point->x+x_offset)*domain_coefficient);
        y      = graph_area->bottom-(int)((point->y+y_offset)*range_coefficient);
        weight = (int)(point->plot_info.weight);

        MoveToEx(dc, x, y, NULL);

        switch(point->plot_info.mode)
        {
        case GRAPH_PLOT_MODE_POINTS:
            brush = CreateSolidBrush(point->plot_info.color);
            if(brush == NULL)
                goto paint_failure;

            original_object = SelectObject(dc, brush);

            Ellipse(
                    dc,
                    x-weight,
                    y-weight,
                    x+weight,
                    y+weight
                   );

            SelectObject(dc, original_object);
            DeleteObject(brush);

            break;

        case GRAPH_PLOT_MODE_LINES:
            if(point->next_point != NULL)
            {
                struct graph_point* to_point;

                pen = CreatePen(PS_SOLID, weight, point->plot_info.color);
                if(pen == NULL)
                    goto paint_failure;

                original_object = SelectObject(dc, pen);

                to_point = point->next_point;

                x = graph_area->left+(int)((to_point->x+x_offset)*domain_coefficient);
                y = graph_area->bottom-(int)((to_point->y+y_offset)*range_coefficient);

                LineTo(dc, x, y);

                SelectObject(dc, original_object);
                DeleteObject(pen);
            }

            break;
        }
    }

paint_failure:
    SelectClipRgn(dc, NULL);
    DeleteObject(graph_region);
}


int Graph_InitializeGUI (HINSTANCE instance)
{
    char       process_path[MAX_PATH];
    WNDCLASSEX window_class;
    ATOM       atom_error;
    DWORD      dword_error;

    dword_error = GetModuleFileName (NULL, process_path, MAX_PATH);
    if(dword_error != 0)
    {
        HANDLE module_handle;

        process_path[MAX_PATH-1] = 0;

        module_handle = GetModuleHandle(NULL);
        process_icon  = ExtractIcon(module_handle, process_path, 0);
    }
    else
        process_icon = NULL;

    if(process_icon == NULL)
        process_icon = LoadIcon(NULL, IDI_INFORMATION);

    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc   = &GraphWindowProc;
    window_class.cbClsExtra    = 0;
    window_class.cbWndExtra    = 0;
    window_class.hInstance     = instance;
    window_class.hIcon         = process_icon;
    window_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName  = NULL;
    window_class.lpszClassName = "TSGraphWindow";
    window_class.hIconSm       = NULL;

    atom_error = RegisterClassEx(&window_class);
    if(atom_error == 0)
        return FFILIB_ERROR_SYSTEM_CALL;

    gui_instance = instance;

    return FFILIB_ERROR_NONE;
}

void Graph_ShutdownGUI (void)
{
    UnregisterClass("TSGraphWindow", gui_instance);

    if(process_icon != NULL)
        DestroyIcon(process_icon);
}

int Graph_Graph (
                 struct tsffi_invocation_data* invocation_data,
                 void*                         group_data,
                 union tsffi_value*            output,
                 union tsffi_value*            input
                )
{
    struct graph_module_data* module_data;
    struct graph_definition*  definition;
    HWND                      window_handle;
    int                       error;
    int                       result;

    error = FFILib_SynchronousFFIFunction(
                                          &Graph_Graph,
                                          invocation_data,
                                          group_data,
                                          output,
                                          input,
                                          &ffilib_gui_thread,
                                          &result
                                         );
    if(error == FFILIB_ERROR_NONE)
        return result;
    else if(error != FFILIB_ERROR_THREAD_SWITCH)
        return TSFFI_ERROR_EXCEPTION;

    definition = malloc(sizeof(struct graph_definition));
    if(definition == NULL)
        goto allocate_definition_failed;

    window_handle = CreateWindowEx(
                                   WS_EX_COMPOSITED,
                                   "TSGraphWindow",
                                   (char*)input->string_data,
                                   WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_POPUP,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   DEFAULT_GRAPH_WIDTH,
                                   DEFAULT_GRAPH_HEIGHT,
                                   NULL,
                                   NULL,
                                   gui_instance,
                                   NULL
                                  );
    if(window_handle == NULL)
        goto create_window_failed;

    SetWindowLongPtr(window_handle, GWLP_USERDATA, (LPARAM)NULL);

    module_data = group_data;

    error = Graph_InitializeDefinition(
                                       window_handle,
                                       invocation_data->unit_invocation_id,
                                       module_data,
                                       definition
                                      );
    if(error != FFILIB_ERROR_NONE)
        goto initialize_definition_failed;

    SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)definition);

    ShowWindow(window_handle, SW_SHOWNORMAL);
    UpdateWindow(window_handle);

    output->int_data = definition->graph_id;

    return TSFFI_ERROR_NONE;

initialize_definition_failed:
    DestroyWindow(window_handle);
create_window_failed:
    free(definition);

allocate_definition_failed:
    return TSFFI_ERROR_EXCEPTION;
}

