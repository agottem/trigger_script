/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_ACTION_H_
#define _TSINT_ACTION_H_


#include <tsdef/def.h>
#include <tsint/module.h>


#define TSINT_ACTION_PENDING   0
#define TSINT_ACTION_TRIGGERED 1
#define TSINT_ACTION_FINISHED  2


extern int  TSInt_InitActions    (struct tsint_unit_state*, struct tsdef_action**);
extern void TSInt_StopActions    (struct tsint_unit_state*);
extern int  TSInt_EvaluateAction (struct tsint_action_state*, unsigned int*);
extern int  TSInt_PrepActionRun  (struct tsint_action_state*);
extern int  TSInt_UpdateAction   (struct tsint_action_state*);


#endif

