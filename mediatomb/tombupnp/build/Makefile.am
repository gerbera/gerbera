EXTRA_DIST=gen_libtombupnp_src.sh

noinst_LIBRARIES=libtombupnp.a
include $(srcdir)/libtombupnp_src
libtombupnp_a_CFLAGS = \
          -I$(top_srcdir)/tombupnp/ixml/src/inc \
          -I$(top_srcdir)/tombupnp/ixml/inc \
          -I$(top_srcdir)/tombupnp/threadutil/inc \
          -I$(top_srcdir)/tombupnp/upnp/src/inc \
          -I$(top_srcdir)/tombupnp/upnp/inc \
          -I$(top_srcdir)

