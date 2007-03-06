DESCRIPTION = "MediaTomb - UPnP AV MediaServer for Linux"
HOMEPAGE = "http://mediatomb.cc/"
LICENSE = "GPLv2"
DEPENDS = "sqlite3 libexif js zlib file id3lib"
PR = "r1"

SRC_URI = "${SOURCEFORGE_MIRROR}/mediatomb/mediatomb-0.9.0-pre.tar.gz"

S = "${WORKDIR}/${PN}-0.9.0-pre"

inherit autotools pkgconfig

EXTRA_OECONF = "--disable-mysql \
                --disable-rpl-malloc \
		--enable-sqlite3 \
                --enable-libjs \
		--enable-libmagic \
		--enable-id3lib \
		--enable-libexif \
                --disable-largefile \
                --with-sqlite3-h=${STAGING_INCDIR} \
                --with-sqlite3-libs=${STAGING_LIBDIR} \
                --with-magic-h=${STAGING_INCDIR} \
                --with-magic-libs=${STAGING_LIBDIR} \
                --with-exif-h=${STAGING_INCDIR} \
                --with-exif-libs=${STAGING_LIBDIR} \
                --with-zlib-h=${STAGING_INCDIR} \
                --with-zlib-libs=${STAGING_LIBDIR} \
                --with-js-h=${STAGING_INCDIR}/js \
                --with-js-libs=${STAGING_LIBDIR} \
                --with-id3lib-h=${STAGING_INCDIR} \
                --with-id3lib-libs=${STAGING_LIBDIR}"
