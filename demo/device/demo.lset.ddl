<?xml version="1.0" encoding="US-ASCII"?>
<DDL
version="1.1"
xml:id="warp.lset.DDL"
><languageset
UUID="72952a76-eb9b-11e2-989b-0017316c497d"
date="2013-07-13"
provider="http://www.esta.org/ddl/draft/"
xml:id="warp.lset"
><UUIDname
UUID="72952a76-eb9b-11e2-989b-0017316c497d"
name="demo_dev.lset"
></UUIDname
><label
key="warp.lset"
set="warp.lset"
></label
><language
lang="en-US"
><string
key="warp.dev"
>ADB Warp top level device</string
><string
key="warp.lset"
>Labelset for ADB Warp devices</string
><string
key="inc-device-id"
>Standard Device Identification subdevice (ACN EPI-23).</string
><string
key="inc-net-config"
>Standard Network Configuration subdevice</string
><string
key="datumDescription"
>The origin for geometric description within the WARP/M is the centre point of the top plate. The Z axis is perpendicular to the top plate with z-value increasing into the unit (vertically downward as WARP is usually hung). The Y axis is within the plane of the top plate, perpendicular to the long sides of the topbox with y-value increasing away from the front (display/pushbutton) side of the unit. The X axis then follows standard mathematical right-hand convention. (within the plane of the top plate, parallel to the long sides of the topbox and increasing towards the network/DMX512 input end).</string
><string
key="rotatingAxisDevice"
>Device representing a generic rotating axis (pan, tilt, azimuth etc.).</string
><string
key="axisPosn"
>Property representing the actual position of the axis</string
><string
key="axisTarget"
>Target (requested position) for this axis</string
><string
key="axisScale"
>Axis scale</string
><string
key="axisDMXTarget"
>Axis target set by DMX512</string
><string
key="warpNetwork.dev"
>Warp networking interface device</string
><string
key="warpDMX.dev"
>Warp DMX512 data interface device (includes E1.31 and Artnet)</string
><string
key="DMX512controlInput"
>DMX512 control input (after prioritization and arbitration)</string
><string
key="mainDMXaddress"
>Main DMX address for Warp control</string
><string
key="DMXunivSelect"
>DMX source universe selector</string
><string
key="DMXsourceSelector"
>DMX source selector</string
><string
key="E1.31interface"
>E1.31 Interface</string
><string
key="E1.31active"
>E1.31 active indicator</string
><string
key="E1.31enable"
>E1.31 enable</string
><string
key="E1.31-universe"
>E1.31 universe select</string
><string
key="E1.31controllerCID"
>CID of currently resolved E1.31 universe source</string
><string
key="E1.31controllerUACN"
>UACN (source name) of currently resolved E1.31 universe source</string
><string
key="E1.31controllerPriority"
>Priority of currently resolved E1.31 stream</string
><string
key="E1.31sourcesExceded"
>E1.31 sources exceeded error reporter</string
><string
key="E1.31sourcesExcededSel"
>E1.31 sources exceeded numeric error</string
><string
key="sourcesExceededStates"
>E1.31 sources exceeded error labels</string
><string
key="noError"
>no error</string
><string
key="sourcesExceeded"
>too many sources for E1.31 universe (see E1.31 spec.)</string
><string
key="Artnetinterface"
>Artnet interface</string
><string
key="ArtnetActive"
>Artnet active indicator</string
><string
key="artnetEnable"
>Artnet enable</string
><string
key="artnetUniverse"
>Artnet universe select (includes &#8220;subnet&#8221;)</string
><string
key="DMXprimaryPair"
>Primary pair on DMX512 in/out connectors</string
><string
key="DMXsecondaryPair"
>Secondary pair on DMX512 in/out connectors</string
><string
key="axisDMXloLim"
>DMX512 low limit for axis</string
><string
key="axisDMXhiLim"
>DMX512 high limit for axis</string
><string
key="axisMax"
>Maximum value for axis property</string
><string
key="axisOffset"
>Offset of axis property relative to datum</string
><string
key="maxSpeed"
>Speed limit for axis</string
><string
key="upperMaxSpeed"
>Upper limit on axis speed limit</string
><string
key="calibrationState"
>Axis calibration state</string
><string
key="calibStateNames"
>Calibration state names</string
><string
key="calibrationControl"
>Calibration control (desired calibration state)</string
><string
key="lensposn.dev"
>Lens positioning device</string
><string
key="lensPosn"
>Lens position along axis</string
><string
key="lensPosScale"
>Lens position scale factor</string
><string
key="lensTarget"
>Lens target position (DMP)</string
><string
key="lensDMXTarget"
>Lens target position (DMX512/E1.31/Artnet)</string
><string
key="lensMax"
>Lens max value</string
><string
key="lensOffset"
>Offset of lens position property relative to datum</string
><string
key="uncalib"
>Uncalibrated (takes up to 20mins for full calibration)</string
><string
key="needreset"
>Uninitialized</string
><string
key="calibrated"
>Fully calibrated and initialized</string
><string
key="datumDescRef"
>Description of main datum for entire fixture</string
><string
key="tiltAxHeight"
>Tilt axis height (hanging height)</string
><string
key="focalPlane"
>Focal plane datum</string
><string
key="shutterRotateAll"
>Rotate all shutters</string
><string
key="lightBeam"
>Light beam group</string
><string
key="shutter.dev"
>Shutter device</string
><string
key="shutterGroup"
>Shutter group</string
><string
key="srot"
>Shutter rotation</string
><string
key="scaleFactor"
>Scale factor</string
><string
key="srotTarget"
>Shutter rotation target</string
><string
key="srotDMXTarget"
>Shutter rotation DMX512 target</string
><string
key="socc"
>Shutter occlusion</string
><string
key="soccTarget"
>Shutter occlusion target</string
><string
key="soccDMXTarget"
>Shutter occlusion DMX512 target</string
><string
key="iris.dev"
>Iris device</string
><string
key="irisAperture"
>Iris aperture size</string
><string
key="irisTarget"
>target aperture</string
><string
key="irisDMXTarget"
>target aperture set by DMX512</string
><string
key="irisMax"
>Max aperture</string
><string
key="irisOffset"
>Offset for iris aperture</string
><string
key="refToDimmer"
>Reference to dimmer driving lamp</string
><string
key="dimmerCID"
>CID of dimmer component</string
><string
key="gate"
>Gate of luminaire</string
><string
key="lampPosition"
>Lamp position</string
><string
key="accessorySlotFront"
>Front accessory slot</string
><string
key="accessorySlotRear"
>Rear accessory slot</string
><string
key="accessorySelect"
>Accessort type select</string
><string
key="emptySlot"
>Empty slot (no accessory)</string
><string
key="gobo.dev"
>Gobo device</string
><string
key="dmxpair.dev"
>DMX physical pair device</string
><string
key="DMXpair"
>DMX512 physical RS485 interface</string
><string
key="DMXdirection"
>DMX512 physical layer interface direction</string
><string
key="DMXactive"
>DMX enabled and active</string
><string
key="DMXenable"
>Enable DMX512 on this interface</string
><string
key="rearLens"
>Rear lens</string
><string
key="lens"
>Lens</string
><string
key="frontLens"
>Front lens</string
><string
key="gobo"
>Gobo</string
><string
key="goboNameList"
>Source catalog or reference for gobo name</string
><string
key="defaultname"
>Default value for parent</string
><string
key="goboRotator"
>Gobo rotator</string
><string
key="calibStateNames"
>Enumerated names of calibration states</string
><string
key="uncalib"
>Uncalibrated</string
><string
key="needreset"
>Calibrated but needing reset</string
><string
key="calibrated"
>Calibrated and ready to operate</string
><string
key="calibrationControl"
>Calibration control</string
><string
key="linearsposn.dev"
>Linear positioning device</string
><string
key="linearPosn"
>Actual linear position</string
><string
key="cyclicIndex.dev"
>Circular indexing device</string
><string
key="indexPosn"
>Indexed position</string
><string
key="indexedGobo.dev"
>Indexing rotating gobo device</string
></language
></languageset
></DDL
>
