#! /usr/bin/make

########################################################################
# 
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# Copyright (c) 2013, Acuity Brands, Inc.
# 
# Author: Philip Nye <philip.nye@engarts.com>
# 
#tabs=8
########################################################################

# title: Demo Makefile
#
# Makefile for Demonstration Programs.

demos := controller device ddltree

.PHONY: all clean
all clean:
	for d in ${demos} ; do ${MAKE} -C $$d $@ ; done

.PHONY: ${demos}
${demos}:
	${MAKE} -C $@

${addsuffix _clean,${demos}}:
	${MAKE} -C ${subst _clean,,$@} clean
