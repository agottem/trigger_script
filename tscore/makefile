# Copyright 2011 Andrew Gottemoller.
#
# This software is a copyrighted work licensed under the terms of the
# Trigger Script license.  Please consult the file "TS_LICENSE" for
# details.

# This makefile just directs the build to sub-directories

.DEFAULT_GOAL = build

.PHONY: build
build:
	@$(MAKE) -C notify build
	@$(MAKE) -C math build
	@$(MAKE) -C time build
	@$(MAKE) -C graph build
	@$(MAKE) -C ffilib build


.PHONY: clean
clean:
	@$(MAKE) -C notify clean
	@$(MAKE) -C math clean
	@$(MAKE) -C time clean
	@$(MAKE) -C graph clean
	@$(MAKE) -C ffilib clean

