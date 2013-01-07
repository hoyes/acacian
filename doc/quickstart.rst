eaACN Quick Start
=================

Configuration
-------------

There are many configuration options for eaACN. Your local configuration
is expected to be in a header file called *acncfg_local.h* where the 
compiler can find it and it will generate an error if this file is not
found. Any options you define in this file override 
default values. Any which you do not define will have default values
provided.

The file *acncfg.h* lists all configuration options, provides default values 
for them and has extensive doumentation of many.

**Note:** unlike some C projects, virtually *all* configuration options must
have a value or you may not get what you expect - that is, they must have a ``#define`` 
statement and not be left undefined or turned off with ``#undef``. For example,
if you don't want the FOO option, you must write in your *acncfg_local.h*::

  #define CONFIG_FOO 0

and **not**::

  #undef CONFIG_FOO  // wrong

Including Header Files
----------------------

There are many header files in the eaACN sources and their inclusion 
is very sensitive to ordering. The best way to handle them is simply 
to ``#include "acn.h"`` which first includes your configuration and then 
pulls in all the headers necessary in the right order.

Application Structure
---------------------

A basic but effective polling, timer and event system is provided in 
evloop.c If you use this (`#define CONFIG_ACN_EVLOOP 1`) it you can use it
within your application as a generic event loop and register callbacks
for all sorts of timeouts and I/Os outside of eaACN. if you do not want to use
it then you will need to provide the same functionality for eaACN by other means.



Application code must first create and initialize a local component 
structure. If eaACN has been compiled with CONFIG_SINGLE_COMPONENT 
there can only be one local component, but this must nevertheless be 
initialized. Initialization allocates the component ID (UUID) to the
component and performs other initialization as necessary.

`int init_Lcomponent(Lcomponent_t *Lcomp, cid_t cid)`

Return value: 0 on success, -1 on failure.


