########################################################################
# 
# Copyright (c) 2011, Engineering Arts (UK)
#
# All rights reserved.
########################################################################

XSL_d     := xsl
s_dirs       := csrc csrc/ddl

ifneq "${ACNBUILD_OUTPUT}" ""
o_d     := ${ACNBUILD_OUTPUT}
endif

CFLAGS  := -O2

CPPFLAGS :=
CPPFLAGS += -Iinclude

ddl_src   := ../ddl-src

csrc/ddl/bvtab-%.c : ${XSL_d}/bvnames.xsl ${ddl_src}/%.bset.ddl
	xsltproc --nonet --stringparam name "$*" $^ > $@

define srcrule
${o_d}/%.o : ${s_d}/%.c
	$${CC} -c -o $$@ ${CPPFLAGS} ${CFLAGS} $$<

endef

${foreach s_d,${s_dirs},${eval ${srcrule}}}

ts :
	true ${o_d}
