/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSINT_EXPOP_H_
#define _TSINT_EXPOP_H_


#include <tsdef/def.h>
#include <tsint/module.h>


typedef int (*tsint_exp_primary_op_if)    (
                                           unsigned int,
                                           union tsint_value,
                                           union tsint_value*,
                                           union tsint_value*
                                          );
typedef int (*tsint_exp_comparison_op_if) (
                                           unsigned int,
                                           union tsint_value,
                                           union tsint_value,
                                           union tsint_value*
                                          );


extern int TSInt_BoolPrimaryExpOp   (
                                     unsigned int,
                                     union tsint_value,
                                     union tsint_value*,
                                     union tsint_value*
                                    );
extern int TSInt_IntPrimaryExpOp    (
                                     unsigned int,
                                     union tsint_value,
                                     union tsint_value*,
                                     union tsint_value*
                                    );
extern int TSInt_RealPrimaryExpOp   (
                                     unsigned int,
                                     union tsint_value,
                                     union tsint_value*,
                                     union tsint_value*
                                    );
extern int TSInt_StringPrimaryExpOp (
                                     unsigned int,
                                     union tsint_value,
                                     union tsint_value*,
                                     union tsint_value*
                                    );

extern int TSInt_BoolComparisonExpOp   (
                                        unsigned int,
                                        union tsint_value,
                                        union tsint_value,
                                        union tsint_value*
                                       );
extern int TSInt_IntComparisonExpOp    (
                                        unsigned int,
                                        union tsint_value,
                                        union tsint_value,
                                        union tsint_value*
                                       );
extern int TSInt_RealComparisonExpOp   (
                                        unsigned int,
                                        union tsint_value,
                                        union tsint_value,
                                        union tsint_value*
                                       );
extern int TSInt_StringComparisonExpOp (
                                        unsigned int,
                                        union tsint_value,
                                        union tsint_value,
                                        union tsint_value*
                                       );


#endif

