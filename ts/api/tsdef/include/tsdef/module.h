/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#ifndef _TSDEF_MODULE_H_
#define _TSDEF_MODULE_H_


#include <tsdef/def.h>
#include <tsdef/arguments.h>
#include <tsffi/register.h>


#define TSDEF_MODULE_OBJECT_FLAG_UNIT_OBJECT   0x01
#define TSDEF_MODULE_OBJECT_FLAG_FFI_OBJECT    0x02
#define TSDEF_MODULE_OBJECT_FLAG_TYPED_UNIT    0x04
#define TSDEF_MODULE_OBJECT_FLAG_SHARED_OBJECT 0x08
#define TSDEF_MODULE_OBJECT_FLAG_FREE_UNIT     0x10
#define TSDEF_MODULE_OBJECT_FLAG_REFERENCED    0x20

#define TSDEF_MODULE_FFI_GROUP_FLAG_REFERENCED 0x01

#define TSDEF_MODULE_OBJECT_HASH_SIZE 2048


struct tsdef_module_object
{
    union
    {
        struct tsdef_unit* unit;

        struct
        {
            struct tsffi_function_definition* function_definition;
            struct tsdef_module_ffi_group*    group;
        }ffi;
    }type;

    unsigned int flags;

    struct tsdef_module_object* next_hash_module_object;

    struct tsdef_module_object* next_module_object;
    struct tsdef_module_object* previous_module_object;
};

struct tsdef_module_ffi_group
{
    struct tsffi_registration_group* group;
    unsigned int                     group_id;

    char* name;

    unsigned int flags;

    struct tsdef_module_ffi_group* next_group;
    struct tsdef_module_ffi_group* previous_group;

    struct tsdef_module_object group_ffi[];
};

struct tsdef_module_object_hash_node
{
    struct tsdef_module_object* template_unit_objects;
    struct tsdef_module_object* typed_units_objects;
    struct tsdef_module_object* ff_objects;
};

struct tsdef_module
{
    struct tsdef_module_object_hash_node hash_map[TSDEF_MODULE_OBJECT_HASH_SIZE];

    struct tsdef_unit* main_unit;

    unsigned int registered_ffi_group_count;
    unsigned int referenced_unit_count;

    struct tsdef_module_object* unresolved_unit_objects;
    struct tsdef_module_object* referenced_unit_objects;
    struct tsdef_module_object* template_unit_objects;

    struct tsdef_module_object* referenced_ffi_objects;
    struct tsdef_module_object* registered_ffi_objects;

    struct tsdef_module_ffi_group* referenced_ffi_groups;
    struct tsdef_module_ffi_group* registered_ffi_groups;
};

struct tsdef_module_object_type_info
{
    unsigned int argument_index;
    unsigned int from_type;
    unsigned int to_type;
};


extern void TSDef_InitializeModule (struct tsdef_module*);
extern void TSDef_DestroyModule    (struct tsdef_module*);

extern int                         TSDef_LookupModuleObject (
                                                             char*,
                                                             struct tsdef_argument_types*,
                                                             struct tsdef_module*,
                                                             struct tsdef_module_object**,
                                                             struct tsdef_module_object_type_info*
                                                            );
extern struct tsdef_module_object* TSDef_LookupName         (char*, struct tsdef_module*);

extern struct tsdef_module_object* TSDef_AllocateUnitModuleObject (
                                                                   struct tsdef_unit*,
                                                                   unsigned int
                                                                  );

extern void TSDef_AddUnitModuleObject (
                                       struct tsdef_module_object*,
                                       struct tsdef_module*
                                      );
extern int  TSDef_AddFFIGroup         (
                                       char*,
                                       struct tsffi_registration_group*,
                                       struct tsdef_module*
                                      );

extern void TSDef_SetModuleMain    (struct tsdef_unit*, struct tsdef_module*);
extern void TSDef_MarkUnitResolved (struct tsdef_module_object*, struct tsdef_module*);
extern void TSDef_ReferenceFFI     (struct tsdef_module_object*, struct tsdef_module*);


#endif

