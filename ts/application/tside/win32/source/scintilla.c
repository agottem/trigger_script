/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "scintilla.h"
#include "alerts.h"
#include "error.h"

#include <malloc.h>
#include <scintilla/Scintilla.h>
#include <scintilla/SciLexer.h>


#define MAX_ANNOTATION_LINE_LENGTH 50

#define ANNOTATION_STYLE_OFFSET 512

#define ALERT_COMPILE_ERROR_STYLE   ANNOTATION_STYLE_OFFSET+1
#define ALERT_COMPILE_WARNING_STYLE ANNOTATION_STYLE_OFFSET+2
#define ALERT_RUNTIME_ERROR_STYLE   ANNOTATION_STYLE_OFFSET+3
#define ALERT_RUNTIME_WARNING_STYLE ANNOTATION_STYLE_OFFSET+4
#define ALERT_RUNTIME_MESSAGE_STYLE ANNOTATION_STYLE_OFFSET+5


static void SetScintillaStyle (int, struct tside_scintilla_style*, HWND);


static char* ts_keywords = "if else elseif end loop while for to downto continue "
                           "break finish and or not input output action true false";

struct tside_scintilla_configuration tside_default_configuration;


static void SetScintillaStyle (
                               int                           style_id,
                               struct tside_scintilla_style* style,
                               HWND                          window
                              )
{
    SendMessage(window, SCI_STYLESETFONT, style_id, (LPARAM)style->font);
    SendMessage(window, SCI_STYLESETSIZE, style_id, style->point_size);
    SendMessage(window, SCI_STYLESETFORE, style_id, style->foreground);
    SendMessage(window, SCI_STYLESETBACK, style_id, style->background);

    if(style->flags&TSIDE_SCINTILLA_STYLE_BOLD)
        SendMessage(window, SCI_STYLESETBOLD, style_id, 1);
    else
        SendMessage(window, SCI_STYLESETBOLD, style_id, 0);

    if(style->flags&TSIDE_SCINTILLA_STYLE_ITALIC)
        SendMessage(window, SCI_STYLESETITALIC, style_id, 1);
    else
        SendMessage(window, SCI_STYLESETITALIC, style_id, 0);

    if(style->flags&TSIDE_SCINTILLA_STYLE_UNDERLINE)
        SendMessage(window, SCI_STYLESETUNDERLINE, style_id, 1);
    else
        SendMessage(window, SCI_STYLESETUNDERLINE, style_id, 0);
}


