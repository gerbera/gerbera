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
                --with-sqlite3-h=${STAGING_DIR}/${TARGET_SYS}/include \
                --with-sqlite3-libs=${STAGING_DIR}/${TARGET_SYS}/lib \
                --with-magic-h=${STAGING_DIR}/${TARGET_SYS}/include \
                --with-magic-libs=${STAGING_DIR}/${TARGET_SYS}/lib \
                --with-exif-h=${STAGING_DIR}/${TARGET_SYS}/include \
                --with-exif-libs=${STAGING_DIR}/${TARGET_SYS}/lib \
                --with-zlib-h=${STAGING_DIR}/${TARGET_SYS}/include \
                --with-zlib-libs=${STAGING_DIR}/${TARGET_SYS}/lib \
                --with-js-h=${STAGING_DIR}/${TARGET_SYS}/include/js \
                --with-js-libs=${STAGING_DIR}/${TARGET_SYS}/lib \
                --with-taglib-cfg=${STAGING_BINDIR_CROSS}/taglib-config"
