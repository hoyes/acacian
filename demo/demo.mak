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
_r_o       := build
_r_bin     := ${_r_demo}/bin
_r_expat   := ${wildcard ${_r_acacian}/expat-*/lib}
_r_tools   := ${_r_acacian}/tools
_r_utils   := ${_r_demo}/utils
mapgen     := ${_r_tools}/bin/mapgen
mapgen_src := ${wildcard ${_r_tools}/mapgen/*}

# set default target early
all: ${_r_bin}/${demo}

# load configuration options
include config.mak

# Set DDL_PATH if it isn't already
ifeq "${DDL_PATH}" ""
DDL_PATH   := ${HOME}/.acacian/ddlcache:.
export DDL_PATH
endif

# now setup compiler
# this would be a good place to put cross compile options
CFLAGS  += -O2
CFLAGS  += -std=c99
CFLAGS  += -Wall

CPPFLAGS += -I.
CPPFLAGS += -I${_r_acacian}/include
CPPFLAGS += -I${_r_demo}/utils
CPPFLAGS += -D_GNU_SOURCE=1
CPPFLAGS += -MMD

# process some config options
# if CF_DDL we will need expat in some form
ifeq "${CF_DDL}" "1"
ifeq "${expat_buildin}" "yes"
# expat gets built in
CPPFLAGS += -I${_r_expat}
CPPFLAGS += -DHAVE_EXPAT_CONFIG_H=1

objs     += xmlparse.o xmltok.o xmlrole.o
vpath xml%.c ${_r_expat}
else
# use external expat library
LDFLAGS += -lexpat
endif
endif

# if CF_EPI29 then SLP is needed
ifeq "${CF_EPI29}" "1"
LDFLAGS += -lslp
endif

.SUFFIXES:

vpath %.c ${_r_acacian}/csrc ${_r_utils} ${_r_o}

vpath %.h . ${_r_acacian}/include ${_r_utils}

vpath %.ddl ${DDL_DEV}/draft ${DDL_DEV}/release

# Binary
${_r_bin}/${demo}: ${addprefix ${_r_o}/,${objs}}
ifeq "${wildcard ${_r_bin}}" ""
	mkdir -p ${_r_bin}
endif
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} $^

# Objects
${_r_o}/%.o : %.c
	${CC} -c -o $@ -D${demo}=1 ${CPPFLAGS} -I${_r_o} ${CFLAGS} $<

${_r_o}/%_map.c ${_r_o}/%_map.h: %.dev.ddl ${mapgen}
	${mapgen} -c ${_r_o}/$*_map.c -h ${_r_o}/$*_map.h $<

# Some targets for generating debug info
${_r_o}/macros :
	${CC} -E ${CPPFLAGS} -D${demo}=1 -dU ${acacian}/include/acn.h | sort | less

${_r_o}/%.i : %.c
	${CC} -E ${CPPFLAGS} -MT $@ -D${demo}=1 -I${_r_o} ${CFLAGS} -o $@ $<

${_r_o}/%.hi : %.h
ifeq "${wildcard ${_r_o}}" ""
	mkdir -p ${_r_o}
endif
	${CC} -E ${CPPFLAGS} -MT $@ -D${demo}=1 -I${_r_o} ${CFLAGS} -o $@ $<

config.mak : ${_r_o}/mkcfg.hi
	sed -ne '/^@_/{s///;s/ / := /p;}' $< > $@

${mapgen}: ${mapgen_src}
	${MAKE} -C ${_r_tools}/mapgen all

.PHONY : clean

clean :
	rm -rf ${_r_o} config.mak ${_r_bin}/${demo}

include ${wildcard ${_r_o}/*.d}

# target used for testing Makefile variables
.PHONY: ts
ts :
	true ${wildcard ${expat_vpath}/*.c}
