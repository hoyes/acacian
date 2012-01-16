<?xml version="1.0" encoding="US-ASCII"?>
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

<xsl:text/>/*
DO NOT EDIT.
Automatically generated from DDL source.

  Behaviorset: <xsl:value-of select="$setname"/>
         UUID: <xsl:value-of select="@UUID"/>
     Provider: <xsl:value-of select="@provider"/>
         Date: <xsl:value-of select="@date"/>

<xsl:if test="@xml:id">
           ID: <xsl:value-of select="@xml:id"/>
</xsl:if>
<xsl:if test="label">
        Label: <xsl:apply-templates select="label"/>
</xsl:if>
*/

#include "acncommon.h"
#include "uuid.h"
#include "ddl/parse.h"
#include "ddl/behaviors.h"
#include "ddl/bvactions.h"

const bv_t bvs_<xsl:value-of select="$setname"/>[] = {
<xsl:apply-templates select="behaviordef">
	<xsl:with-param name="setname" select="$setname"/>
	<xsl:sort select="@name" data-type="text" order="ascending"/>
</xsl:apply-templates>
};

bvset_t bvset_<xsl:value-of select="$setname"/> = {
	.hd = {.uuid = "<xsl:value-of select="$uuidhexs"/>",},
	.nbvs = arraycount(bvs_<xsl:value-of select="$setname"/>),
	.bvs = bvs_<xsl:value-of select="$setname"/>,
};
</xsl:template>

<!--
########################################################################
	behaviordef
########################################################################
-->
<xsl:template match="behaviordef">
	<xsl:param name="setname"/>
	<xsl:variable name="cname" select="translate(@name, '-.', '__')"/>
	<xsl:variable name="bvaction" select="concat('BVA_', $setname, '_', $cname)"/>

<xsl:text/>#if defined(<xsl:value-of select="$bvaction"/>)
	{
		<xsl:text/>.name = "<xsl:value-of select="@name"/>",
		.action = &amp;<xsl:value-of select="$bvaction"/>,
	},
#endif
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
