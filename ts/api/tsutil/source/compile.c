/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsutil/compile.h>
#include <tsutil/error.h>

#include <tsdef/def.h>
#include <tsdef/construct.h>
#include <tsdef/resolve.h>
#include <tsdef/arguments.h>
#include <tsdef/module.h>
#include <tsdef/error.h>

#include <stdlib.h>
#include <string.h>


struct lookup_data
{
    unsigned int                   warning_count;
    char*                          unit_extension;
    struct tsutil_path_collection* path_collection;
    struct tsdef_def_error_list*   def_errors;

    notify_lookup_function notify_lookup;
};


static int ModuleObjectLookup (
                               char*                        name,
                               struct tsdef_argument_types* types,
                               void*                        user_data,
                               struct tsdef_module_object** found_module_object
                              )
{
    struct lookup_data*         lookup_user_data;
    struct tsdef_module_object* module_object;
    char*                       file_name;
    struct tsdef_unit*          unit;
    int                         error;

    lookup_user_data = user_data;

    if(lookup_user_data->notify_lookup != NULL)
        lookup_user_data->notify_lookup(name);

    error = TSUtil_FindUnitFile(
                                name,
                                lookup_user_data->unit_extension,
                                lookup_user_data->path_collection,
                                &file_name
                               );
    if(error != TSUTIL_ERROR_NONE)
    {
        if(error == TSUTIL_ERROR_NOT_FOUND)
            return TSDEF_ERROR_MODULE_OBJECT_NOT_FOUND;
        else
            return TSDEF_ERROR_MEMORY;
    }

    unit = malloc(sizeof(struct tsdef_unit));
    if(unit == NULL)
    {
        free(file_name);

        return TSDEF_ERROR_MEMORY;
    }

    error = TSDef_ConstructUnitFromFile(
                                        file_name,
                                        name,
                                        unit,
                                        lookup_user_data->def_errors
                                       );

    free(file_name);

    if(error != TSDEF_ERROR_NONE)
    {
        if(error != TSDEF_ERROR_CONSTRUCT_WARNING)
        {
            free(unit);

            return TSDEF_ERROR_MODULE_OBJECT_NOT_FOUND;
        }

        lookup_user_data->warning_count++;
    }

    module_object = TSDef_AllocateUnitModuleObject(unit, TSDEF_MODULE_OBJECT_FLAG_FREE_UNIT);
    if(module_object == NULL)
    {
        TSDef_DestroyUnit(unit);
        free(unit);

        return TSDEF_ERROR_MEMORY;
    }

    *found_module_object = module_object;

    return TSDEF_ERROR_NONE;
}


int TSUtil_CompileUnit (
                        char*                          invocation,
                        unsigned int                   flags,
                        struct tsutil_path_collection* search_paths,
                        char*                          unit_extension,
                        notify_lookup_function         notify_lookup,
                        struct tsdef_def_error_list*   def_errors,
                        struct tsdef_module*           module
                       )
{
    struct lookup_data            lookup_user_data;
    struct tsdef_argument_types   argument_types;
    struct tsdef_unit*            main_unit;
    struct tsdef_module_object*   existing_object;
    char*                         main_source;
    size_t                        main_source_length;
    int                           error;
    int                           return_error;

    existing_object = TSDef_LookupName("_module_main", module);
    if(existing_object != NULL)
        goto module_main_name_not_unique;

    return_error = TSUTIL_ERROR_MEMORY;

    main_unit = malloc(sizeof(struct tsdef_unit));
    if(main_unit == NULL)
        goto allocate_main_unit_failed;

    main_source_length = strlen(invocation);

    if(flags&TSUTIL_COMPILE_FLAG_CAPTURE_OUTPUT)
        main_source_length += sizeof("output forward_output = ");

    main_source_length += sizeof("\n")+1;

    main_source = malloc(main_source_length);
    if(main_source == NULL)
        goto allocate_main_source_failed;

    main_source[0] = 0;

    if(flags&TSUTIL_COMPILE_FLAG_CAPTURE_OUTPUT)
        strcat(main_source, "output forward_output = ");

    strcat(main_source, invocation);
    strcat(main_source, "\n");

    lookup_user_data.warning_count   = 0;
    lookup_user_data.unit_extension  = unit_extension;
    lookup_user_data.path_collection = search_paths;
    lookup_user_data.def_errors      = def_errors;
    lookup_user_data.notify_lookup   = notify_lookup;

    error = TSDef_ConstructUnitFromString(main_source, "_module_main", main_unit, def_errors);

    free(main_source);

    if(error != TSDEF_ERROR_NONE)
    {
        if(error != TSDEF_ERROR_CONSTRUCT_WARNING)
        {
            return_error = TSUTIL_ERROR_COMPILATION_ERROR;

            goto construct_main_unit_failed;
        }
        else
            lookup_user_data.warning_count++;
    }

    TSDef_SetModuleMain(main_unit, module);

    argument_types.count = 0;
    argument_types.types = NULL;

    error = TSDef_ResolveUnit(
                              main_unit,
                              &argument_types,
                              TSDEF_MODULE_OBJECT_FLAG_FREE_UNIT,
                              module,
                              &ModuleObjectLookup,
                              &lookup_user_data,
                              def_errors
                             );
    if(error != TSDEF_ERROR_NONE)
    {
        if(error != TSDEF_ERROR_RESOLVE_WARNING)
        {
            if(error == TSDEF_ERROR_RESOLVE_INITIALIZE_ERROR)
            {
                TSDef_DestroyUnit(main_unit);

                free(main_unit);
            }

            return TSUTIL_ERROR_COMPILATION_ERROR;
        }
        else
            lookup_user_data.warning_count++;
    }

    if(lookup_user_data.warning_count != 0)
        return TSUTIL_ERROR_COMPILATION_WARNING;

    return TSUTIL_ERROR_NONE;

construct_main_unit_failed:
    TSDef_DestroyUnit(main_unit);
allocate_main_source_failed:
    free(main_unit);

allocate_main_unit_failed:
    return return_error;

module_main_name_not_unique:
    return TSUTIL_ERROR_MODULE_MAIN_SYMBOL_NOT_UNIQUE;
}

