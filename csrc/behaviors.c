/**********************************************************************/
/*

	Copyright (C) 2011, Engineering Arts. All rights reserved.

	Author: Philip Nye

	$Id$

#tabs=3
*/
/**********************************************************************/
/*
Behaviors

A behavior is identified by a UUID and a name. This is associated 
with a behavior action structure which defines how to treat 
properties with a given behavior. The action structure is largely 
application dependent and it is likely that multiple behavior names 
will use the same action structure. Actions may be required at class 
build time, at class instantiate time, when the property is 
accessed, when an instance is deleted or when the whole class is 
deleted.

For each behaviorset we generate an array of keys each of which connects
the set UUID, the name string and the action structure. For each 
property we then store an array of behavior keys.

The key array is sorted in lexical order of names allowing binary search
strategies.

Before calling the parser, the application can register known 
behaviorsets. On completion of parsing each device class, we can then 
optionally call the application with any class build time actions.

For behaviors which are not recognized as parts of pre-registered 
behaviorsets, new sets and records are generated with NULL actions, 
but this is a relatively expensive process - it is therefore 
beneficial to pre-register as many behaviors as possible, even if no 
actions are provided for them.

TODO: implement tracing refinements for unknown behaviors.
*/

#include <expat.h>
#include "acncommon.h"
#include "acnlog.h"
#include "acnmem.h"
#include "uuid.h"
#include "behaviors.h"

/**********************************************************************/
/*
Find a behavior (name and set-UUID) from the known behaviors
returns NULL if the behavior is not present
*/

struct bvkey_s *
findbv(uuid_t setuuid, const XML_Char *name, struct bset_s **bsetp)
{
	struct bset_s *bset;
	struct bvkey_s *bkey;
	unsigned int hi, lo, i;
	int cmp;

	bset = findbset(set);
	if (bset == NULL) return NULL;
	if (bsetp) *bsetp = bset;

	lo = 0;
	hi = bset->numkeys;
	bkey = bset->keys;

	while (hi > lo) {
		i = (hi + lo) / 2;
		cmp = strcmp(name, bkey[i].name);
		if (cmp == 0) return bkey + i;
		if (cmp < 0) hi = i;
		else lo = i + 1;
	}
	return NULL;
}

/**********************************************************************/
/*
Call to register a new behavior. If the number of behaviors is known
or can be estimated then this number of entries will be created, else
the set will be created for DFLTBVCOUNT behaviors.

If the number of behaviors added to a set exceeds the number of 
entries then an error is returned. (the set should be dynamically 
expanded but this is an expensive operation).
*/

int
findornewbv(uuid_t setuuid, const XML_Char *name, int countifnew, struct bvkey_s **rtnkey)
{
	struct uuidhd_s *bsethd;
	struct bset_s *bset;
	struct bvkey_s *bkey;
	unsigned int hi, lo, i;
	int cmp;
	int rslt;

	rslt = findornewuuid(&kbehaviors, setuuid, &bsethd, sizeof(struct bset_s));
	if (rslt < 0) return rslt;

	bset = container_of(bsethd, struct bset_s, hd);

	if (rslt) {   /* creating whole new set */
		if (countifnew <= 0) countifnew = DFLTBVCOUNT;
		bset->maxkeys = countifnew;
		
		countifnew *= sizeof(struct bvkey_s);
		
		/* allocate an array */
		bkey = _acnAlloc(countifnew);
		if (!bkey) {
			deluuid(&kbehaviors, &bset->hd, sizeof(struct bset_s));
			return -1;
		}
		memset(bkey, 0, countifnew);
		hi = 0;
	} else {  /* existing set */
		/* is the behavior there? */
		lo = 0;
		hi = bset->numkeys;
		bkey = bset->keys;

		while (hi > lo) {
			i = (hi + lo) / 2;
			cmp = strcmp(name, bkey[i].name);
			if (cmp == 0) {
				if (rtnkey) *rtnkey = bkey + i;
				return 0;
			} else if (cmp < 0) hi = i;
			else lo = i + 1;
		}
		if (bset->numkeys >= bset->maxkeys) return -1;	/* no space */
		bkey += hi;
		memmove(bkey + 1, 
					bkey, 
					sizeof(struct bvkey_s) * (bset->numkeys - hi));
	}
	/* got a new behavior which goes at bkey */
	bset->numkeys++;
	bkey->name = name;
	bkey->set = bset;
	bkey->action = NEWBVACTION;
	if (rtnkey) *rtnkey = bkey;
	return 1;
}

