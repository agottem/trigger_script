/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_EXPEVAL_H_
#define _TSINT_EXPEVAL_H_


#include <tsdef/def.h>
#include <tsint/module.h>


extern int TSInt_PrimaryExpEvaluation    (
                                          struct tsdef_primary_exp*,
                                          struct tsint_unit_state*,
                                          union tsint_value*
                                         );
extern int TSInt_ComparisonExpEvaluation (
                                          struct tsdef_comparison_exp*,
                                          struct tsint_unit_state*,
                                          union tsint_value*
                                         );
extern int TSInt_LogicalExpEvaluation    (
                                          struct tsdef_logical_exp*,
                                          struct tsint_unit_state*,
                                          union tsint_value*
                                         );

extern int TSInt_EvaluateExp (
                              unsigned int,
                              struct tsdef_exp*,
                              struct tsint_unit_state*,
                              union tsint_value*
                             );


#endif

