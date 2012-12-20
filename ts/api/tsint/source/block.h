/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_BLOCK_H_
#define _TSINT_BLOCK_H_


#include <tsdef/def.h>
#include <tsint/module.h>


extern int  TSInt_StartBlock  (
                               struct tsdef_block*,
                               struct tsdef_statement*,
                               struct tsint_unit_state*,
                               struct tsdef_statement**
                              );
extern void TSInt_FinishBlock (struct tsint_unit_state*, struct tsdef_statement**);


#endif

