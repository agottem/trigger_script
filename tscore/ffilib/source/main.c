/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <ffilib/thread.h>
#include <ffilib/module.h>
#include <ffilib/error.h>

#include "groups.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>


BOOL WINAPI DllMain (HANDLE dll_instance, DWORD command, LPVOID reserved);

void RegisterTSFFI  (struct tsffi_registration_group**, unsigned int*);
void ConfigureTSFFI (char*, char*);


static char configure_validation[]     = {
                                          0x77,
                                          0x68,
                                          0x6f,
                                          0x77,
                                          0x72,
                                          0x6f,
                                          0x74,
                                          0x65,
                                          0x6d,
                                          0x65,
                                          0x00
                                         };
static char configure_validation_ack[] = {
                                          0x41,
                                          0x6e,
                                          0x64,
                                          0x72,
                                          0x65,
                                          0x77,
                                          0x20,
                                          0x47,
                                          0x6f,
                                          0x74,
                                          0x74,
                                          0x65,
                                          0x6d,
                                          0x6f,
                                          0x6c,
                                          0x6c,
                                          0x65,
                                          0x72,
                                          0x00
                                         };


BOOL WINAPI DllMain (HANDLE instance, DWORD command, LPVOID reserved)
{
    switch(command)
    {
    case DLL_PROCESS_ATTACH:
        FFILib_InitializeModuleData((HINSTANCE)instance);

        return TRUE;

    case DLL_THREAD_ATTACH:
        return TRUE;

    case DLL_THREAD_DETACH:
        return TRUE;

    case DLL_PROCESS_DETACH:
        FFILib_ShutdownModuleData();

        return TRUE;
    }

    return FALSE;
}

void RegisterTSFFI (struct tsffi_registration_group** registration_group, unsigned int* count)
{
    *count              = ffilib_group_count;
    *registration_group = ffilib_groups;
}

void ConfigureTSFFI (char* variable, char* value)
{
    int delta;

    delta = strcmp(variable, configure_validation);
    if(delta == 0)
        printf("%s\n", configure_validation_ack);
}

