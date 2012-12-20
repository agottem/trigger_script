/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _FFILIB_IDHASH_H_
#define _FFILIB_IDHASH_H_


#include <stdlib.h>


#define FFILIB_ID_HASH_SIZE 256


struct ffilib_id_hash_node
{
    unsigned int id;
    void*        data;

    struct ffilib_id_hash_node* next_node;
    struct ffilib_id_hash_node* previous_node;
};

struct ffilib_id_hash
{
    struct ffilib_id_hash_node* id_hash[FFILIB_ID_HASH_SIZE];
};


#define FFILIB_DECLARE_STATIC_ID_HASH(name) static struct ffilib_id_hash name = {NULL}


extern void FFILib_InitializeHash (struct ffilib_id_hash*);
extern void FFILib_DestroyHash    (struct ffilib_id_hash*);

extern int  FFILib_AddID    (unsigned int, void*, struct ffilib_id_hash*);
extern void FFILib_RemoveID (unsigned int, struct ffilib_id_hash*);

extern void* FFILib_GetIDData (unsigned int, struct ffilib_id_hash*);


#endif

