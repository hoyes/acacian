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

# title: demo.mak
#
# Makefile for Demonstration Programs.
#
_r_acacian := ../..
_r_demo    := ..
_r_srcs    := ${_r_acacian}/csrc ${_r_acacian}/csrc .
_r_o       := build
_r_bin     := ${_r_demo}/bin
_r_expat   := ${wildcard ${_r_acacian}/expat-*/lib}
mapgen    := ${_r_acacian}/tools/bin/mapgen

DDL_PATH   := .:${HOME}/.acacian/ddlcache
export DDL_PATH

ifneq "${expat_buildin}" "yes"
libexpat := -lexpat
else
expat_objs := xmlparse.o xmltok.o xmlrole.o
expat_vpath := ${_r_expat}
endif

CFLAGS  := -O2
CFLAGS  += -std=c99
CFLAGS  += -Wall

CPPFLAGS :=
ifeq "${expat_buildin}" "yes"
CPPFLAGS += -I${_r_expat}
CPPFLAGS +=  -DHAVE_EXPAT_CONFIG_H=1
endif
CPPFLAGS += -I.
CPPFLAGS += -I${_r_acacian}/include
CPPFLAGS += -I${_r_demo}/utils
CPPFLAGS += -D_GNU_SOURCE=1
CPPFLAGS += -MMD

LDFLAGS :=
LDFLAGS += -lslp
LDFLAGS += ${libexpat}
LDFLAGS += -lrt

.SUFFIXES:

all: ${_r_bin}/${demo}

vpath %.c ${_r_acacian}/csrc ${expat_vpath} ${_r_demo}/utils ${_r_o}

vpath %.h . ${_r_acacian}/include

vpath %.ddl ${DDL_DEV}/draft ${DDL_DEV}/release

# Binary
${_r_bin}/${demo}: ${addprefix ${_r_o}/,${objs}}
ifeq "${wildcard ${_r_bin}}" ""
	mkdir -p ${_r_bin}
endif
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} $^

# Objects
${_r_o}/%.o : %.c
ifeq "${wildcard ${_r_o}}" ""
	mkdir -p ${_r_o}
endif
	${CC} -c -o $@ -D${demo}=1 ${CPPFLAGS} -I${_r_o} ${CFLAGS} $<

${_r_o}/%_map.c ${_r_o}/%_map.h: %.dev.ddl ${mapgen}
ifeq "${wildcard ${_r_o}}" ""
	mkdir -p ${_r_o}
endif
	${mapgen} -c ${_r_o}/$*_map.c -h ${_r_o}/$*_map.h $<

# Some targets for generating debug info
${_r_o}/macros :
	${CC} -E ${CPPFLAGS} -D${demo}=1 -dU ${acacian}/include/acn.h | sort | less

${_r_o}/%.i : %.c
	${CC} -E ${CPPFLAGS} -D${demo}=1 -I${_r_o} ${CFLAGS} $< > $@

${_r_o}/%.hi : %.h
	${CC} -E ${CPPFLAGS} -D${demo}=1 -I${_r_o} ${CFLAGS} $< > $@

config.mak : ${_r_o}/mkcfg.hi
	sed -ne '/@_\([^ ]\+\) \1/d;/@_/s/@_\(ACNCFG_\)\?\([^ ]\+\)  *\(.*\)/\2 := \3/p' $< > $@

${mapgen}:
	${MAKE} -C ${_r_acacian}/tools mapgen

.PHONY : clean

clean :
	rm -rf ${_r_o} config.mak ${_r_bin}/${demo}

include ${wildcard ${_r_o}/*.d}

# target used for testing Makefile variables
.PHONY: ts
ts :
	true ${wildcard ${expat_vpath}/*.c}
