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
#
# Makefile for Acacian Documentation
#
# about: Acacian
# 
# Acacian is a full featured implementation of ANSI E1.17 2012
# Architecture for Control Networks (ACN) from Acuity Brands
# 

_r_         := .
_r_acacian  := ..
_r_html     := doc.html
_r_images   := ${_r_html}/pics
_r_imagesrc := images
_r_ddl      := ../../ddl
ddl_r_acacian := ../acacian

docdirs := \
	. \
	${_r_acacian}/csrc \
	${_r_acacian}/include \
	${_r_acacian}/demo \
	${_r_acacian}/tools/mapgen

nodocdirs := \
	${shell find ${_r_acacian} -name build -type d}

pics := Acuity-hpc.png

images := ${addprefix ${_r_images}/,${pics}}

confsrc := ${notdir ${wildcard conf/*}}

.PHONY : all docs images

all docs : ${addprefix nd/,${confsrc}}
ifeq "" "${wildcard ${_r_html}}"
	mkdir -p ${_r_html}
endif
	NaturalDocs -p nd -s local -o FramedHTML ${_r_html} \
	${addprefix -i ,${docdirs}} \
	${addprefix -xi ,${nodocdirs}}

images : ${images}

${_r_images}/%.png : ${_r_imagesrc}/%.png
ifeq "${wildcard ${_r_images}}" ""
	mkdir -p ${_r_images}
endif
	cp $< $@

nd/%: conf/%
	cp $< $@

${_r_html}/demo.dev.html:
	${MAKE} -C ${_r_ddl} SPATH=${ddl_r_acacian}/demo/device \
		dHTML=${ddl_r_acacian}/doc/${_r_html} \
		${ddl_r_acacian}/doc/$@

clean :
	rm -rf ${_r_html}/* nd/Data
