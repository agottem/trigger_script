/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "ref_plugins.h"
#include "gui.h"
#include "ide.h"
#include "text.h"
#include "main.h"
#include "resources.h"
#include "scintilla.h"
#include "plugins.h"
#include "error.h"

#include <malloc.h>
#include <string.h>
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>


#define MAX_CATEGORY_NAME_LENGTH 1024


struct function_data
{
    WCHAR* name;

    char* insert_function_text;
    char* documentation_text;

    struct tsffi_function_definition* definition;

    struct function_data* next_function;
};

struct category_data
{
    struct function_data* functions;
};

struct ref_plugins_data
{
    HWND ref_plugin_window;
    HWND category_window;
    HWND function_window;
    HWND documentation_window;

    struct tside_scintilla_configuration documentation_configuration;
};


static int  InitializeDialog (HWND);
static void DestroyDialog    (struct ref_plugins_data*);
static void AddFunction      (struct tsffi_function_definition*, struct ref_plugins_data*);
static void PopulateControls (struct ref_plugins_data*);

static void SelectCategory (int, struct ref_plugins_data*);
static void SelectFunction (int, struct ref_plugins_data*);

static void InsertFunction (struct ref_plugins_data*);


char* undocumented_function_text = "# This function has not been documented.  Contact\n# the developer of the plugin for more information.";


static int InitializeDialog (HWND window)
{
    struct ref_plugins_data* data;
    HWND                     scintilla_window;
    int                      error;

    data = malloc(sizeof(struct ref_plugins_data));
    if(data == NULL)
        goto allocate_ref_plugins_data_failed;

    EnableThemeDialogTexture(window, ETDT_USETABTEXTURE|ETDT_ENABLE);

    scintilla_window = GetDlgItem(window, TSIDE_C_REF_PLUGINS_DOCUMENTATION);

    data->ref_plugin_window           = window;
    data->category_window             = GetDlgItem(window, TSIDE_C_REF_PLUGINS_CATEGORY);
    data->function_window             = GetDlgItem(window, TSIDE_C_REF_PLUGINS_FUNCTION);
    data->documentation_window        = scintilla_window;
    data->documentation_configuration = tside_default_configuration;

    data->documentation_configuration.flags &= ~TSIDE_SCINTILLA_CONFIG_SHOW_LINE_NUMBERS;

    error = TSIDE_ConfigureScintilla(&data->documentation_configuration, scintilla_window);
    if(error != TSIDE_ERROR_NONE)
        goto configure_scintilla_failed;

    TSIDE_SetScintillaReadOnly(TSIDE_READONLY, scintilla_window);

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    PopulateControls(data);
    SelectCategory(0, data);

    return TSIDE_ERROR_NONE;

configure_scintilla_failed:
    free(data);

allocate_ref_plugins_data_failed:
    return TSIDE_ERROR_REFERENCE_ERROR;
}

static void DestroyDialog (struct ref_plugins_data* data)
{
    LRESULT item_count;

    item_count = SendMessage(data->category_window, LB_GETCOUNT, 0, 0);
    if(item_count != LB_ERR)
    {
        while(item_count--)
        {
            struct category_data* category;
            struct function_data* function;
            LRESULT               item_data;

            item_data = SendMessage(data->category_window, LB_GETITEMDATA, item_count, 0);
            if(item_data == LB_ERR || item_data == 0)
                continue;

            category = (struct category_data*)item_data;
            function = category->functions;

            while(function != NULL)
            {
                struct function_data* free_function;

                free_function = function;
                function      = function->next_function;

                if(free_function->insert_function_text != NULL)
                    free(free_function->insert_function_text);

                free(free_function->name);
                free(free_function);
            }

            free(category);
        }
    }

    free(data);
}

