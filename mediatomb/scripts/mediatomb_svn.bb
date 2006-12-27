# MediaTomb UPnP Server

DESCRIPTION = "MediaTomb - UPnP AV MediaServer for Linux"
LICENSE = "GPLv2"
HOMEPAGE = "http://mediatomb.org/"
SRCDATE = "now"
SRC_URI = "svn://mediatomb.svn.sourceforge.net/svnroot/mediatomb/trunk;proto=https;module=mediatomb"
DEPENDS="sqlite3 libexif js zlib file taglib"
S = "${WORKDIR}/mediatomb"

PV = "0.9pre1+svn${SRCDATE}-sqlite"
PR = "r1"

inherit autotools 

EXTRA_OECONF="--disable-mysql --disable-rpl-malloc --enable-sqlite3 \
            --enable-libjs --enable-libmagic --enable-taglib --enable-libexif \
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
              --with-taglib-cfg=${STAGING_DIR}/${BUILD_SYS}/bin/taglib-config"

