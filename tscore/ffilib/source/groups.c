/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "groups.h"
#include "doc.h"

#include <ffilib/module.h>
#include <notify/init.h>
#include <notify/pipe.h>
#include <notify/message.h>
#include <math/random.h>
#include <math/basic.h>
#include <time/timer.h>
#include <time/time.h>
#include <graph/gui.h>
#include <graph/definition.h>
#include <graph/render.h>

#include <stdlib.h>


struct tsffi_function_definition notify_functions[] = {
                                                          {"pipe",         ffilib_doc_pipe,         &Notify_Pipe,      NULL,                       TSFFI_PRIMITIVE_TYPE_INT,    1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                          {"read_pipe",    ffilib_doc_read_pipe,    &Notify_ReadPipe,  NULL,                       TSFFI_PRIMITIVE_TYPE_STRING, 1, {TSFFI_PRIMITIVE_TYPE_INT}},
                                                          {"hread_pipe",   ffilib_doc_hread_pipe,   &Notify_HReadPipe, NULL,                       TSFFI_PRIMITIVE_TYPE_STRING, 2, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_INT}},
                                                          {"listen_pipe",  ffilib_doc_listen_pipe,  NULL,              &Notify_Action_ListenPipe,  TSFFI_PRIMITIVE_TYPE_VOID,   0},
                                                          {"hlisten_pipe", ffilib_doc_hlisten_pipe, NULL,              &Notify_Action_HListenPipe, TSFFI_PRIMITIVE_TYPE_VOID,   1, {TSFFI_PRIMITIVE_TYPE_INT}},
                                                          {"print",        ffilib_doc_print,        &Notify_Print,     NULL,                       TSFFI_PRIMITIVE_TYPE_VOID,   1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                          {"message",      ffilib_doc_message,      &Notify_Message,   NULL,                       TSFFI_PRIMITIVE_TYPE_VOID,   1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                          {"choice",       ffilib_doc_choice,       &Notify_Choice,    NULL,                       TSFFI_PRIMITIVE_TYPE_BOOL,   1, {TSFFI_PRIMITIVE_TYPE_STRING}}
                                                      };

struct tsffi_function_definition math_functions[] = {
                                                        {"uniform_random",  ffilib_doc_uniform_random,  &Math_UniformRandom,  NULL, TSFFI_PRIMITIVE_TYPE_REAL, 0},
                                                        {"gaussian_random", ffilib_doc_gaussian_random, &Math_GaussianRandom, NULL, TSFFI_PRIMITIVE_TYPE_REAL, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"min",             ffilib_doc_min,             &Math_MinBool,        NULL, TSFFI_PRIMITIVE_TYPE_BOOL, 2, {TSFFI_PRIMITIVE_TYPE_BOOL, TSFFI_PRIMITIVE_TYPE_BOOL}},
                                                        {"min",             ffilib_doc_min,             &Math_MinInt,         NULL, TSFFI_PRIMITIVE_TYPE_INT,  2, {TSFFI_PRIMITIVE_TYPE_INT,  TSFFI_PRIMITIVE_TYPE_INT}},
                                                        {"min",             ffilib_doc_min,             &Math_MinReal,        NULL, TSFFI_PRIMITIVE_TYPE_REAL, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"max",             ffilib_doc_max,             &Math_MaxBool,        NULL, TSFFI_PRIMITIVE_TYPE_BOOL, 2, {TSFFI_PRIMITIVE_TYPE_BOOL, TSFFI_PRIMITIVE_TYPE_BOOL}},
                                                        {"max",             ffilib_doc_max,             &Math_MaxInt,         NULL, TSFFI_PRIMITIVE_TYPE_INT,  2, {TSFFI_PRIMITIVE_TYPE_INT,  TSFFI_PRIMITIVE_TYPE_INT}},
                                                        {"max",             ffilib_doc_max,             &Math_MaxReal,        NULL, TSFFI_PRIMITIVE_TYPE_REAL, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"ceil",            ffilib_doc_ceil,            &Math_Ceil,           NULL, TSFFI_PRIMITIVE_TYPE_INT,  1, {TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"floor",           ffilib_doc_floor,           &Math_Floor,          NULL, TSFFI_PRIMITIVE_TYPE_INT,  1, {TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"round",           ffilib_doc_round,           &Math_Round,          NULL, TSFFI_PRIMITIVE_TYPE_INT,  1, {TSFFI_PRIMITIVE_TYPE_REAL}}
                                                    };

struct tsffi_function_definition time_functions[] = {
                                                        {"timer",  ffilib_doc_timer,  NULL,        &Time_Action_Timer,  TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"ptimer", ffilib_doc_ptimer, NULL,        &Time_Action_PTimer, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"vtimer", ffilib_doc_vtimer, NULL,        &Time_Action_VTimer, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_REAL}},
                                                        {"time",   ffilib_doc_time,   &Time_Time,  NULL,                TSFFI_PRIMITIVE_TYPE_REAL, 0},
                                                        {"delay",  ffilib_doc_delay,  &Time_Delay, NULL,                TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_REAL}}
                                                    };

struct tsffi_function_definition graph_functions[] = {
                                                         {"graph",               ffilib_doc_graph,               &Graph_Graph,                NULL, TSFFI_PRIMITIVE_TYPE_INT,  1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"set_def_graph_color", ffilib_doc_set_def_graph_color, &Graph_SetDefaultBackground, NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"set_def_domain",      ffilib_doc_set_def_domain,      &Graph_SetDefaultDomain,     NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"set_def_range",       ffilib_doc_set_def_range,       &Graph_SetDefaultRange,      NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"set_graph_color",     ffilib_doc_set_graph_color,     &Graph_SetBackground,        NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"set_domain",          ffilib_doc_set_domain,          &Graph_SetDomain,            NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"set_range",           ffilib_doc_set_range,           &Graph_SetRange,             NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"erase_plot",          ffilib_doc_erase_plot,          &Graph_ClearPlot,            NULL, TSFFI_PRIMITIVE_TYPE_VOID, 0},
                                                         {"hset_graph_color",    ffilib_doc_hset_graph_color,    &Graph_HSetBackground,       NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"hset_domain",         ffilib_doc_hset_domain,         &Graph_HSetDomain,           NULL, TSFFI_PRIMITIVE_TYPE_VOID, 3, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"hset_range",          ffilib_doc_hset_range,          &Graph_HSetRange,            NULL, TSFFI_PRIMITIVE_TYPE_VOID, 3, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"herase_plot",         ffilib_doc_herase_plot,         &Graph_HClearPlot,           NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_INT}},
                                                         {"set_def_plot_weight", ffilib_doc_set_def_plot_weight, &Graph_SetDefaultWeight,     NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"set_def_plot_mode",   ffilib_doc_set_def_plot_mode,   &Graph_SetDefaultMode,       NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"set_def_plot_color",  ffilib_doc_set_def_plot_color,  &Graph_SetDefaultColor,      NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"set_plot_weight",     ffilib_doc_set_plot_weight,     &Graph_SetWeight,            NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"set_plot_mode",       ffilib_doc_set_plot_mode,       &Graph_SetMode,              NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"set_plot_color",      ffilib_doc_set_plot_color,      &Graph_SetColor,             NULL, TSFFI_PRIMITIVE_TYPE_VOID, 1, {TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"hset_plot_weight",    ffilib_doc_hset_plot_weight,    &Graph_HSetWeight,           NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"hset_plot_mode",      ffilib_doc_hset_plot_mode,      &Graph_HSetMode,             NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"hset_plot_color",     ffilib_doc_hset_plot_color,     &Graph_HSetColor,            NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_STRING}},
                                                         {"plot",                ffilib_doc_plot,                &Graph_Plot,                 NULL, TSFFI_PRIMITIVE_TYPE_VOID, 2, {TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}},
                                                         {"hplot",               ffilib_doc_hplot,               &Graph_HPlot,                NULL, TSFFI_PRIMITIVE_TYPE_VOID, 3, {TSFFI_PRIMITIVE_TYPE_INT, TSFFI_PRIMITIVE_TYPE_REAL, TSFFI_PRIMITIVE_TYPE_REAL}}
                                                     };

struct tsffi_registration_group ffilib_groups[] = {
                                                      {_countof(notify_functions), notify_functions, &Notify_BeginModule, &Notify_ModuleState, &Notify_EndModule, NULL},
                                                      {_countof(math_functions),   math_functions,   &FFILib_BeginModule, NULL,                &FFILib_EndModule, NULL},
                                                      {_countof(time_functions),   time_functions,   &FFILib_BeginModule, NULL,                &FFILib_EndModule, NULL},
                                                      {_countof(graph_functions),  graph_functions,  &Graph_BeginModule,  NULL,                &Graph_EndModule,  NULL}
                                                  };


unsigned int ffilib_group_count = _countof(ffilib_groups);

