/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <ffilib/idhash.h>
#include <ffilib/error.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>


static unsigned int ComputeHash (unsigned int);



static unsigned int ComputeHash (unsigned int id)
{
    return id%FFILIB_ID_HASH_SIZE;
}


void FFILib_InitializeHash (struct ffilib_id_hash* hash)
{
    memset(hash, 0, sizeof(struct ffilib_id_hash));
}

void FFILib_DestroyHash (struct ffilib_id_hash* hash)
{
    unsigned int index;

    index = FFILIB_ID_HASH_SIZE;
    while(index--)
    {
        struct ffilib_id_hash_node* node;

        node = hash->id_hash[index];
        while(node != NULL)
        {
            struct ffilib_id_hash_node* free_node;

            free_node = node;

            node = node->next_node;

            free(free_node);
        }
    }
}

int FFILib_AddID (unsigned int id, void* data, struct ffilib_id_hash* hash_data)
{
    struct ffilib_id_hash_node** node;
    struct ffilib_id_hash_node*  entry;
    unsigned int                 hash;

    hash = ComputeHash(id);
    node = &hash_data->id_hash[hash];

    entry = *node;
    while(entry != NULL)
    {
        if(entry->id == id)
            break;

        entry = entry->next_node;
    }

    if(entry == NULL)
    {
        entry = malloc(sizeof(struct ffilib_id_hash_node));
        if(entry == NULL)
            return FFILIB_ERROR_MEMORY;

        entry->previous_node = NULL;
        entry->next_node     = *node;
        *node                = entry;

        if(entry->next_node != NULL)
            entry->next_node->previous_node = entry;

        entry->id = id;
    }

    entry->data = data;

    return FFILIB_ERROR_NONE;
}

void FFILib_RemoveID (unsigned int id, struct ffilib_id_hash* hash_data)
{
    struct ffilib_id_hash_node** node;
    struct ffilib_id_hash_node*  entry;
    unsigned int                 hash;

    hash = ComputeHash(id);
    node = &hash_data->id_hash[hash];

    entry = *node;

    while(entry != NULL)
    {
        if(entry->id == id)
        {
            if(entry->next_node != NULL)
                entry->next_node->previous_node = entry->previous_node;
            if(entry->previous_node != NULL)
                entry->previous_node->next_node = entry->next_node;
            else
                *node = entry->next_node;

            free(entry);

            break;
        }

        entry = entry->next_node;
    }
}

void* FFILib_GetIDData (unsigned int id, struct ffilib_id_hash* hash_data)
{
    struct ffilib_id_hash_node*  entry;
    unsigned int                 hash;

    hash  = ComputeHash(id);
    entry = hash_data->id_hash[hash];

    while(entry != NULL)
    {
        if(entry->id == id)
            return entry->data;

        entry = entry->next_node;
    }

    return NULL;
}

