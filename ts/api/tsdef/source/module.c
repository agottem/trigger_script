/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsdef/module.h>
#include <tsdef/error.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>


#define HASH_FNV_PRIME 16777619


static unsigned int ComputeHash (char*);


static unsigned int ComputeHash (char* name)
{
    unsigned int hash;

    hash = 0;
    while(*name != 0)
    {
        hash *= HASH_FNV_PRIME;
        hash ^= *name;

        name++;
    }

    hash = hash%TSDEF_MODULE_OBJECT_HASH_SIZE;

    return hash;
}


void TSDef_InitializeModule (struct tsdef_module* module)
{
    struct tsdef_module_object_hash_node* hash_map;
    unsigned int                          index;

    hash_map = module->hash_map;
    index    = TSDEF_MODULE_OBJECT_HASH_SIZE;
    while(index--)
    {
        hash_map->template_unit_objects = NULL;
        hash_map->typed_units_objects   = NULL;
        hash_map->ff_objects   = NULL;

        hash_map++;
    }

    module->main_unit                  = NULL;
    module->registered_ffi_group_count = 0;
    module->referenced_unit_count      = 0;
    module->unresolved_unit_objects    = NULL;
    module->referenced_unit_objects    = NULL;
    module->template_unit_objects      = NULL;
    module->referenced_ffi_objects     = NULL;
    module->registered_ffi_objects     = NULL;
    module->referenced_ffi_groups      = NULL;
    module->registered_ffi_groups      = NULL;
}

void TSDef_DestroyModule (struct tsdef_module* module)
{
    struct tsdef_module_object*    unit_groups[3];
    struct tsdef_module_ffi_group* ffi_groups[2];
    unsigned int                   index;

    unit_groups[0] = module->unresolved_unit_objects;
    unit_groups[1] = module->referenced_unit_objects;
    unit_groups[2] = module->template_unit_objects;

    for(index = 0; index < _countof(unit_groups); index++)
    {
        struct tsdef_module_object* module_object;

        module_object = unit_groups[index];
        while(module_object != NULL)
        {
            struct tsdef_module_object* free_module_object;

            if(!(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_SHARED_OBJECT))
            {
                TSDef_DestroyUnit(module_object->type.unit);

                if(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_FREE_UNIT)
                    free(module_object->type.unit);
            }

            free_module_object = module_object;
            module_object      = module_object->next_module_object;

            free(free_module_object);
        }
    }

    ffi_groups[0] = module->registered_ffi_groups;
    ffi_groups[1] = module->referenced_ffi_groups;

    for(index = 0; index < _countof(ffi_groups); index++)
    {
        struct tsdef_module_ffi_group* group;

        group = ffi_groups[index];
        while(group != NULL)
        {
            struct tsdef_module_ffi_group* free_group;

            free(group->name);

            free_group = group;
            group      = group->next_group;

            free(free_group);
        }
    }
}

int TSDef_LookupModuleObject (
                              char*                                 name,
                              struct tsdef_argument_types*          argument_types,
                              struct tsdef_module*                  module,
                              struct tsdef_module_object**          found_module_object,
                              struct tsdef_module_object_type_info* type_info
                             )
{
    struct tsdef_ffi_argument_match       best_match_info;
    struct tsdef_module_object_hash_node* node;
    struct tsdef_module_object*           module_object;
    struct tsdef_module_object*           matching_ffi_object;
    unsigned int                          hash;
    int                                   lookup_result;

    lookup_result = TSDEF_ERROR_NONE;

    hash = ComputeHash(name);
    node = &module->hash_map[hash];

    for(
        module_object = node->typed_units_objects;
        module_object != NULL;
        module_object = module_object->next_hash_module_object
       )
    {
        struct tsdef_unit* unit;
        int                delta;
        int                match;

        unit = module_object->type.unit;

        delta = strcmp(unit->name, name);
        if(delta != 0)
            continue;

        match = TSDef_ArgumentTypesMatchInput(unit->input, argument_types);
        if(match == TSDEF_ARGUMENT_MATCH)
            goto match_found;
        else if(match == TSDEF_ARGUMENT_COUNT_MISMATCH)
            goto argument_mismatch;
    }

    for(
        module_object = node->typed_units_objects;
        module_object != NULL;
        module_object = module_object->next_hash_module_object
       )
    {
        struct tsdef_unit* unit;
        int                delta;
        int                match;

        unit = module_object->type.unit;

        delta = strcmp(unit->name, name);
        if(delta != 0)
            continue;

        match = TSDef_ArgumentCountMatchInput(unit->input, argument_types);
        if(match == TSDEF_ARGUMENT_MATCH)
            goto match_found;
        else if(match == TSDEF_ARGUMENT_COUNT_MISMATCH)
            goto argument_mismatch;
    }

    matching_ffi_object = NULL;
    for(
        module_object = node->ff_objects;
        module_object != NULL;
        module_object = module_object->next_hash_module_object
       )
    {
        struct tsdef_ffi_argument_match   match_info;
        struct tsffi_function_definition* ffi;
        int                               delta;
        int                               match;

        ffi = module_object->type.ffi.function_definition;

        delta = strcmp(ffi->name, name);
        if(delta != 0)
            continue;

        match = TSDef_ArgumentTypesMatchFFI(ffi, argument_types, &match_info);
        if(match == TSDEF_ARGUMENT_MATCH || match == TSDEF_ARGUMENT_TYPE_MISMATCH)
        {
            if(matching_ffi_object != NULL)
            {
                if(match_info.delta < best_match_info.delta)
                {
                    matching_ffi_object = module_object;
                    best_match_info     = match_info;
                }
            }
            else
            {
                matching_ffi_object = module_object;
                best_match_info     = match_info;
            }
        }
        else if(match == TSDEF_ARGUMENT_COUNT_MISMATCH)
            goto argument_mismatch;
    }

    if(matching_ffi_object != NULL)
    {
        module_object = matching_ffi_object;

        if(best_match_info.allow_conversion == TSDEF_PRIMITIVE_CONVERSION_DISALLOWED)
        {
            type_info->argument_index = best_match_info.argument_index;
            type_info->from_type      = best_match_info.from_type;
            type_info->to_type        = best_match_info.to_type;

            lookup_result = TSDEF_ARGUMENT_TYPE_MISMATCH;
        }

        goto match_found;
    }

    return TSDEF_ERROR_MODULE_OBJECT_NOT_FOUND;

match_found:
    if(found_module_object != NULL)
        *found_module_object = module_object;

    return lookup_result;

argument_mismatch:
    *found_module_object = module_object;

    return TSDEF_ERROR_MODULE_OBJECT_ARGUMENT_COUNT;
}

