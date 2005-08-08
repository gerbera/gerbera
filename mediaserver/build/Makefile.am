EXTRA_DIST = gen_libmediatomb_src.sh

noinst_LIBRARIES = libmediatomb.a
include $(srcdir)/libmediatomb_src
libmediatomb_a_CXXFLAGS = -I../src

bin_PROGRAMS = mediatomb
mediatomb_SOURCES = ../src/main.cc
mediatomb_CXXFLAGS = -I../src

mediatomb_LDADD = \
    libmediatomb.a \
    -lupnp -lixml -lthreadutil -lz

