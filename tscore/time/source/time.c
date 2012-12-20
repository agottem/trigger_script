/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <time/time.h>

#include <tsffi/error.h>

#include <windows.h>


#define FILETIME_TO_MS_COEFFICIENT (double)(1.0/10000.0)


int Time_Time (
               struct tsffi_invocation_data* invocation_data,
               void*                         group_data,
               union tsffi_value*            output,
               union tsffi_value*            input
              )
{
    ULARGE_INTEGER large_int;
    SYSTEMTIME     system_time;
    FILETIME       file_time;
    double         time_ms;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);

    large_int.u.HighPart = file_time.dwHighDateTime;
    large_int.u.LowPart  = file_time.dwLowDateTime;

    time_ms = (double)large_int.QuadPart*FILETIME_TO_MS_COEFFICIENT;

    output->real_data = (tsffi_real)time_ms;

    return TSFFI_ERROR_NONE;
}

int Time_Delay (
                struct tsffi_invocation_data* invocation_data,
                void*                         group_data,
                union tsffi_value*            output,
                union tsffi_value*            input
               )
{
    DWORD sleep_time;

    sleep_time = (DWORD)input->real_data;

    if(sleep_time > 0)
        Sleep(sleep_time);

    return TSFFI_ERROR_NONE;
}

