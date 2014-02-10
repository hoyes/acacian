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

# title: Controller Demo Makefile
#
# Makefile for Demonstration Programs.
#
# Building Expat in:
# If expat_buildin is "yes" in the next line expat will be built in
# to Acacian, otherwise it is expected to be available as a library in 
# the system.
expat_buildin := yes

objs = \
	behaviors.o \
	dmpmap.o \
	keys.o \
	mapgen.o \
	ddlparse.o \
	printtree.o \
	random.o \
	ddlresolve.o \
	uuid.o \
	${expat_objs}

all: mapgen

_r_acacian := ../..
_r_tools   := ..
_r_o       := build
_r_bin     := ${_r_tools}/bin
_r_expat   := ${wildcard ${_r_acacian}/expat-*/lib}

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
# CPPFLAGS += -I${_r_demo}/utils
CPPFLAGS += -D_GNU_SOURCE=1
CPPFLAGS += -MMD

LDFLAGS :=
# LDFLAGS += -lslp
LDFLAGS += ${libexpat}
LDFLAGS += -lrt

.SUFFIXES:

mapgen: ${_r_bin}/mapgen

vpath %.c ${_r_acacian}/csrc ${expat_vpath}

# Binary
${_r_bin}/mapgen: ${addprefix ${_r_o}/,${objs}}
ifeq "${wildcard ${_r_bin}}" ""
	mkdir -p ${_r_bin}
endif
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} $^

# Objects
${_r_o}/%.o : %.c
ifeq "${wildcard ${_r_o}}" ""
	mkdir -p ${_r_o}
endif
	${CC} -c -o $@ -Dmapgen=1 ${CPPFLAGS} -I${_r_o} ${CFLAGS} $<

.PHONY : clean

clean :
	rm -rf ${_r_o} ${_r_bin}/mapgen

include ${wildcard ${_r_o}/*.d}
