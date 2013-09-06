########################################################################
# 
# Copyright (c) 2013, Philip Nye
#
# All rights reserved.
#
#tabs=8t
########################################################################

_r_acn     := ..
_r_xsl     := ${_r_acn}/xsl
_r_srcs    := ${_r_acn}/csrc ${_r_acn}/csrc/ddl .
_r_o       := build
_r_html    := ${_r_build}/doc/html
_r_ddlsrc  := ${_r_acn}/ddl-src
_r_ddlmod  := ${_r_acn}/ddl-modules

DDL_PATH   := ${abspath .}:${abspath ${_r_ddlmod}}
export DDL_PATH

CFLAGS  := -O2
CFLAGS  += -std=c99

CPPFLAGS :=
CPPFLAGS += -I.
CPPFLAGS += -I${_r_acn}/include
CPPFLAGS += -D_GNU_SOURCE=1
CPPFLAGS += -MMD

LDFLAGS :=
LDFLAGS += -lslp
LDFLAGS += -lexpat

progs := device_demo controller_demo ddl_tree mapgen tst

all:
	@echo No useful default rule

${_r_o}/bvset_%.c : ${_r_xsl}/bvnames.xsl ${_r_ddlsrc}/%.bset.ddl
	xsltproc --nonet --stringparam name "$*" $^ > $@

ddl_tree_objs := \
	ddl_tree.o \
	parse.o \
	uuid.o \
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
	behaviors.o \
	bvactions.o \
	printtree.o \
	resolve.o \
	bvset_acnbase.o \
	bvset_acnbase-r2.o \
	bvset_acnbaseExt1.o \
	bvset_sl.o \
	bvset_artnet.o \

device_demo_objs := \
	demo_dev.o \
	demo_utils.o \
	getip.o \
	uuid.o \
	mcastalloc.o \
	dmpmap.o \
	component.o

controller_demo_objs := \
	demo_dev.o \
	demo_utils.o \
	getip.o \
	uuid.o \
	mcastalloc.o \
	component.o

tst_objs := \
	tst.o

vpath %.c . ${_r_acn}/csrc ${_r_acn}/csrc/ddl ${_r_o}

${_r_o}/devicemap.c ${_r_o}/devicemap.h : demo.dev.ddl mapgen
	./mapgen -c ${_r_o}/devicemap.c -h ${_r_o}/devicemap.h $<

define srcrule
${_r_o}/${prog}/bvset_%.o : ${_r_o}/bvset_%.c
ifeq "${wildcard ${_r_o}/${prog}}" ""
	mkdir -p ${_r_o}/${prog}
endif
	$${CC} -c -o $$@ ${CPPFLAGS} -D${prog}=1 ${CFLAGS} $$<

${_r_o}/${prog}/%.o : %.c
ifeq "${wildcard ${_r_o}/${prog}}" ""
	mkdir -p ${_r_o}/${prog}
endif
	$${CC} -c -o $$@ ${CPPFLAGS} -D${prog}=1 ${CFLAGS} $$<

${_r_o}/${prog}/%.i : %.c
	$${CC} -E ${CPPFLAGS} -D${prog}=1 ${CFLAGS} $$< > $$@

${prog} : $${addprefix ${_r_o}/${prog}/,$${${prog}_objs}}
ifeq "${wildcard ${_r_o}/${prog}}" ""
	mkdir -p ${_r_o}/${prog}
endif
	@echo "Need $$^"
	${CC} -o $$@ ${CFLAGS} ${LDFLAGS} $$^

# dependencies
# ifneq "${wildcard ${_r_o}/${prog}/*.d}" ""
# include ${_r_o}/${prog}/*.d
# endif
include ${wildcard ${_r_o}/${prog}/*.d}

endef

${foreach prog,${progs},${eval ${srcrule}}}

ts :
	true ${_r_o}

.PHONY: doc

doc :
ifeq "" "${wildcard ${_r_html}}"
	mkdir -p ${_r_html}
endif
	NaturalDocs -i include -i csrc -o html ${_r_html} -p doc/nd -s Default local

#	mkdoc --doc_path doc/mkdoc.d --output_path build-doc
