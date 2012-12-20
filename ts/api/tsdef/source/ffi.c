/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <tsdef/ffi.h>
#include <tsdef/def.h>
#include <tsffi/primitives.h>


unsigned int TSDef_TranslateFFIType (unsigned int ffi_type)
{
    switch(ffi_type)
    {
    case TSFFI_PRIMITIVE_TYPE_VOID:
        return TSDEF_PRIMITIVE_TYPE_VOID;

    case TSFFI_PRIMITIVE_TYPE_BOOL:
        return TSDEF_PRIMITIVE_TYPE_BOOL;

    case TSFFI_PRIMITIVE_TYPE_INT:
        return TSDEF_PRIMITIVE_TYPE_INT;

    case TSFFI_PRIMITIVE_TYPE_REAL:
        return TSDEF_PRIMITIVE_TYPE_REAL;

    case TSFFI_PRIMITIVE_TYPE_STRING:
        return TSDEF_PRIMITIVE_TYPE_STRING;
    }

    return TSDEF_PRIMITIVE_TYPE_VOID;
}

