DESCRIPTION = "MediaTomb - UPnP AV MediaServer for Linux"
HOMEPAGE = "http://mediatomb.org/"
LICENSE = "GPLv2"
DEPENDS = "sqlite3 libexif js zlib file taglib"
PV = "0.8+0.9pre1+svn${SRCDATE}-sqlite"
PR = "r1"

SRC_URI = "svn://mediatomb.svn.sourceforge.net/svnroot/mediatomb/trunk;proto=https;module=mediatomb"

S = "${WORKDIR}/mediatomb"

inherit autotools pkgconfig

EXTRA_OECONF = "--disable-mysql \
                --disable-rpl-malloc \
		--enable-sqlite3 \
                --enable-libjs \
		--enable-libmagic \
		--enable-taglib \
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
                --with-taglib-cfg=${STAGING_BINDIR_CROSS}/taglib-config"
