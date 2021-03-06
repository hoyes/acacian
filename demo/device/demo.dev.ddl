<?xml version="1.0" encoding="UTF-8"?>
<!--=================================================================-->
<!--
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Copyright (c) 2013, Acuity Brands, Inc.

Author: Philip Nye <philip.nye@engarts.com>

#tabs=2s
-->
<!--=================================================================-->
<!--
file: demo.dev.ddl

Device Description for simple Acacian demo device.

DCID is 684867b8-eb9b-11e2-b590-0017316c497d
-->
<!--=================================================================-->
<DDL version="1.1" xml:id="demo_dev.dev.DDL">
  <device UUID="684867b8-eb9b-11e2-b590-0017316c497d" date="2013-07-13" provider="http://www.esta.org/ddl/draft/"
          xml:id="demo_dev.dev">
    <UUIDname UUID="684867b8-eb9b-11e2-b590-0017316c497d" name="demo_dev.dev" />

    <UUIDname UUID="72952a76-eb9b-11e2-989b-0017316c497d" name="demo_dev.lset" />

    <UUIDname UUID="c7efa24c-dd82-11d9-881e-00e018a44101" name="devid.dev" />

    <UUIDname UUID="3e2ca216-b753-11df-90fd-0017316c497d" name="acnbase-r2.bset" />

    <label key="demo_dev.dev" set="demo_dev.lset"/>

    <useprotocol name="ESTA.DMP" />

<!--
Include the standard device ID subdevice.
-->
    <includedev UUID="devid.dev" xml:id="deviceID">
      <protocol name="ESTA.DMP">
        <childrule_DMP loc="5" />
      </protocol>

      <setparam name="devicename-UACN-factory-default">&lt;un-named&gt;</setparam>

      <setparam name="modelname-FCTN-value">Acacian Demo Device</setparam>

      <setparam name="manufacturer-name">Acuity Brands Lighting Inc.</setparam>

      <setparam name="manufacturer-URL">http:///acuitybrands.com</setparam>
    </includedev>

<!--
The outer array (array of rows)
-->
    <property array="20" valuetype="NULL" xml:id="outer_array">
      <behavior set="acnbase-r2.bset" name="group"/>
      <protocol name="ESTA.DMP">
        <childrule_DMP inc="5" loc="100" />
      </protocol>

<!--
The inner array (row of scalar bargraph elements).
-->
      <property array="10" valuetype="network" xml:id="bargraph">
        <behavior name="type.uint" set="acnbase-r2.bset" />

        <behavior name="volatile" set="acnbase-r2.bset" />

        <behavior name="scalar" set="acnbase-r2.bset" />

        <protocol name="ESTA.DMP">
<!--
DMP access specification
-->
          <propref_DMP event="true" inc="96" loc="0" read="true" size="2" write="true" />

          <ea:propext name="propdata" value="barvals" xmlns:ea="http://www.engarts.com/namespace/2011/ddlx" />

          <ea:propext name="fn_getprop" value="&amp;getbar" xmlns:ea="http://www.engarts.com/namespace/2011/ddlx" />

          <ea:propext name="fn_setprop" value="(dmprx_fn *)&amp;setbar"
                      xmlns:ea="http://www.engarts.com/namespace/2011/ddlx" />

          <ea:propext name="fn_subscribe" value="&amp;subscribebar"
                      xmlns:ea="http://www.engarts.com/namespace/2011/ddlx" />

          <ea:propext name="fn_unsubscribe" value="&amp;unsubscribebar"
                      xmlns:ea="http://www.engarts.com/namespace/2011/ddlx" />
        </protocol>

<!--
Maximum value of each bargraph element.
-->
        <property valuetype="immediate" xml:id="barMax">
          <behavior name="limitMaxInc" set="acnbase-r2.bset" />

          <value type="uint">499</value>
        </property>
      </property>
    </property>
  </device>
</DDL>
