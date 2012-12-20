/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_SYNC_H_
#define _TSINT_SYNC_H_


#ifdef PLATFORM_WIN32
    #include <windows.h>

    typedef HANDLE           tsint_event;
    typedef CRITICAL_SECTION tsint_mutex;
#else
    #error "Unsupported platform selected"
#endif


struct tsint_module_abort_signal
{
    tsint_event abort_signal;
};

struct tsint_module_sync_data
{
    tsint_event action_signal;
    tsint_mutex action_state_sync;

    struct tsint_module_abort_signal* abort_signal;
};


extern int  TSInt_InitializeSyncData (
                                      struct tsint_module_sync_data*,
                                      struct tsint_module_abort_signal*
                                     );
extern void TSInt_DestroySyncData    (struct tsint_module_sync_data*);

extern void TSInt_LockActionStateSync   (struct tsint_module_sync_data*);
extern void TSInt_UnlockActionStateSync (struct tsint_module_sync_data*);

extern int  TSInt_ListenForAction (struct tsint_module_sync_data*);
extern void TSInt_SignalAction    (struct tsint_module_sync_data*);
extern void TSInt_ClearSignal     (struct tsint_module_sync_data*);

extern int  TSInt_InitializeAbortSignal (struct tsint_module_abort_signal*);
extern void TSInt_DestroyAbortSignal    (struct tsint_module_abort_signal*);

extern void TSInt_SetAbortSignal   (struct tsint_module_abort_signal*);
extern void TSInt_ClearAbortSignal (struct tsint_module_abort_signal*);
extern int  TSInt_TestAbortSignal  (struct tsint_module_sync_data*);


#endif

