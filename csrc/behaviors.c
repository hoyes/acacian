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

typedef void modify_bvaction();
typedef void instance_bvaction();

void devref_inst_action() {}

/*
behavioraction_s defines the actions associated with a behavior
*/
struct bvaction_s {
	instance_bvaction *inst;
	modify_bvaction *modify;
};

struct bvkey_s {
	const char *name;
	struct bvset_s *set;
	struct bvaction_s *action;
};

struct bvset_s {
	struct uuidhd_s hd;
	int numkeys;
	int maxkeys;
	struct bvkey_s *keys;
};




struct bvkey_s bh_devref3 = {
	NULL,
	/* 3e2ca216-b753-11df-90fd-0017316c497d New ACN Base */
	"\x3e\x2c\xa2\x16\xb7\x53\x11\xdf\x90\xfd\x00\x17\x31\x6c\x49\x7d"
	"deviceRef"
};

struct bvkey_s bh_devref2 = {
	&bh_devref3,
	/* 71576eac-e94a-11dc-b664-0017316c497d Rev1 ACN Base */
	"\x71\x57\x6e\xac\xe9\x4a\x11\xdc\xb6\x64\x00\x17\x31\x6c\x49\x7d"
	"deviceRef"
};

struct bvkey_s bh_devref1 = {
	&bh_devref2,
	/* a713a314-a14d-11d9-9f34-000d613667e2 Original DMP base */
	"\xa7\x13\xa3\x14\xa1\x4d\x11\xd9\x9f\x34\x00\x0d\x61\x36\x67\xe2"
	"deviceRef"
};

struct behavior_s known_behaviors[] = {
	{&bh_devref1, {&devref_inst_action, NULL}},
};
/**********************************************************************/
/*
Call to create a new behaviorset. If the number of behaviors is known
or can be estimated then this number of entries will be created, else
the set will be created for DFLTBVCOUNT behaviors.

If the number of behaviors added to a set exceeds the number of 
entries then the set will be dynamically expanded but this is an 
expensive operation.
*/

int
registerbvset(uuid_t uuid, struct bvkey_s keys, int numbhvs)
{
	struct uuidhd_s *bvsethd;
	struct bvset_s *bset;
	int rslt;

	rslt = findornewuuid(bvsets, uuid, &bvsethd, sizeof(struct bvset_s));
	if (rslt < 0) return rslt;

	bset = container_of(bvsethd, struct bvset_s, hd);

	if (rslt == 0) {  /* existing set */
		if (numbhvs > bset->maxkeys) {
			expand?
		}
	} else {
		bset->keys = acnAlloc(numbhvs * sizeof(struct bvkey_s));
	}
}













/**********************************************************************/

/*
Behavior names are unwieldy because they depend on a UUID and a name.
Since the ones we are interested in are

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
typedef unsigned char uint8_t;
typedef uint8_t uuid_t[16];

const uuid_t behaviorsets[] = {
	/* 3e2ca216-b753-11df-90fd-0017316c497d acnbase-r2.bset */
	{0x3e, 0x2c, 0xa2, 0x16, 0xb7, 0x53, 0x11, 0xdf, 0x90, 0xfd, 0x00, 0x17, 0x31, 0x6c, 0x49, 0x7d},
	/* 71576eac-e94a-11dc-b664-0017316c497d acnbase-r1.bset */
	{0x71, 0x57, 0x6e, 0xac, 0xe9, 0x4a, 0x11, 0xdc, 0xb6, 0x64, 0x00, 0x17, 0x31, 0x6c, 0x49, 0x7d},
};
enum behaviorset_e {
	bset_acnbase_r2,
	bset_acnbase_r1,
};






















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


