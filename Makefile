########################################################################
# 
# Copyright (c) 2011, Engineering Arts (UK)
#
# All rights reserved.
########################################################################

_r_xsl     := xsl
_r_srcs    := csrc csrc/ddl
_r_html    := build/doc/html


ifneq "${ACNBUILD_OUTPUT}" ""
o_d     := ${ACNBUILD_OUTPUT}
endif

CFLAGS  := -O2

CPPFLAGS :=
CPPFLAGS += -Iinclude

ddl_src   := ../ddl-src

csrc/ddl/bvtab-%.c : ${_r_xsl}/bvnames.xsl ${ddl_src}/%.bset.ddl
	xsltproc --nonet --stringparam name "$*" $^ > $@

define srcrule
${o_d}/%.o : ${s_d}/%.c
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

ddldemo : ${addprefix ${o_d}/,${ddldemo_objs}}
	$CC -o $@ ${CFLAGS} ${LDFLAGS} $^

ts :
	true ${o_d}

.PHONY: doc

doc :
ifeq "" "${wildcard ${_r_html}}"
	mkdir -p ${_r_html}
endif
	NaturalDocs -i include -i csrc -o html ${_r_html} -p doc/nd -s Default local

#	mkdoc --doc_path doc/mkdoc.d --output_path build-doc
