########################################################################
# 
# Copyright (c) 2011, Engineering Arts (UK)
#
# All rights reserved.
########################################################################

_r_xsl     := xsl
_r_srcs    := csrc csrc/ddl demo
_r_html    := build/doc/html


ifneq "${ACNBUILD_OUTPUT}" ""
_r_o     := ${ACNBUILD_OUTPUT}
else
_r_o     := build
ifeq "" "${wildcard ${_r_o}}"
${shell mkdir -p ${_r_o}}
endif
endif

CFLAGS  := -O2
CFLAGS  += -std=c99

CPPFLAGS :=
CPPFLAGS += -Iinclude
# CPPFLAGS += -D_POSIX_C_SOURCE=200809L
# CPPFLAGS += -D_XOPEN_SOURCE=600
# CPPFLAGS += -D_BSD_SOURCE=1
CPPFLAGS += -D_GNU_SOURCE=1

CPPFLAGS += -MMD

ddl_src   := ../ddl-src

csrc/ddl/bvtab-%.c : ${_r_xsl}/bvnames.xsl ${ddl_src}/%.bset.ddl
	xsltproc --nonet --stringparam name "$*" $^ > $@

define srcrule
${_r_o}/%.o : ${s_d}/%.c
	$${CC} -c -o $$@ ${CPPFLAGS} ${CFLAGS} $$<

endef

${foreach s_d,${_r_srcs},${eval ${srcrule}}}

ddldemo_objs := \
	demo.o \
	parse.o \
	uuid.o \
	behaviors.o \
	printtree.o \
	keys.o \
	resolve.o \
	bvset_acnbase.o \
	bvset_acnbase_r2.o \
	bvset_acnbaseExt1.o \
	bvset_sl.o \
	bvset_artnet.o \

ddldemo : ${addprefix ${_r_o}/,${ddldemo_objs}}
	$CC -o $@ ${CFLAGS} ${LDFLAGS} $^

tst_objs := \
	tst.o \
	getip.o

tst : ${addprefix ${_r_o}/,${tst_objs}}
	${CC} -o $@ ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} $^

ts :
	true ${_r_o}

# dependencies
include ${_r_o}/*.d

.PHONY: doc

doc :
ifeq "" "${wildcard ${_r_html}}"
	mkdir -p ${_r_html}
endif
	NaturalDocs -i include -i csrc -o html ${_r_html} -p doc/nd -s Default local

#	mkdoc --doc_path doc/mkdoc.d --output_path build-doc
