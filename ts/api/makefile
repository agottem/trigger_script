# Copyright 2011 Andrew Gottemoller.
#
# This software is a copyrighted work licensed under the terms of the
# Trigger Script license.  Please consult the file "TS_LICENSE" for
# details.

# This makefile just directs the build to sub-directories

.DEFAULT_GOAL = build

.PHONY: build
build:
	@$(MAKE) -C tsdef build
	@$(MAKE) -C tsint build
	@$(MAKE) -C tsutil build


.PHONY: clean
clean:
	@$(MAKE) -C tsdef clean
	@$(MAKE) -C tsint clean
	@$(MAKE) -C tsutil clean

