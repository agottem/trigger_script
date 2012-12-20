/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_UNIT_H_
#define _TSINT_UNIT_H_


#include <tsdef/def.h>
#include <tsint/module.h>


extern int TSInt_ControlModeForInvokedUnit (int);

extern int  TSInt_ExpListToValueArray (
                                       struct tsdef_exp_list*,
                                       struct tsdef_input*,
                                       struct tsint_unit_state*,
                                       union tsint_value**
                                      );
extern void TSInt_DestroyValueArray   (struct tsdef_input*, union tsint_value*);

extern int TSInt_InvokeUnit (
                             struct tsdef_unit*,
                             union tsint_value*,
                             union tsint_value*,
                             int*,
                             struct tsint_module_state*
                            );

extern int  TSInt_ProcessUnitAction (struct tsint_action_state*, int*);
extern void TSInt_StopUnit          (struct tsint_unit_state*);


#endif

