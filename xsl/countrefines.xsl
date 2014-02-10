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
	xmlns:ddl="http://www.esta.org/acn/namespace/ddl/2008/"
>
<!--
-->
	<xsl:output method="text" encoding="US-ASCII"/>

	<xsl:param name="name" select="''"/>

<!--
########################################################################
	Root node
########################################################################
-->
<xsl:template match="/">
	<xsl:apply-templates select="//behaviorset">
	</xsl:apply-templates>
</xsl:template>

<xsl:template match="behaviorset">
	<xsl:choose>
		<xsl:when test="label/@key">
			<xsl:value-of select="label/@key"/>
		</xsl:when>
		<xsl:when test="label">
			<xsl:value-of select="label"/>
		</xsl:when>
		<xsl:when test="@xml:id">
			<xsl:value-of select="@xml:id"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="@UUID"/>
		</xsl:otherwise>
	</xsl:choose>
	<xsl:text> </xsl:text>
	<xsl:for-each select="behaviordef">
		<xsl:sort select="count(refines)" order="descending"/>
		<xsl:if test="position() = 1">
			<xsl:value-of select="@name"/>
			<xsl:text> </xsl:text>
			<xsl:value-of select="count(refines)"/>
<xsl:text>
</xsl:text>
		</xsl:if>
		<xsl:apply-templates />
	</xsl:for-each>
</xsl:template>

<!--
########################################################################
	suppress output of text values
########################################################################
-->
<xsl:template match="text()|@*"/>

</xsl:stylesheet>
