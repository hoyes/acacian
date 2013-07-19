<?xml version="1.0" encoding="US-ASCII"?>
<DDL version="1.1" xml:id="demo_dev.dev.DDL"
	xmlns:ea="http://www.engarts.com/namespace/2011/ddlx"
>
	<device UUID="684867b8-eb9b-11e2-b590-0017316c497d" date="2013-07-13" provider="http://www.esta.org/ddl/draft/" xml:id="demo_dev.dev">
		<UUIDname UUID="684867b8-eb9b-11e2-b590-0017316c497d" name="demo_dev.dev"/>
		<UUIDname UUID="72952a76-eb9b-11e2-989b-0017316c497d" name="demo_dev.lset"/>
		<UUIDname UUID="c7efa24c-dd82-11d9-881e-00e018a44101" name="devid.dev"/>
		<UUIDname UUID="3e2ca216-b753-11df-90fd-0017316c497d" name="acnbase-r2.bset"/>
		<!--
		<UUIDname UUID="5ff379ec-5f38-11df-9b04-0017316c497d" name="commonLabel.lset"/>
		<UUIDname UUID="4ef14fd4-2e8d-11de-876f-0017316c497d" name="sl.bset"/>
		<UUIDname UUID="3e1c078a-2e8d-11de-8c99-0017316c497d" name="sl.lset"/>
		<UUIDname UUID="654cab5c-b753-11df-a87e-0017316c497d" name="acnbase-r2.lset"/>
		<UUIDname UUID="5def7c40-35c1-11df-b42f-0017316c497d" name="acnbaseExt1.bset"/>
		<UUIDname UUID="49c78420-ceeb-11de-93c2-0017316c497d" name="demo_devNetwork.dev"/>
		-->
		<label key="demo_dev.dev" set="demo_dev.lset"/>
		<useprotocol name="ESTA.DMP"/>
		<includedev UUID="devid.dev">
			<label key="inc-device-id" set="demo_dev.lset"/>
			<protocol name="ESTA.DMP">
				<childrule_DMP loc="0"/>
				<ea:propext name="refsuffix" value="_devid"/>
			</protocol>
			<setparam name="devicename-UACN-factory-default">Un-named EAACN Demo Device</setparam>
			<setparam name="modelname-FCTN-value">EAACN Demo Device</setparam>
			<setparam name="manufacturer-name">Acuity Brands Lighting Inc.</setparam>
			<setparam name="manufacturer-URL">http:///acuitybrands.com</setparam>
		</includedev>
<!--
		<includedev UUID="demo_devNetwork.dev" xml:id="demo_devNetIncdev">
			<label key="inc-net-config" set="demo_dev.lset"/>
			<protocol name="ESTA.DMP">
				<childrule_DMP loc="234"/>
			</protocol>
		</includedev>
-->
		<property sharedefine="true" valuetype="NULL" xml:id="demo_devLuminaire">
			<behavior name="NULL" set="acnbase-r2.bset"/>
			<property sharedefine="true" valuetype="immediate" xml:id="baseDatum">
				<label key="datumDescRef" set="demo_dev.lset"/>
				<behavior name="deviceDatumDescription" set="acnbase-r2.bset"/>
				<behavior name="stringRef" set="acnbase-r2.bset"/>
				<value type="string">datumDescription</value>
			</property>
<!--
			<includedev UUID="linearsposn.dev" xml:id="rearLensIncdev">
				<protocol name="ESTA.DMP">
					<childrule_DMP loc="36"/>
					<ea:propext name="refsuffix" value="_lens_rear"/>
				</protocol>
			</includedev>
-->
		</property>
	</device>
</DDL>
