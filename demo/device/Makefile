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
# Makefile for Device Demo.
#

demo := device

objs = \
	demo_map.o \
	device.o \
	component.o \
	demo_utils.o \
	discovery.o \
	dmp.o \
	dmpmap.o \
	evloop.o \
	getip.o \
	mcastalloc.o \
	random.o \
	rlp_bsd.o \
	sdt.o \
	uuid.o \

include ../demo.mak

${_r_o}/demo_map.o: ${_r_o}/demo_map.c ${_r_o}/demo_map.h