static void AddFunction (struct tsffi_function_definition* function, struct ref_plugins_data* data)
{
    WCHAR                 category_name_from_doc[MAX_CATEGORY_NAME_LENGTH];
    WCHAR*                category_name;
    struct category_data* category;
    struct function_data* scan_functions;
    struct function_data* category_function;
    char*                 documentation_text;
    char*                 insertion_text;
    WCHAR*                function_name;
    HWND                  category_window;
    HWND                  function_window;
    size_t                function_name_length;
    LRESULT               existing_category;

    insertion_text     = NULL;
    documentation_text = function->documentation;
    if(documentation_text != NULL)
    {
        char* category_separator;

        category_separator = strchr(documentation_text, '\n');
        if(category_separator == NULL || category_separator == documentation_text)
            category_name = TSIDE_GetResourceText(TSIDE_S_UNCATEGORIZED_FUNCTION_CATEGORY);
        else
        {
            char*  insertion_text_separator;
            size_t actual_category_length;
            size_t adjusted_category_length;

            actual_category_length   = category_separator-documentation_text;
            adjusted_category_length = actual_category_length;
            if(actual_category_length > MAX_CATEGORY_NAME_LENGTH-1)
                adjusted_category_length = MAX_CATEGORY_NAME_LENGTH-1;

            mbstowcs(
                     category_name_from_doc,
                     documentation_text,
                     adjusted_category_length
                    );

            category_name_from_doc[adjusted_category_length] = 0;
            category_name                                    = category_name_from_doc;

            documentation_text += actual_category_length+1;

            insertion_text_separator = strchr(documentation_text, '\n');
            if(insertion_text_separator != NULL && insertion_text_separator != documentation_text)
            {
                size_t insertion_size;

                insertion_size = insertion_text_separator-documentation_text+1;
                insertion_text = malloc(insertion_size);
                if(insertion_text == NULL)
                    goto allocate_insertion_text_failed;

                strncpy(insertion_text, documentation_text, insertion_size-1);

                insertion_text[insertion_size-1] = 0;

                documentation_text += insertion_size;
            }
        }
    }
    else
        category_name = TSIDE_GetResourceText(TSIDE_S_UNCATEGORIZED_FUNCTION_CATEGORY);

    category_window = data->category_window;

    existing_category = SendMessage(category_window, LB_FINDSTRING, (WPARAM)-1, (LPARAM)category_name);
    if(existing_category == LB_ERR)
    {
        category = malloc(sizeof(struct category_data));
        if(category == NULL)
            goto allocate_category_data_failed;

        category->functions = NULL;

        existing_category = SendMessage(category_window, LB_ADDSTRING, 0, (LPARAM)category_name);
        if(existing_category == LB_ERR || existing_category == LB_ERRSPACE)
        {
            free(category);

            goto add_category_failed;
        }

        SendMessage(category_window, LB_SETITEMDATA, existing_category, (LPARAM)category);
    }
    else
    {
        category = (struct category_data*)SendMessage(
                                                      category_window,
                                                      LB_GETITEMDATA,
                                                      existing_category,
                                                      0
                                                     );
        if(category == NULL)
            goto get_category_failed;
    }

    function_window = data->function_window;

    function_name_length = strlen(function->name)+1;
    function_name        = malloc(sizeof(WCHAR)*function_name_length);
    if(function_name == NULL)
        goto allocate_function_name_failed;

    mbstowcs(function_name, function->name, function_name_length);

    for(
        scan_functions = category->functions;
        scan_functions != NULL;
        scan_functions = scan_functions->next_function
       )
    {
        int delta;

        delta = wcscmp(function_name, scan_functions->name);
        if(delta == 0)
        {
            if(insertion_text != NULL)
                free(insertion_text);

            free(function_name);

            return;
        }
    }

    category_function = malloc(sizeof(struct function_data));
    if(category_function == NULL)
        goto allocate_category_functin_failed;

    category_function->name                 = function_name;
    category_function->insert_function_text = insertion_text;
    category_function->documentation_text   = documentation_text;
    category_function->definition           = function;
    category_function->next_function        = category->functions;
    category->functions                     = category_function;

    return;

allocate_category_functin_failed:
    free(function_name);
allocate_function_name_failed:
get_category_failed:
add_category_failed:
allocate_category_data_failed:
    if(insertion_text != NULL)
        free(insertion_text);
allocate_insertion_text_failed:
    MessageBox(
               tside_reference_window,
               TSIDE_GetResourceText(TSIDE_S_CANNOT_POPULATE_REFERENCE_DATA),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

static void PopulateControls (struct ref_plugins_data* data)
{
    struct tside_registered_plugin* scan_plugins;

    for(
        scan_plugins = tside_available_plugins;
        scan_plugins != NULL;
        scan_plugins = scan_plugins->next_plugin
       )
    {
        struct tsffi_registration_group* scan_groups;
        unsigned int                     group_count;

        scan_groups = scan_plugins->groups;
        group_count = scan_plugins->count;

        while(group_count--)
        {
            struct tsffi_function_definition* scan_functions;
            unsigned int                      function_count;

            scan_functions = scan_groups->functions;
            function_count = scan_groups->function_count;

            while(function_count--)
            {
                AddFunction(scan_functions, data);

                scan_functions++;
            }

            scan_groups++;
        }
    }
}

static void SelectCategory (int category_index, struct ref_plugins_data* data)
{
    struct category_data* category;
    struct function_data* function;
    HWND                  function_window;
    HWND                  category_window;
    LRESULT               item_data;

    category_window = data->category_window;

    SendMessage(category_window, LB_SETCURSEL, category_index, 0);

    item_data = SendMessage(category_window, LB_GETITEMDATA, category_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    category = (struct category_data*)item_data;

    function_window = data->function_window;

    SendMessage(function_window, LB_RESETCONTENT, 0, 0);

    for(function = category->functions; function != NULL; function = function->next_function)
    {
        LRESULT item_index;

        item_index = SendMessage(function_window, LB_ADDSTRING, 0, (LPARAM)function->name);
        if(item_index == LB_ERR || item_index == LB_ERRSPACE)
            goto add_function_failed;

        SendMessage(function_window, LB_SETITEMDATA, item_index, (LPARAM)function);
    }

    SelectFunction(0, data);

    return;

add_function_failed:
    MessageBox(
               tside_reference_window,
               TSIDE_GetResourceText(TSIDE_S_CANNOT_POPULATE_REFERENCE_DATA),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

static void SelectFunction (int function_index, struct ref_plugins_data* data)
{
    struct function_data* function;
    HWND                  function_window;
    HWND                  documentation_window;
    LRESULT               item_data;

    function_window = data->function_window;

    SendMessage(function_window, LB_SETCURSEL, function_index, 0);

    item_data = SendMessage(function_window, LB_GETITEMDATA, function_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    function = (struct function_data*)item_data;

    documentation_window = data->documentation_window;

    TSIDE_SetScintillaReadOnly(TSIDE_WRITABLE, documentation_window);

    if(function->documentation_text != NULL)
        TSIDE_SetScintillaText(function->documentation_text, documentation_window);
    else
        TSIDE_SetScintillaText(undocumented_function_text, documentation_window);

    TSIDE_SetScintillaReadOnly(TSIDE_READONLY, documentation_window);
}

static void InsertFunction (struct ref_plugins_data* data)
{
    struct function_data* function;
    HWND                  function_window;
    LRESULT               item_data;
    LRESULT               function_index;

    function_window = data->function_window;

    function_index = SendMessage(function_window, LB_GETCURSEL, 0, 0);
    if(function_index == LB_ERR)
        return;

    item_data = SendMessage(function_window, LB_GETITEMDATA, function_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    function = (struct function_data*)item_data;
    if(function->insert_function_text == NULL)
        return;

    SendMessage(
                tside_ide_window,
                TSIDE_MESSAGE_INSERT_TEXT,
                0,
                (LPARAM)function->insert_function_text
               );
}


int TSIDE_InitializeRefPlugins (void)
{
    return TSIDE_ERROR_NONE;
}

void TSIDE_ShutdownRefPlugins (void)
{
}

INT_PTR CALLBACK TSIDE_RefPluginsMessageProc (
                                              HWND   window,
                                              UINT   message,
                                              WPARAM wparam,
                                              LPARAM lparam
                                             )
{
    struct ref_plugins_data* data;
    HWND                     origin_window;
    LRESULT                  item_index;
    int                      error;
    WORD                     notification_code;
    WORD                     command;

    switch(message)
    {
    case WM_INITDIALOG:
        error = InitializeDialog(window);
        if(error != TSIDE_ERROR_NONE)
            goto initialize_dialog_failed;

        return TRUE;

    case WM_CLOSE:
        return TRUE;

    case WM_DESTROY:
        data = (struct ref_plugins_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_COMMAND:
        data = (struct ref_plugins_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        notification_code = HIWORD(wparam);

        switch(notification_code)
        {
        case LBN_SELCHANGE:
            origin_window = (HWND)lparam;

            item_index = SendMessage(origin_window, LB_GETCURSEL, 0, 0);
            if(item_index == LB_ERR)
                return FALSE;

            if(origin_window == data->category_window)
                SelectCategory(item_index, data);
            else if(origin_window == data->function_window)
                SelectFunction(item_index, data);

            return TRUE;
        }

        command = LOWORD(wparam);

        switch(command)
        {
        case TSIDE_C_REF_PLUGINS_INSERT_FUNCTION:
            InsertFunction(data);

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_REF_PLUGINS_ERROR);

    return FALSE;
}

