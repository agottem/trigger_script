# Copyright 2011 Andrew Gottemoller.
#
# This software is a copyrighted work licensed under the terms of the
# Trigger Script license.  Please consult the file "TS_LICENSE" for
# details.

# This makefile is intended to build the timer static lib, which provides
# time related functions to the ts language via the tscore plugin.  The
# makefile assumes a windows environment with the Microsoft Visual C++
# compiler available.  Furthermore, it assumes make was launched from a
# visual studio command prompt (the msvc tools cl.exe and lib.exe need to
# be in the path).
#
# Valid targets for this makefile are:
#     build
#     clean
#
# Optionally, the config variable may be passed to make.  By default,
# config=release.  Setting config=debug will produce a debug build.
#
# WARNING: MSVC does not provide an elegant way to dump header dependencies
#          for a compiled source file.  As such, make may not rebuild
#          an obj file when a header dependency changes.

# Force the shell to the standard Windows command prompt
SHELL = cmd.exe

# Define a function to switch unix-style '/' directory separators to '\'
swap_dir_sep = $(subst /,\,$(1))


# Set the name of the static lib, as well as the default config value
name    = time
config ?= release

# Specify the paths to build to
lib_path = ../../build/$(config)/lib
obj_path = ../../build/$(config)/obj/$(name)


# Set various compiler and linker options common to all build configurations
include_paths += ../../ts/api/tsffi/include \
                 ../ffilib/include          \
                 include

preprocessor_definitions += _CRT_SECURE_NO_WARNINGS _WINDOWS WIN32 _LIB

compiler_flags += /nologo /TC /W4 /wd4100

linker_flags += /nologo /SUBSYSTEM:WINDOWS /MACHINE:X86

# Set compiler and linker options specific to debug / release configurations
ifeq ("$(config)", "debug")
    compiler_flags           += /MTd /Z7 /Od
    preprocessor_definitions += _DEBUG
else
    compiler_flags           += /MT /Z7 /O2 /GL
    linker_flags             += /LTCG
    preprocessor_definitions += NDEBUG
endif


# Build objects have a 1 to 1 mapping with source c files.  Append to this
# list to specify new c files to be built.
objects += time  \
           timer


.DEFAULT_GOAL = build

.PHONY: build
build: $(lib_path)/$(name).lib

# Command to link together compiled objs
$(lib_path)/$(name).lib : $(addsuffix .obj, $(addprefix $(obj_path)/, $(objects))) | $(lib_path)
	lib $(linker_flags) /OUT:$(call swap_dir_sep,$@) $(call swap_dir_sep,$^)

# Command to build an obj from a source file
$(obj_path)/%.obj : source/%.c | $(obj_path)
	cl $(compiler_flags) $(addprefix /I, $(call swap_dir_sep,$(include_paths))) $(addprefix /D, $(preprocessor_definitions)) /c /Fo$(call swap_dir_sep,$@) $(call swap_dir_sep,$<)

# Command to make any necessary directories
$(lib_path) $(obj_path) :
	mkdir $(call swap_dir_sep,$@)


.PHONY: clean
# Commands to undo the build
clean:
    ifneq ($(wildcard $(obj_path)),)
	    rmdir /s /q $(call swap_dir_sep,$(obj_path))
    endif
    ifneq ($(wildcard $(lib_path)/$(name).*),)
	    del /F /Q $(call swap_dir_sep,$(lib_path)/$(name).*)
    endif