/**********************************************************************/
/*
TODO: We need behaviors to be added quickly and easily as slight 
variants are introduced which do not require major reprogramming 
effort.

TODO: For unknown behaviors we should have a scheme to retrieve the 
behaviorset and look for refinements we DO know about.
*/

/*
regex replacement to convert conventional UUID string into C byte sequence:
search  "-?([0-9a-fA-F][0-9a-fA-F])"
replace "0x\1, "
*/

/*
WARNING!
These are lexically sorted and the index in the array is key
new behaviorsets added MUST be in lexical order and the enum below
updated to compensate.
*/
/*
typedef struct pstring_t {uint8_t l; uint8_t s[];} pstring_t;
*/

/*
#define pstr(str) {sizeof(str),  str}
*/
#define pstr(str) str "\0"

const uint8_t behaviornames[] = {
	pstr("CID")
	pstr("CIDreference")
	pstr("DCID")
	pstr("DDLpropertyRef")
	pstr("DHCPLeaseRemaining")
	pstr("DHCPLeaseTime")
	pstr("DHCPclientState")
	pstr("DHCPserviceAddress")
	pstr("DMPbinding")
	pstr("DMPeventBinding")
	pstr("DMPgetPropBinding")
	pstr("DMPpropertyAddress")
	pstr("DMPpropertyRef")
	pstr("DMPsetPropBinding")
	pstr("DMXpropRef")
	pstr("DMXpropRef-SC0")
	pstr("EMPTY")
	pstr("ESTA_OrgID")
	pstr("FCTN")
	pstr("FCTNstring")
	pstr("IEEE_OUI")
	pstr("ISOdate")
	pstr("NULL")
	pstr("STARTCode")
	pstr("UACN")
	pstr("UACNstring")
	pstr("URI")
	pstr("URL")
	pstr("URN")
	pstr("UUID")
	pstr("abstractPriority")
	pstr("accessClass")
	pstr("accessEnable")
	pstr("accessInhibit")
	pstr("accessMatch")
	pstr("accessNetInterface")
	pstr("accessOrder")
	pstr("accessWindow")
	pstr("actionProperty")
	pstr("actionSpecifier")
	pstr("actionState")
	pstr("actionStateAfter")
	pstr("actionStateBefore")
	pstr("actionTimer")
	pstr("algorithm")
	pstr("angle")
	pstr("angle-deg")
	pstr("angle-rad")
	pstr("angleX")
	pstr("angleY")
	pstr("angleZ")
	pstr("area-sq-m")
	pstr("arraySize")
	pstr("atTime")
	pstr("atomicGroupMember")
	pstr("atomicLoad")
	pstr("atomicMaster")
	pstr("atomicMasterRef")
	pstr("atomicParent")
	pstr("atomicTrigger")
	pstr("atomicWithAncestor")
	pstr("autoAssignContextWindow")
	pstr("autoConnectedState")
	pstr("autoTrackedConnection")
	pstr("baseAddressDMX512")
	pstr("beamDiverter")
	pstr("beamGroup")
	pstr("beamShape")
	pstr("beamTemplate")
	pstr("behaviorRef")
	pstr("behaviorsetID")
	pstr("binObject")
	pstr("binder")
	pstr("binderRef")
	pstr("binding")
	pstr("bindingAnchor")
	pstr("bindingDMXalt-refresh")
	pstr("bindingDMXnull")
	pstr("bindingMechanism")
	pstr("bindingState")
	pstr("bitmap")
	pstr("boolean")
	pstr("boundProperty")
	pstr("case")
	pstr("character")
	pstr("charge-C")
	pstr("choice")
	pstr("colorFilter")
	pstr("colorSpec")
	pstr("componentReference")
	pstr("connectedState")
	pstr("connectedSwitch")
	pstr("connection.ESTA.DMP")
	pstr("connection.ESTA.SDT")
	pstr("connection.ESTA.SDT.ESTA.DMP")
	pstr("connectionContextDependent")
	pstr("connectionDependent")
	pstr("connectionMatch")
	pstr("connectionReporter")
	pstr("constant")
	pstr("contextDependent")
	pstr("contextMatchWindow")
	pstr("controllerContextDependent")
	pstr("coordinateReference")
	pstr("countdownTime")
	pstr("current-A")
	pstr("currentTarget")
	pstr("cyclic")
	pstr("cyclicDir.decreasing")
	pstr("cyclicDir.increasing")
	pstr("cyclicDir.shortest")
	pstr("cyclicPath")
	pstr("cyclicPath.decreasing")
	pstr("cyclicPath.increasing")
	pstr("cyclicPath.scalar")
	pstr("cyclicPath.shortest")
	pstr("date")
	pstr("date.firmwareRev")
	pstr("date.manufacture")
	pstr("datum")
	pstr("datumProperty")
	pstr("defaultRouteAddress")
	pstr("delayTime")
	pstr("devInfoItem")
	pstr("devModelName")
	pstr("devSerialNo")
	pstr("deviceDatum")
	pstr("deviceDatumDescription")
	pstr("deviceInfoGroup")
	pstr("deviceRef")
	pstr("deviceSupervisory")
	pstr("dim-angle")
	pstr("dim-area")
	pstr("dim-charge")
	pstr("dim-current")
	pstr("dim-energy")
	pstr("dim-force")
	pstr("dim-freq")
	pstr("dim-illuminance")
	pstr("dim-length")
	pstr("dim-luminous-flux")
	pstr("dim-luminous-intensity")
	pstr("dim-mass")
	pstr("dim-power")
	pstr("dim-pressure")
	pstr("dim-resistance")
	pstr("dim-solid-angle")
	pstr("dim-temp")
	pstr("dim-time")
	pstr("dim-torque")
	pstr("dim-voltage")
	pstr("dim-volume")
	pstr("dimension")
	pstr("dimensional-scale")
	pstr("direction")
	pstr("direction3D")
	pstr("driven")
	pstr("drivenAnd")
	pstr("drivenOr")
	pstr("driver")
	pstr("dynamicAccessEnable")
	pstr("encoding")
	pstr("energy-J")
	pstr("enumLabel")
	pstr("enumSelector")
	pstr("enumeration")
	pstr("errorReport")
	pstr("explicitConnectedState")
	pstr("force-N")
	pstr("fractionalSelector")
	pstr("freq-Hz")
	pstr("fullScale")
	pstr("globalDDLpropertyRef")
	pstr("group")
	pstr("hardwareVersion")
	pstr("illuminance-lx")
	pstr("initialization.enum")
	pstr("initializationBool")
	pstr("initializationState")
	pstr("initializer")
	pstr("internalBidiRef")
	pstr("internalMasterRef")
	pstr("internalSlaveRef")
	pstr("label")
	pstr("labelRef")
	pstr("labelString")
	pstr("languagesetID")
	pstr("length")
	pstr("length-m")
	pstr("lightSource")
	pstr("limit")
	pstr("limitByAccess")
	pstr("limitMaxExc")
	pstr("limitMaxInc")
	pstr("limitMinExc")
	pstr("limitMinInc")
	pstr("limitNetWrite")
	pstr("loadOnAction")
	pstr("localDDLpropertyRef")
	pstr("localDatum")
	pstr("localPropertyAddress")
	pstr("logratio")
	pstr("logunit")
	pstr("luminous-flux-lm")
	pstr("luminous-intensity-cd")
	pstr("manufacturer")
	pstr("mass-g")
	pstr("maunfacturerURL")
	pstr("maxDriven")
	pstr("maxDrivenPrioritized")
	pstr("maxPollInterval")
	pstr("measure")
	pstr("measureOffset")
	pstr("minDriven")
	pstr("minPollInterval")
	pstr("moveRelative")
	pstr("moveTarget")
	pstr("multidimensionalGroup")
	pstr("myAddressDHCP")
	pstr("myAddressLinkLocal")
	pstr("myAddressStatic")
	pstr("myNetAddress")
	pstr("namedPropertyRef")
	pstr("netAddress")
	pstr("netAddressIEEE-EUI")
	pstr("netAddressIPv4")
	pstr("netAddressIPv6")
	pstr("netCarrierRef")
	pstr("netDMX512-XLRpri")
	pstr("netDMX512-XLRsec")
	pstr("netHostAddress")
	pstr("netIfaceE1.31")
	pstr("netIfaceIPv4")
	pstr("netInterface")
	pstr("netInterfaceDMX512")
	pstr("netInterfaceDMX512pair")
	pstr("netInterfaceDirection")
	pstr("netInterfaceIEEE802.11")
	pstr("netInterfaceIEEE802.3")
	pstr("netInterfaceItem")
	pstr("netInterfaceRef")
	pstr("netInterfaceState")
	pstr("netMask")
	pstr("netMaskIPv4")
	pstr("netNetworkAddress")
	pstr("nonLinearity")
	pstr("nonlin-S-curve")
	pstr("nonlin-S-curve-precise")
	pstr("nonlin-ln")
	pstr("nonlin-log")
	pstr("nonlin-log10")
	pstr("nonlin-monotonic")
	pstr("nonlin-squareLaw")
	pstr("normalized-monotonic")
	pstr("normalized-nonlinearity")
	pstr("normalized-square-law")
	pstr("opticalLens")
	pstr("ordX")
	pstr("ordY")
	pstr("ordZ")
	pstr("ordered")
	pstr("ordinate")
	pstr("orientation")
	pstr("orientation3D")
	pstr("orthogonalLength")
	pstr("paramSzArray")
	pstr("perceptual-dimension")
	pstr("persistent")
	pstr("point2D")
	pstr("point3D")
	pstr("polarOrdinate")
	pstr("pollInterval")
	pstr("position3D")
	pstr("positionalSelector")
	pstr("power-W")
	pstr("power-dBmW")
	pstr("preferredValue")
	pstr("preferredValue.abstract")
	pstr("prefix-atto")
	pstr("prefix-exa")
	pstr("prefix-femto")
	pstr("prefix-giga")
	pstr("prefix-kilo")
	pstr("prefix-mega")
	pstr("prefix-micro")
	pstr("prefix-milli")
	pstr("prefix-nano")
	pstr("prefix-peta")
	pstr("prefix-pico")
	pstr("prefix-tera")
	pstr("prefix-yocto")
	pstr("prefix-yotta")
	pstr("prefix-zepto")
	pstr("prefix-zetta")
	pstr("pressure-Pa")
	pstr("priority")
	pstr("priorityZeroOff")
	pstr("progressCounter")
	pstr("progressIndicator")
	pstr("progressTimer")
	pstr("propertyActionSpecifier")
	pstr("propertyChangeAction")
	pstr("propertyLoadAction")
	pstr("propertyRef")
	pstr("propertySet")
	pstr("propertySetSelector")
	pstr("publishEnable")
	pstr("publishMaxTime")
	pstr("publishMinTime")
	pstr("publishParam")
	pstr("publishThreshold")
	pstr("pullBindingMechanism")
	pstr("pushBindingMechanism")
	pstr("radialLength")
	pstr("rangeOver")
	pstr("rate")
	pstr("rate1st")
	pstr("rate1stLimit")
	pstr("rate2nd")
	pstr("rate2ndLimit")
	pstr("ratio")
	pstr("readConnectedState")
	pstr("refInArray")
	pstr("reference")
	pstr("relativeTarget")
	pstr("repeatPrefVal")
	pstr("repeatPrefValOffset")
	pstr("resistance-ohm")
	pstr("routerAddress")
	pstr("scalable-nonLinearity")
	pstr("scalar")
	pstr("scale")
	pstr("selected")
	pstr("selector")
	pstr("serviceAddress")
	pstr("sharedProps")
	pstr("simplified-specialized")
	pstr("slotAddressDMX512")
	pstr("softwareVersion")
	pstr("solid-angle-sr")
	pstr("spatialCoordinate")
	pstr("streamCoverter")
	pstr("streamGroup")
	pstr("streamInput")
	pstr("streamMeasure")
	pstr("streamOuput")
	pstr("streamPoint")
	pstr("streamSource")
	pstr("stringRef")
	pstr("suspend")
	pstr("syncGroupMember")
	pstr("systemPropertyAddress")
	pstr("target")
	pstr("targetTimer")
	pstr("temp-K")
	pstr("temp-celsius")
	pstr("textString")
	pstr("time")
	pstr("time-s")
	pstr("timePeriod")
	pstr("timePoint")
	pstr("torque-Nm")
	pstr("trackTargetRef")
	pstr("transportConnection")
	pstr("trippable")
	pstr("type.NCName")
	pstr("type.bitmap")
	pstr("type.boolean")
	pstr("type.char.UTF-16")
	pstr("type.char.UTF-32")
	pstr("type.char.UTF-8")
	pstr("type.character")
	pstr("type.enum")
	pstr("type.enumeration")
	pstr("type.fixBinob")
	pstr("type.float")
	pstr("type.floating_point")
	pstr("type.integer")
	pstr("type.signed.integer")
	pstr("type.sint")
	pstr("type.string")
	pstr("type.uint")
	pstr("type.unsigned.integer")
	pstr("type.varBinob")
	pstr("typingPrimitive")
	pstr("unattainableAction")
	pstr("unitScale")
	pstr("universeIdDMX512")
	pstr("universeIdE1.31")
	pstr("volatile")
	pstr("voltage-V")
	pstr("volume-L")
	pstr("volume-cu-m")
	pstr("windowProperty")
	pstr("writeConnectedState")
	pstr("xenoBinder")
	pstr("xenoPropRef")
	pstr("xenoPropertyReference")
		
};


