/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <notify/message.h>

#include <tsffi/error.h>

#include <windows.h>
#include <stdio.h>


int Notify_Print (
                  struct tsffi_invocation_data* invocation_data,
                  void*                         group_data,
                  union tsffi_value*            output,
                  union tsffi_value*            input
                 )
{
    char   location[512];
    char*  output_string;
    char*  string_data;
    size_t string_length;

    _snprintf(
              location,
              sizeof(location),
              "function=%s line=%d: ",
              invocation_data->unit_name,
              invocation_data->unit_location
             );

    location[sizeof(location)-1] = 0;

    string_length = strlen(location);

    string_data = input->string_data;

    string_length += strlen(string_data);

    output_string = malloc(string_length+1);
    if(output_string == NULL)
        return TSFFI_ERROR_EXCEPTION;

    strcpy(output_string, location);
    strcat(output_string, string_data);

    invocation_data->execif->alert(
                                   invocation_data->execif_data,
                                   TSFFI_ALERT_MESSAGE,
                                   output_string
                                  );

    free(output_string);

    return TSFFI_ERROR_NONE;
}

int Notify_Message (
                    struct tsffi_invocation_data* invocation_data,
                    void*                         group_data,
                    union tsffi_value*            output,
                    union tsffi_value*            input
                   )
{
    char caption[512];
    int  error;

    _snprintf(
              caption,
              sizeof(caption),
              "%s, line: %d",
              invocation_data->unit_name,
              invocation_data->unit_location
             );

    caption[sizeof(caption)-1] = 0;

    error = MessageBox(
                       NULL,
                       input->string_data,
                       caption,
                       MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_SERVICE_NOTIFICATION
                      );
    if(error == 0)
        return TSFFI_ERROR_EXCEPTION;

    return TSFFI_ERROR_NONE;
}

int Notify_Choice (
                   struct tsffi_invocation_data* invocation_data,
                   void*                         group_data,
                   union tsffi_value*            output,
                   union tsffi_value*            input
                  )
{
    char caption[512];
    int  choice;

    _snprintf(
              caption,
              sizeof(caption),
              "%s, line: %d",
              invocation_data->unit_name,
              invocation_data->unit_location
             );

    caption[sizeof(caption)-1] = 0;

    choice = MessageBox(
                        NULL,
                        input->string_data,
                        caption,
                        MB_YESNO|MB_ICONQUESTION|MB_SETFOREGROUND|MB_SERVICE_NOTIFICATION
                       );
    if(choice == IDYES)
        output->bool_data = TSFFI_BOOL_TRUE;
    else if(choice == IDNO)
        output->bool_data = TSFFI_BOOL_FALSE;
    else
        return TSFFI_ERROR_EXCEPTION;

    return TSFFI_ERROR_NONE;
}

