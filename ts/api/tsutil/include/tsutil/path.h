/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSUTIL_PATH_H_
#define _TSUTIL_PATH_H_


#include <stddef.h>


struct tsutil_path
{
    char* path;

    struct tsutil_path* next_path;
};

struct tsutil_path_collection
{
    struct tsutil_path* first_path;
    struct tsutil_path* last_path;

    size_t max_path_length;
};


extern void TSUtil_InitializePathCollection (struct tsutil_path_collection*);
extern void TSUtil_DestroyPathCollection    (struct tsutil_path_collection*);

extern int TSUtil_AppendPath  (char*, struct tsutil_path_collection*);
extern int TSUtil_PrependPath (char*, struct tsutil_path_collection*);

extern int TSUtil_FindUnitFile (char*, char*, struct tsutil_path_collection*, char**);


#endif

