EXTRA_DIST = gen_libmediatomb_src.sh

noinst_LIBRARIES = libmediatomb.a
include $(srcdir)/libmediatomb_src
libmediatomb_a_CXXFLAGS = -I../src \
                     -I../tombupnp/src/ixml/inc \
                     -I../tombupnp/src/threadutil/inc \
                     -I../tombupnp/src/upnp/inc

bin_PROGRAMS = mediatomb
mediatomb_SOURCES = ../src/main.cc
mediatomb_CXXFLAGS = -I../src \
                     -I../tombupnp/src/ixml/inc \
                     -I../tombupnp/src/threadutil/inc \
                     -I../tombupnp/src/upnp/inc

mediatomb_LDADD = \
    libmediatomb.a \
    ../tombupnp/build/libtombupnp.a 

