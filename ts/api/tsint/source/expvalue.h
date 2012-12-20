/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_EXPVALUE_H_
#define _TSINT_EXPVALUE_H_


#include <tsdef/def.h>
#include <tsint/module.h>


typedef int (*tsint_extract_exp_value_if) (
                                           struct tsdef_exp_value_type*,
                                           struct tsint_unit_state*,
                                           union tsint_value*
                                          );


extern int TSInt_ExpValueAsBool   (
                                   struct tsdef_exp_value_type*,
                                   struct tsint_unit_state*,
                                   union tsint_value*
                                  );
extern int TSInt_ExpValueAsInt    (
                                   struct tsdef_exp_value_type*,
                                   struct tsint_unit_state*,
                                   union tsint_value*
                                  );
extern int TSInt_ExpValueAsReal   (
                                   struct tsdef_exp_value_type*,
                                   struct tsint_unit_state*,
                                   union tsint_value*
                                  );
extern int TSInt_ExpValueAsString (
                                   struct tsdef_exp_value_type*,
                                   struct tsint_unit_state*,
                                   union tsint_value*
                                  );


#endif

