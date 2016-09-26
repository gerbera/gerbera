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
                     $(LIBJS_CFLAGS) \
                     $(TAGLIB_CFLAGS) \
                     $(LIBEXTRACTOR_CFLAGS) \
                     $(ICONV_CXXFLAGS) \
                     $(LIBMAGIC_CFLAGS) \
                     $(ID3LIB_CFLAGS) \
                     $(FLAC_CFLAGS) \
                     $(LIBEXIF_CFLAGS) \
                     $(ZLIB_CFLAGS) \
                     $(PTHREAD_CFLAGS) \
                     $(INOTIFY_CFLAGS) \
                     $(AVFORMAT_CFLAGS) \
                     $(AVUTIL_CFLAGS) \
                     $(FFMPEGTHUMBNAILER_CFLAGS) \
                     $(CURL_CFLAGS) \
                     $(EXPAT_CFLAGS) \
                     $(LIBMP4V2_CFLAGS) \
                     $(LASTFMLIB_CFLAGS)

bin_PROGRAMS = mediatomb
mediatomb_SOURCES = $(top_srcdir)/src/main.cc
mediatomb_CXXFLAGS = -I$(top_srcdir)/src \
                     -I$(top_srcdir)/tombupnp/ixml/inc \
                     -I$(top_srcdir)/tombupnp/threadutil/inc \
                     -I$(top_srcdir)/tombupnp/upnp/inc \
                     -I$(top_srcdir) \
                     $(SQLITE3_CFLAGS) \
                     $(MYSQL_CFLAGS) \
                     $(LIBJS_CFLAGS) \
                     $(TAGLIB_CFLAGS) \
                     $(LIBEXTRACTOR_CFLAGS) \
                     $(ICONV_CXXFLAGS) \
                     $(LIBMAGIC_CFLAGS) \
                     $(ID3LIB_CFLAGS) \
                     $(FLAC_CFLAGS) \
                     $(LIBEXIF_CFLAGS) \
                     $(ZLIB_CFLAGS) \
                     $(PTHREAD_CFLAGS) \
                     $(INOTIFY_CFLAGS) \
                     $(AVFORMAT_CFLAGS) \
                     $(AVUTIL_CFLAGS) \
                     $(FFMPEGTHUMBNAILER_CFLAGS) \
                     $(CURL_CFLAGS) \
                     $(EXPAT_CFLAGS) \
                     $(LIBMP4V2_CFLAGS) \
                     $(LASTFMLIB_CFLAGS)

mediatomb_LDADD = \
    libmediatomb.a \
    $(top_srcdir)/tombupnp/build/libtombupnp.a \
    -luuid \
    $(LIBEXIF_LDFLAGS) \
    $(ZLIB_LDFLAGS) \
    $(EXPAT_LDFLAGS) \
    $(RT_LDFLAGS) \
    $(SQLITE3_LDFLAGS) \
    $(LIBMP4V2_LDFLAGS) \
	$(LIBMAGIC_LDFLAGS) \
    $(PTHREAD_LDFLAGS) \
    $(LIBEXTRACTOR_LDFLAGS) \
    $(AVFORMAT_LDFLAGS) \
    $(AVUTIL_LDFLAGS) \
    $(FFMPEGTHUMBNAILER_LDFLAGS) \
    $(LASTFMLIB_LDFLAGS) \
    $(ICONV_LIBS) \
    $(SQLITE3_LIBS) \
    $(MYSQL_LIBS) \
    $(TAGLIB_LIBS) \
    $(LIBEXTRACTOR_LIBS) \
    $(LIBJS_LIBS) \
	$(LIBMAGIC_LIBS) \
    $(ID3LIB_LIBS) \
    $(FLAC_LIBS) \
    $(LIBEXIF_LIBS) \
    $(ZLIB_LIBS) \
    $(ZLIB_LIBS) \
    $(RT_LIBS) \
    $(PTHREAD_CFLAGS) \
    $(PTHREAD_LIBS) \
    $(AVFORMAT_LIBS) \
    $(AVUTIL_LIBS) \
    $(FFMPEGTHUMBNAILER_LIBS) \
    $(EXPAT_LIBS) \
    $(LIBMP4V2_LIBS) \
    $(NSL_LIBS) \
    $(LWRES_LIBS) \
    $(LASTFMLIB_LIBS) \
    $(CURL_LIBS)
    
