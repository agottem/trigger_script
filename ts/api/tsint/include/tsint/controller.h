/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_CONTROLLER_H_
#define _TSINT_CONTROLLER_H_


#define TSINT_CONTROL_RUN       0
#define TSINT_CONTROL_STEP      1
#define TSINT_CONTROL_STEP_INTO 2
#define TSINT_CONTROL_HALT      3


struct tsint_unit_state;

typedef unsigned int (*tsint_controller) (struct tsint_unit_state*, void*);

struct tsint_controller_data
{
    tsint_controller function;
    void*            user_data;
};


#endif

