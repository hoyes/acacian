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
#

_r_acacian := ..
_r_xsl     := ${_r_acacian}/xsl
_r_srcs    := ${_r_acacian}/csrc ${_r_acacian}/csrc/ddl .
_r_o       := build
_r_expat   := ${wildcard ${_r_acacian}/expat-*/lib}

DDL_PATH   := .:${DDL_CACHE}:${DDL_DEV}/draft:${DDL_DEV}/release
export DDL_PATH

CFLAGS  := -O2
CFLAGS  += -std=c99
CFLAGS  += -Wall

CPPFLAGS :=
CPPFLAGS += -I.
CPPFLAGS += -I${_r_acacian}/include
CPPFLAGS += -D_GNU_SOURCE=1
CPPFLAGS += -MMD

LDFLAGS :=
LDFLAGS += -lslp
# LDFLAGS += -lexpat
LDFLAGS += -lrt

.SUFFIXES:

progs := device_demo controller_demo ddl_tree mapgen tst

all:
	@echo No useful default rule

device dev : device_demo

controller strl : controller_demo

tree : ddl_tree

ddl_tree_objs := \
	behaviors.o \
	ddl_tree.o \
	dmpmap.o \
	keys.o \
	parse.o \
	printtree.o \
	random.o \
	resolve.o \
	uuid.o \
	xmlparse.o \
	xmltok.o \
	xmlrole.o \

mapgen_objs := \
	behaviors.o \
	dmpmap.o \
	keys.o \
	mapgen.o \
	parse.o \
	printtree.o \
	random.o \
	resolve.o \
	uuid.o \
	xmlparse.o \
	xmltok.o \
	xmlrole.o \

mapgen_includes := \

device_demo_objs := \
	component.o \
	demo_utils.o \
	device_demo.o \
	devicemap.o \
	discovery.o \
	dmp.o \
	dmpmap.o \
	evloop.o \
	getip.o \
	mcastalloc.o \
	random.o \
	rlp_bsd.o \
	sdt.o \
	uuid.o \

controller_demo_objs := \
	behaviors.o \
	component.o \
	controller_demo.o \
	demo_utils.o \
	discovery.o \
	dmp.o \
	dmpmap.o \
	evloop.o \
	getip.o \
	keys.o \
	mcastalloc.o \
	parse.o \
	printtree.o \
	random.o \
	resolve.o \
	rlp_bsd.o \
	sdt.o \
	uuid.o \
	xmlparse.o \
	xmltok.o \
	xmlrole.o \

tst_objs := \
	tst.o

ddl_modules := \
	c7efa24c-dd82-11d9-881e-00e018a44101

vpath %.c ${_r_acacian}/csrc ${_r_acacian}/csrc/ddl ${_r_o}/device_demo

vpath %.ddl ${DDL_DEV}/draft ${DDL_DEV}/release

.PRECIOUS : ${_r_o}/device_demo/devicemap.c ${_r_o}/device_demo/devicemap.h

${_r_o}/device_demo/devicemap.c ${_r_o}/device_demo/devicemap.h : demo.dev.ddl mapgen ${patsubst %,${DDL_CACHE}/%.ddl,${ddl_modules}}
	./mapgen -c ${_r_o}/device_demo/devicemap.c -h ${_r_o}/device_demo/devicemap.h ${addprefix -i,${mapgen_includes}} $<

define srcrule

${_r_o}/${prog}/bvset_%.c : ${_r_xsl}/bvnames.xsl %.bset.ddl
	xsltproc --nonet --stringparam name "$$*" $$^ > $$@

${_r_o}/${prog}/bvset_%.o : ${_r_o}/${prog}/bvset_%.c
ifeq "${wildcard ${_r_o}/${prog}}" ""
	mkdir -p ${_r_o}/${prog}
endif
	$${CC} -c -o $$@ -D${prog}=1 ${CPPFLAGS} ${CFLAGS} $$<

${_r_o}/${prog}/%.o : %.c
ifeq "${wildcard ${_r_o}/${prog}}" ""
	mkdir -p ${_r_o}/${prog}
endif
	$${CC} -c -o $$@ -D${prog}=1 ${CPPFLAGS} -I${_r_o}/${prog} ${CFLAGS} $$<

${_r_o}/${prog}/xml%.o : ${_r_expat}/xml%.c
	$${CC} -c -o $$@ -D${prog}=1 ${CPPFLAGS} -DHAVE_EXPAT_CONFIG_H=1 -I${_r_o}/${prog} -I${_r_expat} ${CFLAGS} $$<

${_r_o}/${prog}/macros :
	$${CC} -E ${CPPFLAGS} -D${prog}=1 -dM ../include/acn.h | sort | less

${_r_o}/${prog}/%.i : %.c
	$${CC} -E ${CPPFLAGS} -D${prog}=1 -I${_r_o}/${prog} ${CFLAGS} $$< > $$@

${_r_o}/${prog}/%.hi : ${_r_acacian}/include/%.h
	$${CC} -E ${CPPFLAGS} -D${prog}=1 -I${_r_o}/${prog} ${CFLAGS} $$< > $$@

${prog} : $${addprefix ${_r_o}/${prog}/,$${${prog}_objs}}
ifeq "${wildcard ${_r_o}/${prog}}" ""
	mkdir -p ${_r_o}/${prog}
endif
	@echo "Need $$^"
	${CC} -o $$@ ${CFLAGS} ${LDFLAGS} $$^

.PHONY : ${prog}_clean

${prog}_clean :
	rm -rf ${_r_o}/${prog}/*
	

# dependencies
# ifneq "${wildcard ${_r_o}/${prog}/*.d}" ""
# include ${_r_o}/${prog}/*.d
# endif
include ${wildcard ${_r_o}/${prog}/*.d}

endef

${foreach prog,${progs},${eval ${srcrule}}}

.PHONY : clean
clean : ${addsuffix _clean,${progs}}

.PHONY: ts
ts :
	true ${_r_expat}
