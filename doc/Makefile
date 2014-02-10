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

# title: Documentation Makefile
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

now := ${shell date +"%Y%m%dt%H%M%S"}

exclude := .DS_Store .Trashes ._.*

tar_exclude := ${patsubst %,--exclude "%",${exclude}}

subdirs := \
	. \
	${_r_acacian}/csrc \
	${_r_acacian}/include \
	${_r_acacian}/demo \

pics := Acuity-hpc.png

images := ${addprefix ${_r_images}/,${pics}}

.PHONY : all docs images

all docs :
ifeq "" "${wildcard ${_r_html}}"
	mkdir -p ${_r_html}
endif
	NaturalDocs  -s local ${addprefix -i ,${subdirs}} ${addprefix -xi ,${nd_excldirs}} -o FramedHTML ${_r_html} -p nd

images : ${images}

${_r_images}/%.png : ${_r_imagesrc}/%.png
ifeq "${wildcard ${_r_images}}" ""
	mkdir -p ${_r_images}
endif
	cp $< $@

clean :
	rm -rf ${_r_html} nd/Data