struct tsdef_module_object* TSDef_LookupName (char* name, struct tsdef_module* module)
{
    struct tsdef_module_object_hash_node* node;
    struct tsdef_module_object*           module_object;
    unsigned int                          hash;

    hash = ComputeHash(name);

    node = &module->hash_map[hash];

    for(
        module_object = node->typed_units_objects;
        module_object != NULL;
        module_object = module_object->next_hash_module_object
       )
    {
        struct tsdef_unit* unit;
        int                delta;

        unit = module_object->type.unit;

        delta = strcmp(unit->name, name);
        if(delta == 0)
            return module_object;
    }

    for(
        module_object = node->typed_units_objects;
        module_object != NULL;
        module_object = module_object->next_hash_module_object
       )
    {
        struct tsdef_unit* unit;
        int                delta;

        unit = module_object->type.unit;

        delta = strcmp(unit->name, name);
        if(delta == 0)
            return module_object;
    }

    for(
        module_object = node->ff_objects;
        module_object != NULL;
        module_object = module_object->next_hash_module_object
       )
    {
        struct tsffi_function_definition* ffi;
        int                               delta;

        ffi = module_object->type.ffi.function_definition;

        delta = strcmp(ffi->name, name);
        if(delta == 0)
            return module_object;
    }

    return NULL;
}

struct tsdef_module_object* TSDef_AllocateUnitModuleObject (
                                                            struct tsdef_unit* unit,
                                                            unsigned int       flags
                                                           )
{
    struct tsdef_module_object* module_object;

    module_object = malloc(sizeof(struct tsdef_module_object));
    if(module_object == NULL)
        return NULL;

    module_object->type.unit  = unit;
    module_object->flags      = TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT|flags;

    return module_object;
}

void TSDef_AddUnitModuleObject (
                                struct tsdef_module_object* module_object,
                                struct tsdef_module*        module
                               )
{
    struct tsdef_module_object_hash_node* node;
    unsigned int                          hash;
    unsigned int                          flags;

    flags = module_object->flags;

    hash = ComputeHash(module_object->type.unit->name);
    node = &module->hash_map[hash];

    if(flags&TSDEF_MODULE_OBJECT_FLAG_TYPED_UNIT)
    {
        module_object->next_hash_module_object = node->typed_units_objects;
        node->typed_units_objects              = module_object;

        module_object->next_module_object = module->unresolved_unit_objects;
        module->unresolved_unit_objects   = module_object;
    }
    else
    {
        module_object->next_hash_module_object = node->template_unit_objects;
        node->template_unit_objects            = module_object;

        module_object->next_module_object = module->template_unit_objects;
        module->template_unit_objects     = module_object;
    }

    if(module_object->next_module_object != NULL)
        module_object->next_module_object->previous_module_object = module_object;

    module_object->previous_module_object = NULL;
}

