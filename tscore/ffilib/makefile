# Copyright 2011 Andrew Gottemoller.
#
# This software is a copyrighted work licensed under the terms of the
# Trigger Script license.  Please consult the file "TS_LICENSE" for
# details.

# This makefile is intended to build the tscore plugin, which provides
# core functionality to the trigger script language.  The makefile
# assumes a windows environment with the Microsoft Visual C++ compiler
# available.  Furthermore, it assumes make was launched from a visual
# studio command prompt (the msvc tools cl.exe and lib.exe need to be
# in the path).
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
name    = tscore.ts
config ?= release

# Specify the paths to build to
bin_path = ../../build/$(config)/bin
lib_path = ../../build/$(config)/lib
obj_path = ../../build/$(config)/obj/$(name)


# Set various compiler and linker options common to all build configurations
include_paths += ../../ts/api/tsffi/include  \
                 ../notify/include           \
                 ../math/include             \
                 ../time/include             \
                 ../graph/include            \
                 include

preprocessor_definitions += _CRT_SECURE_NO_WARNINGS _WINDOWS WIN32 _USRDLL _WINDLL

compiler_flags += /nologo /TC /W4 /wd4100 /wd4127 /wd4996

linker_flags += /nologo /SUBSYSTEM:WINDOWS /MACHINE:X86 /LARGEADDRESSAWARE /DEBUG /MAP /DLL

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
objects += groups \
           doc    \
           idhash \
           thread \
           module \
           main

# Specify the definition file
definition_file = source/ffilib.def

# Specify dependent static libraries
libs +=  notify \
         math   \
         time   \
         graph

# Specify standard win32 libs
win32_libs += user32 gdi32 shell32

# Compiled dependencies of the tscore dll.  These dependencies
# are automatically determined based on the libs/objects specified
compiled_deps += $(addsuffix .obj, $(addprefix $(obj_path)/, $(objects)))
compiled_deps += $(addsuffix .lib, $(addprefix $(lib_path)/, $(libs)))


.DEFAULT_GOAL = build

.PHONY: build
build: $(bin_path)/$(name).dll

# Command to link together compiled objs
$(bin_path)/$(name).dll : $(compiled_deps)
$(bin_path)/$(name).dll : $(definition_file)
$(bin_path)/$(name).dll : | $(bin_path)
	link $(linker_flags) /DEF:$(call swap_dir_sep,$(definition_file)) /OUT:$(call swap_dir_sep,$@) /IMPLIB:$(lib_path)/$(name).lib $(call swap_dir_sep,$(compiled_deps)) $(addsuffix .lib, $(win32_libs))

# Command to build an obj from a source file
$(obj_path)/%.obj : source/%.c | $(obj_path)
	cl $(compiler_flags) $(addprefix /I, $(call swap_dir_sep,$(include_paths))) $(addprefix /D, $(preprocessor_definitions)) /c /Fo$(call swap_dir_sep,$@) $(call swap_dir_sep,$<)

# Command to make any necessary directories
$(bin_path) $(obj_path) :
	mkdir $(call swap_dir_sep,$@)


.PHONY: clean
# Commands to undo the build
clean:
    ifneq ($(wildcard $(obj_path)),)
	    rmdir /s /q $(call swap_dir_sep,$(obj_path))
    endif
    ifneq ($(wildcard $(bin_path)/$(name).*),)
	    del /F /Q $(call swap_dir_sep,$(bin_path)/$(name).*)
    endif
    ifneq ($(wildcard $(lib_path)/$(name).*),)
	    del /F /Q $(call swap_dir_sep,$(lib_path)/$(name).*)
    endif

