/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include "ref_templates.h"
#include "gui.h"
#include "ide.h"
#include "text.h"
#include "file.h"
#include "main.h"
#include "resources.h"
#include "scintilla.h"
#include "paths.h"
#include "error.h"

#include <malloc.h>
#include <string.h>
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>


#define MAX_CATEGORY_NAME_LENGTH 1024


struct category_data
{
    struct tside_template_data* templates;
};

struct ref_templates_data
{
    HWND ref_template_window;
    HWND category_window;
    HWND template_window;
    HWND content_window;

    struct tside_scintilla_configuration content_configuration;
};


static int  FindTemplates         (void);
static void DestroyFoundTemplates (void);

static int  InitializeDialog (HWND);
static void DestroyDialog    (struct ref_templates_data*);
static void PopulateControls (struct ref_templates_data*);

static void SelectCategory (int, struct ref_templates_data*);
static void SelectTemplate (int, struct ref_templates_data*);

static void UseTemplate (struct ref_templates_data*);


struct tside_template_data* tside_found_templates;


static int FindTemplates (void)
{
    WIN32_FIND_DATA find_data;
    WCHAR*          search_path;
    WCHAR*          template_path;
    char*           file_content;
    HANDLE          find_handle;
    size_t          path_length;
    size_t          alloc_size;

    tside_found_templates = NULL;

    path_length = wcslen(tside_templates_path_w);

    alloc_size  = path_length+sizeof(L"\\*.ts")+1;
    search_path = malloc(sizeof(WCHAR)*alloc_size);
    if(search_path == NULL)
        goto alloc_search_path_failed;

    wcscpy(search_path, tside_templates_path_w);
    wcscat(search_path, L"\\*.ts");

    find_handle = FindFirstFile(search_path, &find_data);
    if(find_handle == NULL)
        goto no_files_found;

    do
    {
        WCHAR                       template_name[MAX_PATH];
        WCHAR                       template_category[MAX_PATH];
        WCHAR*                      name_category_separator;
        WCHAR*                      replace_position;
        struct tside_template_data* template;
        size_t                      divided_length;
        int                         error;

        name_category_separator = wcschr(find_data.cFileName, '-');
        if(name_category_separator == NULL || name_category_separator == find_data.cFileName)
            continue;

        divided_length = name_category_separator-find_data.cFileName;

        wcsncpy(template_category, find_data.cFileName, divided_length);

        template_category[divided_length] = 0;
        replace_position                  = template_category;

        while(*replace_position != 0)
        {
            if(*replace_position == '_')
                *replace_position = ' ';

            replace_position++;
        }

        name_category_separator++;
        if(*name_category_separator == 0 || *name_category_separator == '.')
            continue;

        wcscpy(template_name, name_category_separator);

        replace_position = wcsrchr(template_name, '.');
        if(replace_position == NULL)
            continue;

        *replace_position = 0;

        replace_position = template_name;
        while(*replace_position != 0)
        {
            if(*replace_position == '_')
                *replace_position = ' ';

            replace_position++;
        }

        alloc_size    = path_length+wcslen(find_data.cFileName)+sizeof("\\")+1;
        template_path = malloc(alloc_size*sizeof(WCHAR));
        if(template_path == NULL)
            goto allocate_template_path_failed;

        wcscpy(template_path, tside_templates_path_w);
        wcscat(template_path, L"\\");
        wcscat(template_path, find_data.cFileName);

        error = TSIDE_GetFileText(template_path, &file_content);
        if(error != TSIDE_ERROR_NONE)
            goto get_file_text_failed;

        template = malloc(sizeof(struct tside_template_data));
        if(template == NULL)
            goto allocate_template_failed;

        free(template_path);

        template->content = file_content;

        wcscpy(template->category_name, template_category);
        wcscpy(template->template_name, template_name);

        template->next_template = tside_found_templates;
        tside_found_templates   = template;
    }while(FindNextFile(find_handle, &find_data) != 0);

    FindClose(find_handle);

no_files_found:
    free(search_path);

    return TSIDE_ERROR_NONE;

allocate_template_failed:
    free(file_content);
get_file_text_failed:
    free(template_path);
allocate_template_path_failed:
    free(search_path);

alloc_search_path_failed:
    return TSIDE_ERROR_SYSTEM_CALL_FAILED;
}