int TSIDE_InitializeScintilla (void)
{
    tside_default_configuration.flags = TSIDE_SCINTILLA_CONFIG_SHOW_LINE_NUMBERS|
                                        TSIDE_SCINTILLA_CONFIG_TABS_AS_SPACES;

    tside_default_configuration.indent_size = 4;

    tside_default_configuration.execution_foreground_color = RGB(255, 255, 255);
    tside_default_configuration.execution_background_color = RGB(0, 200, 200);

    tside_default_configuration.default_style.font       = "Consolas";
    tside_default_configuration.default_style.point_size = 9;
    tside_default_configuration.default_style.flags      = 0;
    tside_default_configuration.default_style.foreground = RGB(0, 0, 0);
    tside_default_configuration.default_style.background = RGB(255, 255, 255);

    tside_default_configuration.line_number_style.font       = "Consolas";
    tside_default_configuration.line_number_style.point_size = 9;
    tside_default_configuration.line_number_style.flags      = 0;
    tside_default_configuration.line_number_style.foreground = RGB(0, 0, 0);
    tside_default_configuration.line_number_style.background = RGB(198, 195, 198);

    tside_default_configuration.brace_highlight_style.font       = "Consolas";
    tside_default_configuration.brace_highlight_style.point_size = 9;
    tside_default_configuration.brace_highlight_style.flags      = TSIDE_SCINTILLA_STYLE_BOLD;
    tside_default_configuration.brace_highlight_style.foreground = RGB(0, 0, 255);
    tside_default_configuration.brace_highlight_style.background = RGB(255, 255, 255);

    tside_default_configuration.brace_mismatch_style.font       = "Consolas";
    tside_default_configuration.brace_mismatch_style.point_size = 9;
    tside_default_configuration.brace_mismatch_style.flags      = TSIDE_SCINTILLA_STYLE_BOLD;
    tside_default_configuration.brace_mismatch_style.foreground = RGB(255, 0, 0);
    tside_default_configuration.brace_mismatch_style.background = RGB(255, 255, 255);

    tside_default_configuration.comment_style.font       = "Consolas";
    tside_default_configuration.comment_style.point_size = 9;
    tside_default_configuration.comment_style.flags      = 0;
    tside_default_configuration.comment_style.foreground = RGB(66, 134, 66);
    tside_default_configuration.comment_style.background = RGB(255, 255, 255);

    tside_default_configuration.number_style.font       = "Consolas";
    tside_default_configuration.number_style.point_size = 9;
    tside_default_configuration.number_style.flags      = 0;
    tside_default_configuration.number_style.foreground = RGB(0, 125, 123);
    tside_default_configuration.number_style.background = RGB(255, 255, 255);

    tside_default_configuration.keyword_style.font       = "Consolas";
    tside_default_configuration.keyword_style.point_size = 9;
    tside_default_configuration.keyword_style.flags      = TSIDE_SCINTILLA_STYLE_BOLD;
    tside_default_configuration.keyword_style.foreground = RGB(0, 0, 123);
    tside_default_configuration.keyword_style.background = RGB(255, 255, 255);

    tside_default_configuration.string_style.font       = "Consolas";
    tside_default_configuration.string_style.point_size = 9;
    tside_default_configuration.string_style.flags      = 0;
    tside_default_configuration.string_style.foreground = RGB(156, 0, 123);
    tside_default_configuration.string_style.background = RGB(255, 255, 255);

    tside_default_configuration.operator_style.font       = "Consolas";
    tside_default_configuration.operator_style.point_size = 9;
    tside_default_configuration.operator_style.flags      = TSIDE_SCINTILLA_STYLE_BOLD;
    tside_default_configuration.operator_style.foreground = RGB(183, 0, 0);
    tside_default_configuration.operator_style.background = RGB(255, 255, 255);

    tside_default_configuration.identifier_style.font       = "Consolas";
    tside_default_configuration.identifier_style.point_size = 9;
    tside_default_configuration.identifier_style.flags      = 0;
    tside_default_configuration.identifier_style.foreground = RGB(0, 0, 0);
    tside_default_configuration.identifier_style.background = RGB(255, 255, 255);

    tside_default_configuration.plugin_function_style.font       = "Consolas";
    tside_default_configuration.plugin_function_style.point_size = 9;
    tside_default_configuration.plugin_function_style.flags      = 0;
    tside_default_configuration.plugin_function_style.foreground = RGB(0, 0, 0);
    tside_default_configuration.plugin_function_style.background = RGB(255, 255, 255);

    tside_default_configuration.user_defined_function_style.font       = "Consolas";
    tside_default_configuration.user_defined_function_style.point_size = 9;
    tside_default_configuration.user_defined_function_style.flags      = 0;
    tside_default_configuration.user_defined_function_style.foreground = RGB(0, 0, 0);
    tside_default_configuration.user_defined_function_style.background = RGB(255, 255, 255);

    tside_default_configuration.alert_compile_error_style.font       = "Consolas";
    tside_default_configuration.alert_compile_error_style.point_size = 9;
    tside_default_configuration.alert_compile_error_style.flags      = 0;
    tside_default_configuration.alert_compile_error_style.foreground = RGB(148, 0, 66);
    tside_default_configuration.alert_compile_error_style.background = RGB(255, 235, 235);

    tside_default_configuration.alert_compile_warning_style.font       = "Consolas";
    tside_default_configuration.alert_compile_warning_style.point_size = 9;
    tside_default_configuration.alert_compile_warning_style.flags      = 0;
    tside_default_configuration.alert_compile_warning_style.foreground = RGB(173, 130, 0);
    tside_default_configuration.alert_compile_warning_style.background = RGB(255, 255, 235);

    tside_default_configuration.alert_runtime_error_style.font       = "Consolas";
    tside_default_configuration.alert_runtime_error_style.point_size = 9;
    tside_default_configuration.alert_runtime_error_style.flags      = 0;
    tside_default_configuration.alert_runtime_error_style.foreground = RGB(148, 0, 66);
    tside_default_configuration.alert_runtime_error_style.background = RGB(255, 235, 235);

    tside_default_configuration.alert_runtime_warning_style.font       = "Consolas";
    tside_default_configuration.alert_runtime_warning_style.point_size = 9;
    tside_default_configuration.alert_runtime_warning_style.flags      = 0;
    tside_default_configuration.alert_runtime_warning_style.foreground = RGB(173, 130, 0);
    tside_default_configuration.alert_runtime_warning_style.background = RGB(255, 255, 235);

    tside_default_configuration.alert_runtime_message_style.font       = "Consolas";
    tside_default_configuration.alert_runtime_message_style.point_size = 9;
    tside_default_configuration.alert_runtime_message_style.flags      = 0;
    tside_default_configuration.alert_runtime_message_style.foreground = RGB(0, 0, 0);
    tside_default_configuration.alert_runtime_message_style.background = RGB(255, 255, 255);

    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownScintilla (void)
{
}

int TSIDE_ConfigureScintilla (struct tside_scintilla_configuration* configuration, HWND window)
{
    SendMessage(window, SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED, 0);
    SendMessage(window, SCI_SETLEXER, SCLEX_TS, 0);
    SendMessage(window, SCI_SETKEYWORDS, 0, (LPARAM)ts_keywords);

    SetScintillaStyle(STYLE_DEFAULT,               &configuration->default_style,               window);
    SetScintillaStyle(STYLE_LINENUMBER,            &configuration->line_number_style,           window);
    SetScintillaStyle(STYLE_BRACELIGHT,            &configuration->brace_highlight_style,       window);
    SetScintillaStyle(STYLE_BRACEBAD,              &configuration->brace_mismatch_style,        window);
    SetScintillaStyle(SCE_TS_COMMENT,              &configuration->comment_style,               window);
    SetScintillaStyle(SCE_TS_NUMBER,               &configuration->number_style,                window);
    SetScintillaStyle(SCE_TS_WORD1,                &configuration->keyword_style,               window);
    SetScintillaStyle(SCE_TS_STRING,               &configuration->string_style,                window);
    SetScintillaStyle(SCE_TS_OPERATOR,             &configuration->operator_style,              window);
    SetScintillaStyle(SCE_TS_IDENTIFIER,           &configuration->identifier_style,            window);
    SetScintillaStyle(SCE_TS_WORD2,                &configuration->plugin_function_style,       window);
    SetScintillaStyle(SCE_TS_WORD3,                &configuration->user_defined_function_style, window);
    SetScintillaStyle(ALERT_COMPILE_ERROR_STYLE,   &configuration->alert_compile_error_style,   window);
    SetScintillaStyle(ALERT_COMPILE_WARNING_STYLE, &configuration->alert_compile_warning_style, window);
    SetScintillaStyle(ALERT_RUNTIME_ERROR_STYLE,   &configuration->alert_runtime_error_style,   window);
    SetScintillaStyle(ALERT_RUNTIME_WARNING_STYLE, &configuration->alert_runtime_warning_style, window);
    SetScintillaStyle(ALERT_RUNTIME_MESSAGE_STYLE, &configuration->alert_runtime_message_style, window);

    if(configuration->flags&TSIDE_SCINTILLA_CONFIG_SHOW_LINE_NUMBERS)
    {
        LRESULT margin_width;

        margin_width = SendMessage(window, SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"999_");

        SendMessage(window, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
        SendMessage(window, SCI_SETMARGINWIDTHN, 0, margin_width);
    }
    else
        SendMessage(window, SCI_SETMARGINWIDTHN, 0, 0);

    SendMessage(window, SCI_SETSCROLLWIDTH, 1, 0);
    SendMessage(window, SCI_SETSCROLLWIDTHTRACKING, 1, 0);

    if(configuration->flags&TSIDE_SCINTILLA_CONFIG_TABS_AS_SPACES)
        SendMessage(window, SCI_SETUSETABS, 0, 0);
    else
        SendMessage(window, SCI_SETUSETABS, 1, 0);

    SendMessage(window, SCI_SETTABWIDTH, configuration->indent_size, 0);
    SendMessage(window, SCI_SETINDENT, configuration->indent_size, 0);
    SendMessage(window, SCI_ANNOTATIONSETVISIBLE, ANNOTATION_BOXED, 0);
    SendMessage(window, SCI_ANNOTATIONSETSTYLEOFFSET, ANNOTATION_STYLE_OFFSET, 0);

    SendMessage(
                window,
                SCI_MARKERSETFORE,
                TSIDE_HIGHLIGHT_EXECUTION,
                configuration->execution_foreground_color
               );
    SendMessage(
                window,
                SCI_MARKERSETBACK,
                TSIDE_HIGHLIGHT_EXECUTION,
                configuration->execution_background_color
               );
    SendMessage(window, SCI_MARKERDEFINE, TSIDE_HIGHLIGHT_EXECUTION, SC_MARK_ARROW);

    return TSIDE_ERROR_NONE;
}

void TSIDE_SetScintillaText (char* text, HWND window)
{
    if(text == NULL)
        text = "";

    SendMessage(window, SCI_SETTEXT, 0, (LPARAM)text);
    SendMessage(window, SCI_SETSAVEPOINT, 0, 0);
    SendMessage(window, SCI_EMPTYUNDOBUFFER, 0, 0);
}

unsigned int TSIDE_ScintillaHasModifiedText (HWND window)
{
    LRESULT modified;

    modified = SendMessage(window, SCI_GETMODIFY, 0, 0);
    if(modified == 0)
        return TSIDE_TEXT_UNMODIFIED;

    return TSIDE_TEXT_MODIFIED;
}

int TSIDE_GetScintillaText (char** content, HWND window)
{
    char* fetched_content;
    int   document_length;

    document_length = SendMessage(window, SCI_GETLENGTH, 0, 0)+1;

    fetched_content = malloc(document_length);
    if(fetched_content == NULL)
        return TSIDE_ERROR_MEMORY;

    SendMessage(window, SCI_GETTEXT, document_length, (LPARAM)fetched_content);

    *content = fetched_content;

    return TSIDE_ERROR_NONE;
}

void TSIDE_ResetModifiedState (HWND window)
{
    SendMessage(window, SCI_SETSAVEPOINT, 0, 0);
}

void TSIDE_GotoScintillaLine (int line_number, HWND window)
{
    SendMessage(window, SCI_GOTOLINE, line_number, 0);
}

unsigned int TSIDE_SearchNextScintillaText (
                                            int   initial_position,
                                            char* find_text,
                                            char* replace_text,
                                            HWND  window
                                           )
{
    size_t text_length;
    int    current_position;
    int    end_position;

    text_length = strlen(find_text);

    if(replace_text != NULL)
    {
        current_position = SendMessage(window, SCI_SEARCHINTARGET, text_length, (LPARAM)find_text);
        if(current_position >= 0)
            SendMessage(window, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replace_text);
    }

    if(initial_position < 0)
        current_position = SendMessage(window, SCI_GETSELECTIONEND, 0, 0);
    else
        current_position = initial_position;

    end_position = SendMessage(window, SCI_GETLENGTH, 0, 0);

    SendMessage(window, SCI_SETTARGETSTART, current_position, 0);
    SendMessage(window, SCI_SETTARGETEND, end_position, 0);

    current_position = SendMessage(window, SCI_SEARCHINTARGET, text_length, (LPARAM)find_text);
    if(current_position < 0)
        return TSIDE_TEXT_NOT_FOUND;

    end_position = SendMessage(window, SCI_GETTARGETEND, 0, 0);

    SendMessage(window, SCI_SETSELECTIONSTART, current_position, 0);
    SendMessage(window, SCI_SETSELECTIONEND, end_position, 0);
    SendMessage(window, SCI_SCROLLCARET, 0, 0);

    return TSIDE_TEXT_FOUND;
}

unsigned int TSIDE_SearchPrevScintillaText (
                                            int   initial_position,
                                            char* find_text,
                                            char* replace_text,
                                            HWND  window
                                           )
{
    size_t text_length;
    int    current_position;
    int    end_position;

    text_length = strlen(find_text);

    if(replace_text != NULL)
    {
        current_position = SendMessage(window, SCI_SEARCHINTARGET, text_length, (LPARAM)find_text);
        if(current_position >= 0)
            SendMessage(window, SCI_REPLACETARGET, (WPARAM)-1, (LPARAM)replace_text);
    }

    if(initial_position < 0)
        current_position = SendMessage(window, SCI_GETSELECTIONSTART, 0, 0);
    else
        current_position = initial_position;

    end_position = 0;

    SendMessage(window, SCI_SETTARGETSTART, current_position, 0);
    SendMessage(window, SCI_SETTARGETEND, end_position, 0);

    current_position = SendMessage(window, SCI_SEARCHINTARGET, text_length, (LPARAM)find_text);
    if(current_position < 0)
        return TSIDE_TEXT_NOT_FOUND;

    end_position = SendMessage(window, SCI_GETTARGETEND, 0, 0);

    SendMessage(window, SCI_SETSELECTIONSTART, current_position, 0);
    SendMessage(window, SCI_SETSELECTIONEND, end_position, 0);
    SendMessage(window, SCI_SCROLLCARET, 0, 0);

    return TSIDE_TEXT_FOUND;
}

void TSIDE_ScintillaUndo (HWND window)
{
    SendMessage(window, SCI_UNDO, 0, 0);
}

void TSIDE_ScintillaRedo (HWND window)
{
    SendMessage(window, SCI_REDO, 0, 0);
}

void TSIDE_ScintillaCut (HWND window)
{
    SendMessage(window, SCI_CUT, 0, 0);
}

void TSIDE_ScintillaCopy (HWND window)
{
    SendMessage(window, SCI_COPY, 0, 0);
}

void TSIDE_ScintillaPaste (HWND window)
{
    SendMessage(window, SCI_PASTE, 0, 0);
}

void TSIDE_ScintillaSelectAll (HWND window)
{
    SendMessage(window, SCI_SELECTALL, 0, 0);
}

void TSIDE_ScintillaInsertText (char* text, HWND window)
{
    SendMessage(window, SCI_INSERTTEXT, (WPARAM)-1, (LPARAM)text);
}

int TSIDE_AnnotateScintilla (char* text, unsigned int location, unsigned int type, HWND window)
{
    char*  annotation_text;
    char*  copy_text;
    char*  new_string;
    char*  text_styles;
    size_t text_length;
    size_t new_lines_needed;
    size_t distance_to_new_line;
    size_t existing_annotation_size;
    size_t new_string_size;
    int    style;

    location--;

    text_length      = strlen(text);
    new_lines_needed = text_length/MAX_ANNOTATION_LINE_LENGTH;

    new_string = malloc((text_length+new_lines_needed+1)*2);
    if(new_string == NULL)
        goto allocate_new_string_failed;

    copy_text            = new_string;
    distance_to_new_line = MAX_ANNOTATION_LINE_LENGTH;

    do
    {
        *copy_text = *text;

        copy_text++;
        text++;

        distance_to_new_line--;

        if(distance_to_new_line == 0)
        {

            do
            {
                copy_text--;
                text--;
            }while(copy_text != new_string && !(*copy_text == ' ' || *copy_text == '\t'));

            *copy_text = '\n';

            copy_text++;

            if(*text == ' ' || *text == '\t')
                text++;

            distance_to_new_line = MAX_ANNOTATION_LINE_LENGTH;
        }
    }while(*text != 0);

    *copy_text = 0;

    switch(type)
    {
    case TSIDE_ALERT_TYPE_COMPILE_ERROR:
        style = ALERT_COMPILE_ERROR_STYLE;

        break;

    case TSIDE_ALERT_TYPE_COMPILE_WARNING:
        style = ALERT_COMPILE_WARNING_STYLE;

        break;

    case TSIDE_ALERT_TYPE_RUNTIME_ERROR:
        style = ALERT_RUNTIME_ERROR_STYLE;

        break;

    case TSIDE_ALERT_TYPE_RUNTIME_WARNING:
        style = ALERT_RUNTIME_WARNING_STYLE;

        break;

    default:
    case TSIDE_ALERT_TYPE_RUNTIME_MESSAGE:
        style = ALERT_RUNTIME_MESSAGE_STYLE;

        break;
    }

    new_string_size          = strlen(new_string);
    existing_annotation_size = SendMessage(window, SCI_ANNOTATIONGETTEXT, location, 0);
    if(existing_annotation_size != 0)
    {
        size_t new_annotation_size;

        new_annotation_size = existing_annotation_size+new_string_size+2;

        annotation_text = malloc(new_annotation_size*2);
        if(annotation_text == NULL)
            goto allocate_annotation_text_failed;

        text_styles = &annotation_text[new_annotation_size];

        SendMessage(window, SCI_ANNOTATIONGETTEXT, location, (LPARAM)annotation_text);
        SendMessage(window, SCI_ANNOTATIONGETSTYLES, location, (LPARAM)text_styles);

        annotation_text[existing_annotation_size]   = '\n';
        existing_annotation_size++;
        annotation_text[existing_annotation_size] = 0;

        strcat(annotation_text, new_string);

        free(new_string);
    }
    else
    {
        annotation_text = new_string;
        text_styles     = copy_text+1;
    }

    memset(
           &text_styles[existing_annotation_size],
           style-ANNOTATION_STYLE_OFFSET,
           new_string_size
          );

    SendMessage(window, SCI_ANNOTATIONSETTEXT, location, (LPARAM)annotation_text);
    SendMessage(window, SCI_ANNOTATIONSETSTYLES, location, (LPARAM)text_styles);

    free(annotation_text);

    return TSIDE_ERROR_NONE;

allocate_annotation_text_failed:
    free(new_string);

allocate_new_string_failed:
    return TSIDE_ERROR_MEMORY;
}

void TSIDE_ClearScintillaAnnotations (HWND window)
{
    SendMessage(window, SCI_ANNOTATIONCLEARALL, 0, 0);
}

void TSIDE_HighlightScintillaLine (unsigned int line, unsigned int type, HWND window)
{
    line--;

    switch(type)
    {
    case TSIDE_HIGHLIGHT_NONE:
        SendMessage(window, SCI_MARKERDELETE, line, -1);

        break;

    case TSIDE_HIGHLIGHT_EXECUTION:
        SendMessage(window, SCI_MARKERADD, line, type);
        SendMessage(window, SCI_GOTOLINE, line, 0);

        break;
    }
}

void TSIDE_SetScintillaReadOnly (unsigned int mode, HWND window)
{
    switch(mode)
    {
    case TSIDE_READONLY:
        SendMessage(window, SCI_SETREADONLY, TRUE, 0);

        break;

    case TSIDE_WRITABLE:
        SendMessage(window, SCI_SETREADONLY, FALSE, 0);

        break;
    }
}

