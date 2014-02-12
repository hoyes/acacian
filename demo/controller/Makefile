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
expat_buildin := no

demo := controller

objs = \
	controller.o \
	behaviors.o \
	component.o \
	demo_utils.o \
	discovery.o \
	dmp.o \
	dmpmap.o \
	evloop.o \
	getip.o \
	keys.o \
	mcastalloc.o \
	ddlparse.o \
	printtree.o \
	random.o \
	ddlresolve.o \
	rlp_bsd.o \
	sdt.o \
	uuid.o

include ../demo.mak