static void DestroyFoundTemplates (void)
{
    struct tside_template_data* template;

    template = tside_found_templates;
    while(template != NULL)
    {
        struct tside_template_data* free_template;

        free_template = template;
        template      = template->next_template;

        free(free_template->content);
        free(free_template);
    }
}

static int InitializeDialog (HWND window)
{
    struct ref_templates_data* data;
    HWND                       scintilla_window;
    int                        error;

    data = malloc(sizeof(struct ref_templates_data));
    if(data == NULL)
        goto allocate_ref_templates_data_failed;

    EnableThemeDialogTexture(window, ETDT_USETABTEXTURE|ETDT_ENABLE);

    scintilla_window = GetDlgItem(window, TSIDE_C_REF_TEMPLATES_CONTENT);

    data->ref_template_window   = window;
    data->category_window       = GetDlgItem(window, TSIDE_C_REF_TEMPLATES_CATEGORY);
    data->template_window       = GetDlgItem(window, TSIDE_C_REF_TEMPLATES_TEMPLATE);
    data->content_window        = scintilla_window;
    data->content_configuration = tside_default_configuration;

    data->content_configuration.flags &= ~TSIDE_SCINTILLA_CONFIG_SHOW_LINE_NUMBERS;

    error = TSIDE_ConfigureScintilla(&data->content_configuration, scintilla_window);
    if(error != TSIDE_ERROR_NONE)
        goto configure_scintilla_failed;

    TSIDE_SetScintillaReadOnly(TSIDE_READONLY, scintilla_window);

    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)data);

    PopulateControls(data);
    SelectCategory(0, data);

    return TSIDE_ERROR_NONE;

configure_scintilla_failed:
    free(data);

allocate_ref_templates_data_failed:
    return TSIDE_ERROR_REFERENCE_ERROR;
}

static void DestroyDialog (struct ref_templates_data* data)
{
    LRESULT item_count;

    item_count = SendMessage(data->category_window, LB_GETCOUNT, 0, 0);
    if(item_count != LB_ERR)
    {
        while(item_count--)
        {
            LRESULT item_data;

            item_data = SendMessage(data->category_window, LB_GETITEMDATA, item_count, 0);
            if(item_data == LB_ERR || item_data == 0)
                continue;

            free((void*)item_data);
        }
    }

    free(data);
}

