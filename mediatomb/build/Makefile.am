EXTRA_DIST = gen_libmediatomb_src.sh
CLEANFILES  = IUpnpErrFile.txt IUpnpInfoFile.txt

noinst_LIBRARIES = libmediatomb.a
include $(srcdir)/libmediatomb_src

libmediatomb_a_CXXFLAGS = \
                     -I$(top_srcdir)/src \
                     -I$(top_srcdir)/tombupnp/ixml/inc \
                     -I$(top_srcdir)/tombupnp/threadutil/inc \
                     -I$(top_srcdir)/tombupnp/upnp/inc \
                     -I$(top_srcdir) \
                     $(SQLITE3_CXXFLAGS) \
                     $(MYSQL_CFLAGS) \
                     $(PGSQL_CFLAGS) \
                     $(JS_CXXFLAGS) \
                     $(TAGLIB_CFLAGS) \
                     $(EXTRACTOR_CXXFLAGS) \
                     $(ICONV_CXXFLAGS) \
                     $(MAGIC_CXXFLAGS) \
                     $(ID3_CXXFLAGS) \
                     $(EXIF_CXXFLAGS) \
                     $(Z_CXXFLAGS) \
                     $(PTHREAD_CFLAGS) \
                     $(INOTIFY_CXXFLAGS)

bin_PROGRAMS = mediatomb
mediatomb_SOURCES = $(top_srcdir)/src/main.cc
mediatomb_CXXFLAGS = -I$(top_srcdir)/src \
                     -I$(top_srcdir)/tombupnp/ixml/inc \
                     -I$(top_srcdir)/tombupnp/threadutil/inc \
                     -I$(top_srcdir)/tombupnp/upnp/inc \
                     -I$(top_srcdir) \
                     $(SQLITE3_CXXFLAGS) \
                     $(MYSQL_CFLAGS) \
                     $(PGSQL_CFLAGS) \
                     $(JS_CXXFLAGS) \
                     $(TAGLIB_CFLAGS) \
                     $(EXTRACTOR_CXXFLAGS) \
                     $(ICONV_CXXFLAGS) \
                     $(MAGIC_CXXFLAGS) \
                     $(ID3_CXXFLAGS) \
                     $(EXIF_CXXFLAGS) \
                     $(Z_CXXFLAGS) \
                     $(PTHREAD_CFLAGS) \
                     $(INOTIFY_CXXFLAGS)

mediatomb_LDADD = \
    libmediatomb.a \
    $(top_srcdir)/tombupnp/build/libtombupnp.a \
    $(ICONV_LIBS) \
    $(SQLITE3_LIBS) \
    $(MYSQL_LIBS) \
    $(PGSQL_LIBS) \
    $(TAGLIB_LIBS) \
    $(EXTRACTOR_LIBS) \
    $(JS_LIBS) \
    $(MAGIC_LIBS) \
    $(ID3_LIBS) \
    $(EXIF_LIBS) \
    $(Z_LIBS) \
    $(RT_LIBS) \
    $(PTHREAD_CFLAGS) \
    $(PTHREAD_LIBS)

