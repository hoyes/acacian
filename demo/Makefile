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
# 
# about: Acacian
# 
# Acacian is a full featured implementation of ANSI E1.17 2012
# Architecture for Control Networks (ACN) from Acuity Brands
# 
# file: Makefile
# 
# Makefile for demo programs.
########################################################################

_r_acacian := ..
_r_xsl     := ${_r_acacian}/xsl
_r_srcs    := ${_r_acacian}/csrc ${_r_acacian}/csrc/ddl .
_r_o       := build
_r_html    := ${_r_build}/doc/html
_r_ddlsrc  := ${_r_acacian}/ddl-src
_r_ddlmod  := ${_r_acacian}/ddl-modules

DDL_PATH   := ${abspath .}:${abspath ${_r_ddlmod}}
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
LDFLAGS += -lexpat
LDFLAGS += -lrt

progs := device_demo controller_demo ddl_tree mapgen tst

all:
	@echo No useful default rule

device dev : device_demo

controller strl : controller_demo

tree : ddl_tree

ddl_tree_objs := \
	ddl_tree.o \
	parse.o \
	uuid.o \
	dmpmap.o \
	behaviors.o \
	bvactions.o \
	printtree.o \
	resolve.o \
	bvset_acnbase.o \
	bvset_acnbase-r2.o \
	bvset_acnbaseExt1.o \
	bvset_sl.o \
	bvset_artnet.o \

mapgen_objs := \
	mapgen.o \
	parse.o \
	uuid.o \
	dmpmap.o \
	behaviors.o \
	bvactions.o \
	printtree.o \
	resolve.o \
	bvset_acnbase.o \
	bvset_acnbase-r2.o \
	bvset_acnbaseExt1.o \
	bvset_sl.o \
	bvset_artnet.o \

mapgen_includes := \

device_demo_objs := \
	device_demo.o \
	demo_utils.o \
	discovery.o \
	getip.o \
	uuid.o \
	mcastalloc.o \
	dmpmap.o \
	dmp.o \
	sdt.o \
	random.o \
	rlp.o \
	netxface.o \
	component.o \
	evloop.o \
	devicemap.o

controller_demo_objs := \
	controller_demo.o \
	demo_utils.o \
	discovery.o \
	getip.o \
	uuid.o \
	mcastalloc.o \
	dmpmap.o \
	dmp.o \
	sdt.o \
	random.o \
	rlp.o \
	netxface.o \
	behaviors.o \
	bvactions.o \
	resolve.o \
	parse.o \
	printtree.o \
	component.o \
	evloop.o \
	bvset_acnbase.o \
	bvset_acnbase-r2.o \
	bvset_acnbaseExt1.o \
	bvset_sl.o \
	bvset_artnet.o \

tst_objs := \
	tst.o

ddl_modules := \
	c7efa24c-dd82-11d9-881e-00e018a44101

VPATH := . ${_r_acacian}/csrc ${_r_acacian}/csrc/ddl ${_r_o}/device_demo

.PRECIOUS : ${_r_o}/device_demo/devicemap.c ${_r_o}/device_demo/devicemap.h

${_r_o}/device_demo/devicemap.c ${_r_o}/device_demo/devicemap.h : demo.dev.ddl mapgen ${addprefix ${_r_ddlmod}/,${ddl_modules}}
	./mapgen -c ${_r_o}/device_demo/devicemap.c -h ${_r_o}/device_demo/devicemap.h ${addprefix -i,${mapgen_includes}} $<

define srcrule
${_r_o}/${prog}/bvset_%.c : ${_r_xsl}/bvnames.xsl ${_r_ddlsrc}/%.bset.ddl
	xsltproc --nonet --stringparam name "$$*" $$^ > $$@

vpath bvset_%.c ${_r_o}/${prog}
#${_r_o}/${prog}/bvset_%.o : ${_r_o}/bvset_%.c
#ifeq "${wildcard ${_r_o}/${prog}}" ""
#	mkdir -p ${_r_o}/${prog}
#endif
#	$${CC} -c -o $$@ ${CPPFLAGS} -D${prog}=1 ${CFLAGS} $$<

${_r_o}/${prog}/%.o : %.c
ifeq "${wildcard ${_r_o}/${prog}}" ""
	mkdir -p ${_r_o}/${prog}
endif
	$${CC} -c -o $$@ -D${prog}=1 ${CPPFLAGS} -I${_r_o}/${prog} ${CFLAGS} $$<

${_r_o}/${prog}/macros :
	$${CC} -E ${CPPFLAGS} -D${prog}=1 -dM ../include/acn.h | sort | less

${_r_o}/${prog}/%.i : %.c
	$${CC} -E ${CPPFLAGS} -D${prog}=1 ${CFLAGS} $$< > $$@

${_r_o}/${prog}/%.hi : ${_r_acacian}/include/%.h
	$${CC} -E ${CPPFLAGS} -D${prog}=1 ${CFLAGS} $$< > $$@

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
	true ${_r_o}

.PHONY: doc

doc :
ifeq "" "${wildcard ${_r_html}}"
	mkdir -p ${_r_html}
endif
	NaturalDocs -i include -i csrc -o html ${_r_html} -p doc/nd -s Default local

#	mkdoc --doc_path doc/mkdoc.d --output_path build-doc