static void PopulateControls (struct ref_templates_data* data)
{
    struct category_data*       category;
    struct tside_template_data* template;
    HWND                        category_window;

    category_window = data->category_window;

    for(template = tside_found_templates; template != NULL; template = template->next_template)
    {
        WCHAR*  template_category;
        LRESULT existing_category;

        template_category = template->category_name;

        existing_category = SendMessage(
                                        category_window,
                                        LB_FINDSTRING,
                                        (WPARAM)-1,
                                        (LPARAM)template_category
                                       );
        if(existing_category == LB_ERR)
        {
            category = malloc(sizeof(struct category_data));
            if(category == NULL)
                goto allocate_category_data_failed;

            category->templates = NULL;

            existing_category = SendMessage(
                                            category_window,
                                            LB_ADDSTRING,
                                            0,
                                            (LPARAM)template_category
                                           );
            if(existing_category == LB_ERR || existing_category == LB_ERRSPACE)
                goto add_category_failed;

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

        template->next_category_template = category->templates;
        category->templates              = template;
    }

    return;

add_category_failed:
    free(category);

allocate_category_data_failed:
get_category_failed:
    MessageBox(
               tside_reference_window,
               TSIDE_GetResourceText(TSIDE_S_CANNOT_POPULATE_REFERENCE_DATA),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

static void SelectCategory (int category_index, struct ref_templates_data* data)
{
    struct category_data*       category;
    struct tside_template_data* template;
    HWND                        template_window;
    HWND                        category_window;
    LRESULT                     item_data;

    category_window = data->category_window;

    SendMessage(category_window, LB_SETCURSEL, category_index, 0);

    item_data = SendMessage(category_window, LB_GETITEMDATA, category_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    category = (struct category_data*)item_data;

    template_window = data->template_window;

    SendMessage(template_window, LB_RESETCONTENT, 0, 0);

    for(template = category->templates; template != NULL; template = template->next_category_template)
    {
        LRESULT item_index;

        item_index = SendMessage(template_window, LB_ADDSTRING, 0, (LPARAM)template->template_name);
        if(item_index == LB_ERR || item_index == LB_ERRSPACE)
            goto add_template_failed;

        SendMessage(template_window, LB_SETITEMDATA, item_index, (LPARAM)template);
    }

    SelectTemplate(0, data);

    return;

add_template_failed:
    MessageBox(
               tside_reference_window,
               TSIDE_GetResourceText(TSIDE_S_CANNOT_POPULATE_REFERENCE_DATA),
               TSIDE_GetResourceText(TSIDE_S_GENERIC_ERROR_CAPTION),
               MB_ICONINFORMATION|MB_OK
              );
}

static void SelectTemplate (int template_index, struct ref_templates_data* data)
{
    struct tside_template_data* template;
    HWND                        template_window;
    HWND                        content_window;
    LRESULT                     item_data;

    template_window = data->template_window;

    SendMessage(template_window, LB_SETCURSEL, template_index, 0);

    item_data = SendMessage(template_window, LB_GETITEMDATA, template_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    template = (struct tside_template_data*)item_data;

    content_window = data->content_window;

    TSIDE_SetScintillaReadOnly(TSIDE_WRITABLE, content_window);

    TSIDE_SetScintillaText(template->content, content_window);

    TSIDE_SetScintillaReadOnly(TSIDE_READONLY, content_window);
}

static void UseTemplate (struct ref_templates_data* data)
{
    struct tside_template_data* template;
    HWND                        template_window;
    LRESULT                     item_data;
    LRESULT                     template_index;

    template_window = data->template_window;

    template_index = SendMessage(template_window, LB_GETCURSEL, 0, 0);
    if(template_index == LB_ERR)
        return;

    item_data = SendMessage(template_window, LB_GETITEMDATA, template_index, 0);
    if(item_data == LB_ERR || item_data == 0)
        return;

    template = (struct tside_template_data*)item_data;

    SendMessage(
                tside_ide_window,
                TSIDE_MESSAGE_USE_TEMPLATE,
                0,
                (LPARAM)template->content
               );
}


int TSIDE_InitializeRefTemplates (void)
{
    int error;

    error = FindTemplates();
    if(error != TSIDE_ERROR_NONE)
        goto find_templates_failed;

    return TSIDE_ERROR_NONE;

find_templates_failed:
    DestroyFoundTemplates();

    return error;
}

void TSIDE_ShutdownRefTemplates (void)
{
    DestroyFoundTemplates();
}

INT_PTR CALLBACK TSIDE_RefTemplatesMessageProc (
                                                HWND   window,
                                                UINT   message,
                                                WPARAM wparam,
                                                LPARAM lparam
                                               )
{
    struct ref_templates_data* data;
    HWND                       origin_window;
    LRESULT                    item_index;
    int                        error;
    WORD                       notification_code;
    WORD                       command;

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
        data = (struct ref_templates_data*)GetWindowLongPtr(window, GWLP_USERDATA);
        if(data == NULL)
            return FALSE;

        DestroyDialog(data);

        return TRUE;

    case WM_COMMAND:
        data = (struct ref_templates_data*)GetWindowLongPtr(window, GWLP_USERDATA);
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
            else if(origin_window == data->template_window)
                SelectTemplate(item_index, data);

            return TRUE;
        }

        command = LOWORD(wparam);

        switch(command)
        {
        case TSIDE_C_REF_TEMPLATES_USE_TEMPLATE:
            UseTemplate(data);

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

initialize_dialog_failed:
    PostQuitMessage(TSIDE_ERROR_REF_TEMPLATES_ERROR);

    return FALSE;
}

