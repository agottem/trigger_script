# Copyright 2011 Andrew Gottemoller.
#
# This software is a copyrighted work licensed under the terms of the
# Trigger Script license.  Please consult the file "TS_LICENSE" for
# details.

# This makefile is intended to build the tside executable, which provides
# a gui interface for the trigger script language.  The makefile assumes
# a windows environment with the Microsoft Visual C++ compiler available.
# Furthermore, it assumes make was launched from a visual studio command
# prompt (the msvc tools cl.exe and lib.exe need to be in the path).
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
name    = tside
config ?= release

# Specify the paths to build to
bin_path = ../../../../build/$(config)/bin
lib_path = ../../../../build/$(config)/lib
obj_path = ../../../../build/$(config)/obj/$(name)


# Set various compiler and linker options common to all build configurations
include_paths += ../../../api/tsdef/include    \
                 ../../../api/tsffi/include    \
                 ../../../api/tsint/include    \
                 ../../../api/tsutil/include   \
                 ../../../../scintilla/include

preprocessor_definitions += _CRT_SECURE_NO_WARNINGS _WINDOWS WIN32 WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0501 _UNICODE UNICODE

compiler_flags += /nologo /TC /W4 /wd4127 /wd4244 /wd4131 /wd4996 /wd4100 /wd4702 /wd4200 /wd4701

resource_compiler_flags += /n

linker_flags += /nologo /SUBSYSTEM:WINDOWS /MACHINE:X86 /LARGEADDRESSAWARE /DEBUG /MAP

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
objects += main          \
           paths         \
           plugins       \
           variables     \
           gui           \
           ide           \
           select        \
           editor        \
           search_text   \
           search_goto   \
           scintilla     \
           run           \
           alerts        \
           file          \
           reference     \
           ref_syntax    \
           ref_plugins   \
           ref_templates \
           text

# Specify resource objects
resource_objects = resources

# Specify the manifest file
manifest_file = resources/embedded/application_i386.manifest

# Specify dependent static libraries
libs += tsdef     \
        tsint     \
        tsutil    \
        scintilla

# Specify standard win32 libs
win32_libs += user32 gdi32 ole32 imm32 comctl32 comdlg32 shell32 uxtheme

# Syntax documentation to be installed in the bin path
ts_syntax = syntax/00._Overview.ts                 \
            syntax/01._Comments.ts                 \
            syntax/02._Variables.ts                \
            syntax/03._Mathematical_Expressions.ts \
            syntax/04._Comparison_Expressions.ts   \
            syntax/05._Logical_Expressions.ts      \
            syntax/06._If_Statements.ts            \
            syntax/07._Loops.ts                    \
            syntax/08._Actions.ts                  \
            syntax/09._Functions.ts                \
            syntax/10._Plugins.ts                  \
            syntax/11._Support.ts

# Templates to be installed in the bin path
ts_templates = templates/Graphing-Dual_Graphs.ts       \
               templates/Graphing-Single_Plot.ts       \
               templates/Introduction-Hello_World.ts   \
               templates/Messages-Types_of_Messages.ts \
               templates/Pipes-Command_Processor.ts    \
               templates/Pipes-Simple.ts


# Compiled dependencies of the tside executable.  These dependencies
# are automatically determined based on the libs/objects/resource_objects
# specified
compiled_deps += $(addsuffix .obj, $(addprefix $(obj_path)/, $(objects)))
compiled_deps += $(addsuffix .res, $(addprefix $(obj_path)/, $(resource_objects)))
compiled_deps += $(addsuffix .lib, $(addprefix $(lib_path)/, $(libs)))


.DEFAULT_GOAL = build

.PHONY: build
build: $(bin_path)/$(name).exe install_ts_syntax install_ts_templates

# Command to link together compiled objs
$(bin_path)/$(name).exe : $(compiled_deps)
$(bin_path)/$(name).exe : $(manifest_file)
$(bin_path)/$(name).exe : | $(bin_path)
	link $(linker_flags) /OUT:$(call swap_dir_sep,$@) $(call swap_dir_sep,$(compiled_deps)) $(addsuffix .lib,$(win32_libs))
	mt -nologo -manifest $(call swap_dir_sep,$(manifest_file)) -outputresource:$(call swap_dir_sep,$@);1

# Command to build an obj from a source file
$(obj_path)/%.obj : source/%.c | $(obj_path)
	cl $(compiler_flags) $(addprefix /I, $(call swap_dir_sep,$(include_paths))) $(addprefix /D, $(preprocessor_definitions)) /c /Fo$(call swap_dir_sep,$@) $(call swap_dir_sep,$<)

# Command to build a res from a resource file
$(obj_path)/%.res : source/%.rc | $(obj_path)
	rc /nologo /Fo$(call swap_dir_sep,$@) $(resource_compiler_flags) $<

.PHONY: install_ts_syntax
install_ts_syntax: $(addprefix $(bin_path)/, $(ts_syntax))

.PHONY: install_ts_templates
install_ts_templates: $(addprefix $(bin_path)/, $(ts_templates))

# Command to copy a syntax file
$(bin_path)/%.ts: resources/%.ts | $(bin_path)/syntax $(bin_path)/templates
	copy $(call swap_dir_sep,$<) $(call swap_dir_sep,$@)

# Command to make any necessary directories
$(bin_path) $(obj_path) $(bin_path)/syntax $(bin_path)/templates :
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
    ifneq ($(wildcard $(bin_path)/syntax),)
	    rmdir /s /q $(call swap_dir_sep,$(bin_path)/syntax)
    endif
    ifneq ($(wildcard $(bin_path)/templates),)
	    rmdir /s /q $(call swap_dir_sep,$(bin_path)/templates)
    endif

