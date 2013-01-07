.. _discovery:

Discovery in eaACN
==================

Introduction
------------

As with most parts of the ACN standard, discovery is treated as a 
separate module which can be changed or replaced with minimal affect 
on other parts of the standard. The preferred method for discovery 
is to use SLP (Service Location Protocol ). However, alternative 
standardized discovery techniques such as `Avahi <http://avahi.org/>`_ 
(or `Apple Bonjour <http://www.apple.com/support/bonjour/>`_) 
should be catered for\ [#]_.

Discovery is obviously dependent on the services and protocols which
are required. Discovery of DMP devices using SLP is covered by epi19_, whilst
discovery of E1.31_ devices is covered elsewhere.

eaACN Approach
--------------

Because SLP is an open protocol with open-source code available 
:ref:OpenSLP, no attempt has been made in eaACN to implement 
discovery but these guidelines should be followed in integrating 
other software.

 - Discovery is a separate layer in ACN which talks directly to the 
   application.

 - Any application which does not initiate new connections but which 
   simply listens for incoming connections or only extablishes 
   additional connections to components which are already known at the 
   transport layer (this includes most ordinary devices) have no use 
   of active discovery at all, they simply need to advertise their 
   services and this may simply be a matter of supplying a static 
   configuration file to a standalone module such as openSLP.

 - Applications which actively discover other components (for 
   example, most controllers) need to extract sufficient information 
   from the process that they can then supply to lower layers to 
   establish connections.

 - When discovery protocols provide additional information beyond the
   most basic connection information, the application may use this to
   make decisions about how, when and whether to connect without ever
   involving the protocol layers of eaACN.


DMP Discovery
-------------

For DMP to work, a host on the system needs to be able to reliably
discover all (or a user configured subset of) the devices\ [#]_
accessible on the network.

For each device discovered, the following information must be provided:

  - the CID of the component containing that device. This enables 
    the application to detect when the same device is accessible 
    through multiple interfaces or protocols and select between them.

  - the DCID of the root level Device class of the device. This 
    information is needed before DMP can determine the property map of 
    the device and is not available through DMP itself.
  
  - the transport protocol used to connect to the device and sufficient 
    protocol information to establish a network connection to it.

Procedure
~~~~~~~~~

The application should interface directly with openSLP or whatever 
other discovery is used and having identified components to which it 
wants to connect should then extract the necessary protocol data (IP 
address, port, CID, etc.) to pass to the transport layer calls to 
make the connection.

Discovery for Other Protocols
-----------------------------

For other protocols than DMP the requirements are usually similar but
may be standardized differently.

To Do: Discovery
================

Implement helper calls for OpenSLP to handle URLs and attributes of ACN
components in accordance with epi19_.

.. rubric::Footnotes

.. [#]
   inventing (or borrowing) ad-hoc lightweight discovery 
   protocols based on multicast or broadcast queries often appears 
   trivial and attractive but such protocols rarely scale well and 
   inevitably generate portability and performance issues when the 
   network environment deviates from that envisaged when creating the 
   protocol. Use of such protocols is strongly discouraged.

.. [#]
   there is no necessity to be able to discover components which 
   do not expose devices (those with no properties) although such 
   discovery may be useful for system management and is provided by any 
   component complying with epi19.


