
===
API
===

Components
==========

eaACN tracks local components and remote components separately. 
Local components are those implemented by the application you build 
and linked to eaACN. Remote components are those which your 
application communicates with. In many cases there may only be one 
local component, however in large applications where multiple 
components share a single eaACN instance, if these components 
communicate with each other via ACN they may occur as both local and 
remote components depending on their role in any particular the 
exchange.

Both local and remote component structures include sub-structures 
which are used by different processing layers. These are bundled 
together in a single structure because they almost always occur 
together and this saves lookups and references when components are 
passed between layers.

.. c:type:: Lcomponent_t

::

  typedef struct Lcomponent_s {
	  uuidhd_t hd;
	  enum useflags_e useflags;
	  epi10_Lcomp_t epi10;
	  sdt_Lcomp_t sdt;
	  dmp_Lcomp_t dmp;
  #if defined(app_Lcomp_t)
	  app_Lcomp_t app;
  #endif
  } Lcomponent_t;

.. c:type:: Rcomponent_t

::

  typedef struct Rcomponent_s {
	  uuidhd_t hd;
	  enum useflags_e useflags;
	  sdt_Rcomp_t sdt;
	  dmp_Rcomp_t dmp;
  #if defined(app_Rcomp_t)
	  app_Rcomp_t app;
  #endif
  } Rcomponent_t;

In both structures hd contains the UUID of the component in a strucure used for
rapid lookup bu UUID. Macros and routines for accessing the UUID and for
manipulating component sets by UUID are provided in uuid.h

DMP
===

.. c:function:: int dmp_register(Lcomponent_t *Lcomp, uint8_t expiry, netx_addr_t *adhocaddr, struct mcastscope_s *pscope)

dmp_register is called by the application to register a local component (Lcomponent_t)
which it implements. This call initializes all the necessary lower layers
if necessary.

Argument Lcomp is the local component structure which must have been initialized
and assigned its UUID see init_Lcomponent()

expiry is the time in seconds before the component expires
