EXTRA_DIST=gen_libtombupnp_src.sh

noinst_LIBRARIES=libtombupnp.a
include $(srcdir)/libtombupnp_src
AM_CFLAGS=\
          -I$(top_srcdir)/tombupnp/src/ixml/src/inc \
          -I$(top_srcdir)/tombupnp/src/ixml/inc \
          -I$(top_srcdir)/tombupnp/src/threadutil/inc \
          -I$(top_srcdir)/tombupnp/src/upnp/src/inc \
          -I$(top_srcdir)/tombupnp/src/upnp/inc

