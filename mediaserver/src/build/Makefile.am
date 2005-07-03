EXTRA_DIST = gen_libmediatomb_src.sh

noinst_LIBRARIES = libmediatomb.a
include $(srcdir)/libmediatomb_src
libmediatomb_a_CXXFLAGS = -I..

bin_PROGRAMS = mediatomb
mediatomb_SOURCES =

mediatomb_LDADD = \
    libmediatomb.a \
    -lupnp -lixml -lthreadutil -lz

