<?xml version="1.0" encoding="US-ASCII"?>
<!--
########################################################################

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=3
########################################################################

about: Acacian

Acacian is a full featured implementation of ANSI E1.17 2012
Architecture for Control Networks (ACN) from Acuity Brands

file: bvnames.xsl

Generate c source from a DDL behaviorset
########################################################################
-->
<!DOCTYPE stylesheet [
<!ENTITY tb "&#x9;">
<!ENTITY nl "&#xa;">
]>
<!-- vi: set sw=3 ts=3: -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:ea="http://www.engarts.com/namespace/2011/ddlx"
	xmlns:ddl="http://www.esta.org/acn/namespace/ddl/2008/"
	xmlns:str="http://exslt.org/strings"
>
<!--
-->
	<xsl:output method="text" encoding="US-ASCII"/>

	<xsl:param name="name" select="''"/>
	<xsl:variable name="space" select="'                                         '"/>

<!--
########################################################################
	Root node
########################################################################
-->
<xsl:template match="/">
	<xsl:apply-templates select="//behaviorset"/>
</xsl:template>

<!--
########################################################################
	Behaviorset
########################################################################
-->
<xsl:template match="behaviorset">
	<xsl:variable name="uuidhex" select="translate(@UUID, '-', '')"/>
	<xsl:variable name="uuidhexs" select="concat(
		'\x', substring($uuidhex,  1, 2),
		'\x', substring($uuidhex,  3, 2),
		'\x', substring($uuidhex,  5, 2),
		'\x', substring($uuidhex,  7, 2),
		'\x', substring($uuidhex,  9, 2),
		'\x', substring($uuidhex, 11, 2),
		'\x', substring($uuidhex, 13, 2),
		'\x', substring($uuidhex, 15, 2),
		'\x', substring($uuidhex, 17, 2),
		'\x', substring($uuidhex, 19, 2),
		'\x', substring($uuidhex, 21, 2),
		'\x', substring($uuidhex, 23, 2),
		'\x', substring($uuidhex, 25, 2),
		'\x', substring($uuidhex, 27, 2),
		'\x', substring($uuidhex, 29, 2),
		'\x', substring($uuidhex, 31, 2)
	)"/>
	<xsl:variable name="numdefs" select="count(behaviordef)"/>
	<xsl:variable name="maxstr">
		<xsl:for-each select="behaviordef/@name">
			<xsl:sort select="string-length(.)" order="descending" data-type="number"/>
			<xsl:if test="position() = 1">
				<xsl:value-of select="string-length(.)"/>
			</xsl:if>
		</xsl:for-each>
	</xsl:variable>
	<xsl:variable name="setname">
		<xsl:choose>
			<xsl:when test="$name">
				<xsl:value-of select="translate($name, '.-', '__')"/>
			</xsl:when>
			<xsl:when test="@xml:id and contains(@xml:id, '.bset')">
				<xsl:value-of select="translate(substring-before(@xml:id, '.bset'), '.-', '__')"/>
			</xsl:when>
			<xsl:when test="label/@key and contains(label/@key, '.bset')">
				<xsl:value-of select="translate(substring-before(label/@key, '.bset'), '.-', '__')"/>
			</xsl:when>
			<xsl:when test="@xml:id">
				<xsl:value-of select="translate(@xml:id, '.-', '__')"/>
			</xsl:when>
			<xsl:when test="label/@key">
				<xsl:value-of select="translate(label/@key, '.-', '__')"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:message terminate="yes">Cannot compute behaviorset name
s</xsl:message> 
			</xsl:otherwise>
		</xsl:choose>
	</xsl:variable>

<xsl:text/>
	{"<xsl:value-of select="@UUID"/>", NULL},  /* <xsl:value-of select="$setname"/> */
<xsl:apply-templates select="behaviordef"/>
</xsl:template>

<!--
########################################################################
	behaviordef
########################################################################
-->
<xsl:template match="behaviordef">
	<xsl:variable name="cname" select="translate(@name, '-.', '__')"/>

	<xsl:text>//	{"</xsl:text>
	<xsl:value-of select="@name"/>
	<xsl:text>",</xsl:text>
	<xsl:value-of select="substring($space,1,30 - string-length(@name))"/>
	<xsl:value-of select="$cname"/>
	<xsl:text>_bva</xsl:text>
	<xsl:value-of select="substring($space,1,30 - string-length(@name))"/>
	<xsl:text>},
</xsl:text>
</xsl:template>

<!--
########################################################################
	Label
########################################################################
-->
<xsl:template match="label">

<xsl:choose>
<!--
	<xsl:when test="$string">
		<xsl:value-of select="$string"/>
	</xsl:when>
-->
	<xsl:when test="@key">
		<xsl:value-of select="@key"/>
	</xsl:when>
	<xsl:otherwise>
		<xsl:value-of select="."/>
	</xsl:otherwise>
</xsl:choose>
</xsl:template>

<!--
########################################################################
	Generic recursive operation across string
########################################################################
-->
<xsl:template name="stringop">
<xsl:param name="numstr"/>
<xsl:param name="op"/>

<xsl:variable name="mystr" select="substring-before($numstr, ' ')"/>
<xsl:variable name="others" select="substring-after($numstr, ' ')"/>

	<xsl:choose>
		<xsl:when test="not($others)">
			<xsl:value-of select="$mystr"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:variable name="opot">
				<xsl:call-template name="stringop">
					<xsl:with-param name="numstr" select="$others"/>
					<xsl:with-param name="op" select="$op"/>
				</xsl:call-template>
			</xsl:variable>
	
			<xsl:choose>
				<xsl:when test="$op = 'sum'">
					<xsl:value-of select="$mystr + $opot"/>
				</xsl:when>
				<xsl:when test="$op = 'max' and $opot &gt; $mystr">
					<xsl:value-of select="$opot"/>
				</xsl:when>
				<xsl:when test="$op = 'min' and $opot &lt; $mystr">
					<xsl:value-of select="$opot"/>
				</xsl:when>
				<xsl:when test="$op = 'product'">
					<xsl:value-of select="$mystr * $opot"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:value-of select="$mystr"/>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<!--
########################################################################
	suppress output of text values
########################################################################
-->
<xsl:template match="text()|@*" mode="*">
</xsl:template>

</xsl:stylesheet>