int TSDef_AddFFIGroup (
                       char*                            name,
                       struct tsffi_registration_group* ffi_group,
                       struct tsdef_module*             module
                      )
{
    struct tsdef_module_ffi_group*    module_ffi_group;
    struct tsffi_function_definition* function;
    size_t                            alloc_size;
    unsigned int                      function_count;

    function_count = ffi_group->function_count;

    alloc_size = sizeof(struct tsdef_module_ffi_group)+sizeof(struct tsdef_module_object)*function_count;
    module_ffi_group = malloc(alloc_size);
    if(module_ffi_group == NULL)
        goto allocate_group_failed;

    module_ffi_group->name = strdup(name);
    if(module_ffi_group->name == NULL)
        goto duplicate_name_failed;

    module_ffi_group->group       = ffi_group;
    module_ffi_group->group_id    = module->registered_ffi_group_count;
    module_ffi_group->flags       = 0;
    module_ffi_group->next_group  = module->registered_ffi_groups;
    module->registered_ffi_groups = module_ffi_group;
    if(module_ffi_group->next_group != NULL)
        module_ffi_group->next_group->previous_group = module_ffi_group;

    module_ffi_group->previous_group = NULL;

    function = ffi_group->functions;
    while(function_count--)
    {
        struct tsdef_module_object*           module_ffi;
        struct tsdef_module_object_hash_node* node;
        unsigned int                          hash;

        module_ffi = &module_ffi_group->group_ffi[function_count];
        hash       = ComputeHash(function->name);

        node = &module->hash_map[hash];

        module_ffi->type.ffi.function_definition = function;
        module_ffi->type.ffi.group               = module_ffi_group;
        module_ffi->flags                        = TSDEF_MODULE_OBJECT_FLAG_FFI_OBJECT;
        module_ffi->next_hash_module_object      = node->ff_objects;

        node->ff_objects = module_ffi;

        module_ffi->next_module_object = module->registered_ffi_objects;
        if(module_ffi->next_hash_module_object != NULL)
            module_ffi->next_hash_module_object->previous_module_object = module_ffi;

        module_ffi->previous_module_object = NULL;
        module->registered_ffi_objects     = module_ffi;

        function++;
    }

    module->registered_ffi_group_count++;

    return TSDEF_ERROR_NONE;

duplicate_name_failed:
    free(module_ffi_group);

allocate_group_failed:
    return TSDEF_ERROR_MEMORY;
}

void TSDef_SetModuleMain (struct tsdef_unit* unit, struct tsdef_module* module)
{
    module->main_unit = unit;
}

void TSDef_MarkUnitResolved (
                             struct tsdef_module_object* module_object,
                             struct tsdef_module*        module
                            )
{
    module_object->type.unit->unit_id = module->referenced_unit_count;
    module->referenced_unit_count++;

    module_object->flags |= TSDEF_MODULE_OBJECT_FLAG_REFERENCED;

    if(module_object->next_module_object != NULL)
        module_object->next_module_object->previous_module_object = module_object->previous_module_object;
    if(module_object->previous_module_object != NULL)
        module_object->previous_module_object->next_module_object = module_object->next_module_object;
    else
        module->unresolved_unit_objects = module_object->next_module_object;

    module_object->next_module_object = module->referenced_unit_objects;
    if(module_object->next_module_object != NULL)
        module_object->next_module_object->previous_module_object = module_object;

    module->referenced_unit_objects = module_object;
}

void TSDef_ReferenceFFI (struct tsdef_module_object* module_object, struct tsdef_module* module)
{
    struct tsdef_module_ffi_group* ffi_group;

    ffi_group = module_object->type.ffi.group;

    if(!(ffi_group->flags&TSDEF_MODULE_FFI_GROUP_FLAG_REFERENCED))
    {
        if(ffi_group->next_group != NULL)
            ffi_group->next_group->previous_group = ffi_group->previous_group;
        if(ffi_group->previous_group != NULL)
            ffi_group->previous_group->next_group = ffi_group->next_group;
        else
            module->registered_ffi_groups = ffi_group->next_group;

        ffi_group->next_group = module->referenced_ffi_groups;
        if(ffi_group->next_group != NULL)
            ffi_group->next_group->previous_group = ffi_group;

        ffi_group->previous_group     = NULL;
        module->referenced_ffi_groups = ffi_group;

        ffi_group->flags |= TSDEF_MODULE_FFI_GROUP_FLAG_REFERENCED;
    }

    if(!(module_object->flags&TSDEF_MODULE_OBJECT_FLAG_REFERENCED))
    {
        if(module_object->next_module_object != NULL)
            module_object->next_module_object->previous_module_object = module_object->previous_module_object;
        if(module_object->previous_module_object != NULL)
            module_object->previous_module_object->next_module_object = module_object->next_module_object;
        else
            module->registered_ffi_objects = module_object->next_module_object;

        module_object->next_module_object = module->referenced_ffi_objects;
        if(module_object->next_module_object != NULL)
            module_object->next_module_object->previous_module_object = module_object;

        module_object->previous_module_object = NULL;
        module->referenced_ffi_objects        = module_object;

        module_object->flags |= TSDEF_MODULE_OBJECT_FLAG_REFERENCED;
    }
}

