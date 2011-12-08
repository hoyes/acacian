########################################################################
# 
# Copyright (c) 2011, Engineering Arts (UK)
#
# All rights reserved.
########################################################################

XSL_d     := xsl

ddl_src   := ../ddl-src

csrc/ddl/bvtab-%.c : ${XSL_d}/bvnames.xsl ${ddl_src}/%.bset.ddl
	xsltproc --nonet --stringparam name "$*" $^ > $@
