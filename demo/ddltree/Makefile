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
# Makefile for DDL tree demo.
#
# Building Expat in:
# If expat_buildin is "yes" in the next line expat will be built in
# to Acacian, otherwise it is expected to be available as a library in 
# the system.
expat_buildin := no

demo := ddltree

objs = \
	ddltree.o \
	behaviors.o \
	dmpmap.o \
	keys.o \
	ddlparse.o \
	printtree.o \
	random.o \
	ddlresolve.o \
	uuid.o

include ../demo.mak
