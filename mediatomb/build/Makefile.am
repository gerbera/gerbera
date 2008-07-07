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
                     $(SQLITE3_CFLAGS) \
                     $(MYSQL_CFLAGS) \
                     $(JS_CXXFLAGS) \
                     $(TAGLIB_CFLAGS) \
                     $(EXTRACTOR_CXXFLAGS) \
                     $(ICONV_CXXFLAGS) \
                     $(LIBMAGIC_CFLAGS) \
                     $(ID3LIB_CFLAGS) \
                     $(LIBEXIF_CFLAGS) \
                     $(ZLIB_CFLAGS) \
                     $(PTHREAD_CFLAGS) \
                     $(INOTIFY_CFLAGS) \
                     $(FFMPEG_CFLAGS) \
                     $(CURL_CFLAGS) \
                     $(EXPAT_CFLAGS) \
                     $(LIBMP4V2_CFLAGS) \
                     $(LIBDVDREAD_CFLAGS)

bin_PROGRAMS = mediatomb
mediatomb_SOURCES = $(top_srcdir)/src/main.cc
mediatomb_CXXFLAGS = -I$(top_srcdir)/src \
                     -I$(top_srcdir)/tombupnp/ixml/inc \
                     -I$(top_srcdir)/tombupnp/threadutil/inc \
                     -I$(top_srcdir)/tombupnp/upnp/inc \
                     -I$(top_srcdir) \
                     $(SQLITE3_CFLAGS) \
                     $(MYSQL_CFLAGS) \
                     $(JS_CXXFLAGS) \
                     $(TAGLIB_CFLAGS) \
                     $(EXTRACTOR_CXXFLAGS) \
                     $(ICONV_CXXFLAGS) \
                     $(LIBMAGIC_CFLAGS) \
                     $(ID3LIB_CFLAGS) \
                     $(LIBEXIF_CFLAGS) \
                     $(ZLIB_CFLAGS) \
                     $(PTHREAD_CFLAGS) \
                     $(INOTIFY_CFLAGS) \
                     $(FFMPEG_CFLAGS) \
                     $(CURL_CFLAGS) \
                     $(EXPAT_CFLAGS) \
                     $(LIBMP4V2_CFLAGS) \
                     $(LIBDVDREAD_CFLAGS)

mediatomb_LDADD = \
    libmediatomb.a \
    $(top_srcdir)/tombupnp/build/libtombupnp.a \
    $(LIBEXIF_LDFLAGS) \
    $(ZLIB_LDFLAGS) \
    $(EXPAT_LDFLAGS) \
    $(RT_LDFLAGS) \
    $(SQLITE3_LDFLAGS) \
    $(LIBMP4V2_LDFLAGS) \
    $(LIBMAGIC_LDFLAGS) \
    $(PTHREAD_LDFLAGS) \
    $(LIBDVDREAD_LDFLAGS) \
    $(FFMPEG_LDFLAGS) \
    $(ICONV_LIBS) \
    $(SQLITE3_LIBS) \
    $(MYSQL_LIBS) \
    $(TAGLIB_LIBS) \
    $(EXTRACTOR_LIBS) \
    $(JS_LIBS) \
    $(LIBMAGIC_LIBS) \
    $(ID3LIB_LIBS) \
    $(LIBEXIF_LIBS) \
    $(Z_LIBS) \
    $(RT_LIBS) \
    $(PTHREAD_CFLAGS) \
    $(PTHREAD_LIBS) \
    $(FFMPEG_LIBS) \
    $(CURL_LIBS) \
    $(EXPAT_LIBS) \
    $(LIBMP4V2_LIBS) \
    $(NSL_LIBS) \
    $(LWRES_LIBS) \
	$(LIBDVDREAD_LIBS)
    

