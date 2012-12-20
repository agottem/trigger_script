/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSIDE_VARIABLES_H_
#define _TSIDE_VARIABLES_H_


struct tside_variable
{
    char* name;
    char* value;

    struct tside_variable* next_variable;
};


extern struct tside_variable* tside_set_variables;


extern int  TSIDE_InitializeVariables (void);
extern void TSIDE_ShutdownVariables   (void);


#endif

