<?xml version="1.0" encoding="UTF-8"?>
<!--
#tabs=3s
-->
<DDL version="1.1" xml:id="demo_dev.dev.DDL"
   xmlns:ea="http://www.engarts.com/namespace/2011/ddlx"
>
   <device UUID="684867b8-eb9b-11e2-b590-0017316c497d" date="2013-07-13" provider="http://www.esta.org/ddl/draft/"
            xml:id="demo_dev.dev">
      <UUIDname UUID="684867b8-eb9b-11e2-b590-0017316c497d" name="demo_dev.dev" />

      <UUIDname UUID="72952a76-eb9b-11e2-989b-0017316c497d" name="demo_dev.lset" />

      <UUIDname UUID="c7efa24c-dd82-11d9-881e-00e018a44101" name="devid.dev" />

      <UUIDname UUID="3e2ca216-b753-11df-90fd-0017316c497d" name="acnbase-r2.bset" />

      <label key="demo_dev.dev" set="demo_dev.lset"></label>

      <useprotocol name="ESTA.DMP" />

      <includedev UUID="devid.dev" xml:id="deviceID">
         <protocol name="ESTA.DMP">
            <childrule_DMP loc="5" />
         </protocol>
   
         <setparam name="devicename-UACN-factory-default">Un-named EAACN Demo Device</setparam>
   
         <setparam name="modelname-FCTN-value">EAACN Demo Device</setparam>
   
         <setparam name="manufacturer-name">Acuity Brands Lighting Inc.</setparam>
   
         <setparam name="manufacturer-URL">http:///acuitybrands.com</setparam>
      </includedev>

      <property array="10" valuetype="NULL" xml:id="container">
         <protocol name="ESTA.DMP">
            <childrule_DMP loc="10" inc="8"/>
         </protocol>
            
         <property array="8" valuetype="network" xml:id="bargraph">
            <behavior name="type.uint" set="acnbase-r2.bset" />
      
            <behavior name="volatile" set="acnbase-r2.bset" />
      
            <behavior name="scalar" set="acnbase-r2.bset" />
      
            <protocol name="ESTA.DMP">
               <propref_DMP event="true" inc="1" loc="0" read="true" size="2" write="true" />
               <ea:propext name="functiondata" value="barvals"/>
            </protocol>
      
            <property valuetype="immediate" xml:id="barMax">
               <behavior name="limitMaxInc" set="acnbase-r2.bset" />
      
               <value type="uint">15</value>
            </property>
         </property>
      </property>
   </device>
</DDL>
