# Copyright 2011 Andrew Gottemoller.
#
# This software is a copyrighted work licensed under the terms of the
# Trigger Script license.  Please consult the file "TS_LICENSE" for
# details.

# This makefile is intended to build the tsi executable, which provides
# a commane line interface for the trigger script language.  The
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
name    = tsi
config ?= release

# Specify the paths to build to
bin_path = ../../../build/$(config)/bin
lib_path = ../../../build/$(config)/lib
obj_path = ../../../build/$(config)/obj/$(name)


# Set various compiler and linker options common to all build configurations
include_paths += ../../api/tsdef/include  \
                 ../../api/tsffi/include  \
                 ../../api/tsint/include  \
                 ../../api/tsutil/include

preprocessor_definitions += PLATFORM_WIN32 _CRT_SECURE_NO_WARNINGS _CONSOLE WIN32

compiler_flags += /nologo /TC /W4 /wd4127 /wd4244 /wd4131 /wd4996 /wd4100 /wd4702 /wd4200

linker_flags += /nologo /SUBSYSTEM:CONSOLE /MACHINE:X86 /LARGEADDRESSAWARE /DEBUG /MAP

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
objects += main       \
           context    \
           controller \
           execif     \
           error

# Platform specific objects
objects += register_win32

# Specify dependent static libraries
libs += tsdef  \
        tsint  \
        tsutil


.DEFAULT_GOAL = build

.PHONY: build
build: $(bin_path)/$(name).exe

# Command to link together compiled objs
$(bin_path)/$(name).exe : $(addsuffix .obj, $(addprefix $(obj_path)/, $(objects)))
$(bin_path)/$(name).exe : $(addsuffix .lib, $(addprefix $(lib_path)/, $(libs)))
$(bin_path)/$(name).exe : | $(bin_path)
	link $(linker_flags) /OUT:$(call swap_dir_sep,$@) $(call swap_dir_sep,$^)

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

