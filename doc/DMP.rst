.. _DMP-API:

DMP Interface
=============

Function of the DMP layer
-------------------------

The messages of DMP are all concerned with exchanging values of properties 
within *devices*. In DMP terminology a *device* is any ACN component which
has properties which may be accessed using DMP.

It is the job of the application, not the ACN stack to determine 
which values to send or receive for which properties and in some of 
the simplest applications, the DMP layer may be omitted and simple 
DMP blocks sent to SDT directly. But in most cases, the DMP layer's 
primary role, is to connect the internal device model of the 
application to the property addresses and sizes of the protocol. This 
must be done without placing a straightjacket on the architecture of 
the application.

Connections and Components
--------------------------

DMP expects to operate over connections between components and the 
connection must be established before DMP messages can be exchanged\ 
[#]_. Because of this, most DMP messaging functions operate on 
pointers to connection structures which abstract whatever is needed 
by the connection layer. The connection implicitly identifies the 
remote component.

Main configuration options
--------------------------

DMP code includes both device and controller code since many 
components will include both. However, compile options 
:c:macro:`CONFIG_DMP_DEVICE` and :c:macro:`CONFIG_DMP_CONTROLLER` 
(both enabled by default) allow only one or the other to be built 
and the unneeded code is then omitted.

As with other layers, :c:macro:`CONFIG_SINGLE_COMPONENT` indicates 
there is guaranteed to be just one statically allocated local 
component (typical of many embedded implementations) and simplifies 
some of the code.

Also relevant is :c:macro:`CONFIG_SDT_SINGLE_CLIENT` and the 
corresponding DMP layer option :c:macro:`CONFIG_DMPISONLYCLIENT`. 
See: :ref:`single-clients`

DMP Strategy
------------

The boundary between generic DMP code and application specific code 
is not always clear. The approach in eaACN is rather different for
transmitting and receiving messages.

All DMP messages fit one of two basic formats. Both start with a 
Message code and an address format specifier. These are followed by 
either any number of addresses (e.g. *GET_PROPERTY*) of by addresses 
and data values interspersed (e.g. *SET_PROPERTY*). When data values 
are present these may either be property values (e.g. 
*SET_PROPERTY*) or reason codes (e.g. *SUBSCRIBE_REJECT*).

Transmitting DMP Messages
~~~~~~~~~~~~~~~~~~~~~~~~~

eaACN makes no attempt to parse or check outgoing messages for 
validity or compliance. It does however provide functions for 
accumulating multiple messages into a single block (a client 
protocol block in SDT). In the case of SDT transport, it is also 
possible to accumulate multiple blocks for different remote 
components into a single wrapper.

eaACN can also provides the application with tables of device properties
to assist in creating correct messages.

The DMP send support works like this and 
creates all the lower layer wrappers as well as the DMP stuff:

 1. Create and initialize a :c:type:`struct dmxtxcxt_s` which holds transmit 
    state and associated connection. This can be static, or can be 
    generated using :c:func:`newtxcxt()` [in dmpccs.h]. (For device messages 
    requiring responses (*GET_PROPERTY*, *SET_PROPERTY*, *SUBSCRIBE*) 
    this structure is automatically created in the context of the 
    received message.)
 
 2. Call :c:func:`dmp_openpdu()` passing your :c:type:`struct dmptxcxt_s`, 
    a combined command and range type  (e.g. see 
    :c:macro:`PDU_SETPROP_MANY` in dmpccs.h),  start address, 
    increment and count. This will start a new buffer if necessary, 
    create the PDU headers and return a pointer to the data area. It 
    will automatically use the best address format for the 
    parameters given - including using relative addresses where 
    appropriate. At this stage, count is only used to decide whether 
    to use range addresses and predict space requirements - it can 
    be reduced later (this is particularly useful when generating 
    response messages where the responses are expected to match the 
    queries but DMP level code cannot be sure until they are all 
    fully processed).
 
 3. Write appropriate data (if any) to the data area. Only some 
    commands actually have data - others e.g. *GET_PROPERTY* do not.
 
 4. Call :c:func:`dmp_closepdu()` with the final value for property 
    count and a pointer to the end of the data.

Steps 2 - 4 can be repeated as often as required to accumulate 
multiple commands in the buffer.

 5. Call :c:func:`dmp_flushpdus()` to transmit the accumulated buffer.

If the buffer fills up during 2 - 4 it will automatically be flushed 
and a new buffer started.

Receiving DMP Messages
~~~~~~~~~~~~~~~~~~~~~~

On receipt of a property message (and this means all DMP messages), 
the property address must be mapped onto the internal functionality 
for that property. In a device, this probably means making a local 
function call to find the property's value or to initiate the action 
implied by setting the property. In a controller, this means mapping 
the received message into the specific structure that the controller 
uses to do its work.

So for incoming PDUs the eaACN DMP not only decodes commands but 
resolves and checks DMP addresses against a property map of the 
remote device. Having resolved the property address eaACN then calls 
routines provided by the application to handle the message, passing 
a pointer to the entry in the property map. The property map format 
may be customized by the application to add any information it needs 
to tie the message to its own internal representation of specific 
properties. The application must provide the necessary message 
handling functions.

If a property address within a message is not found in the property 
map for the device or basic errors such as attempting to write to a 
read-only property are detected, DMP code will not pass the message 
to the application but can log the error and will generate an error 
response if the message expects one.

Property mapping
----------------

This implementation aims for a common format of tables for handling 
proterty maps of both local and remote components. In both cases, it 
is expected that these tables must be generated from the DDL 
description of the device.\ [#]_

Whilst property maps for both local and remote components are 
generated from DDL, local component maps can usually be generated 
once at compile time and built in to the code. Changing the DDL and 
re-building the code from it, is then the way to make any changes 
which would affect the description. In contrast, property maps for 
remote components need to be generated on the fly (except in some 
special-purpose dedicated controllers) and tools are provided to do 
this - see doc/DDL.txt

Property Map Formats
~~~~~~~~~~~~~~~~~~~~

eaACN provides two alternative mechanisms for looking up properties 
within a map:

:c:macro:`CONFIG_DMPMAP_INDEX`
  The simplest and fastest method uses a direct array lookup. This
  can be used where the number of properties and the address range
  they span are known to fit into a direct array within the resource 
  limits of the implementation. This is true for many device 
  implementations but rarely the case for controllers.

:c:macro:`CONFIG_DMPMAP_SEARCH`
  A more generic method which works better where the number and 
  distribution of propertiy addresses cannot be predicted or for 
  devices with very large property address range. In these 
  cases the primary lookup mechanism is a binary search which 
  rapidly identifies single properties and packed arrays, but a 
  secondary check may be needed where arrays overlap or are not 
  packed.

To Do: DMP
==========

DMP layer connection initiation and management calls are currently 
unimplemented. The intention is to create functions to establish DMP level
connections to remote components based on information passed by the 
application - usually from 
discovery. These will be thin wrappers around the transport layer 
(SDT for `standard`_ configuration) functions.

.. rubric:: Footnotes

.. [#]
   In the `standard`_ implementation this connection is a reciprocal pair of 
   SDT channels which are *connected* for DMP, but DMP has also been 
   operated over TCP connections and other transports are possible.

.. [#]
   For remote devices, using the DDL to generate tables is the only 
   extensible way to work. Doing so for local devices helps to 
   ensure that the local device functions as it's DDL says it does.
