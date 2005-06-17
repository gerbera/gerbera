AC_INIT(mediatomb, 0.8.0)
AM_INIT_AUTOMAKE(mediatomb, 0.8.0)

AC_CONFIG_FILES([
    Makefile

    doc/Makefile
    config/Makefile
    scripts/Makefile
    scripts/js/Makefile

    src/Makefile
    src/md5/Makefile
    src/metadata/Makefile
    src/mxml/Makefile
    src/storage/Makefile
    src/storage/fs/Makefile
    src/storage/sqlite3/Makefile
    src/scripting/Makefile
    src/uuid/Makefile
    src/web/Makefile
    src/zmm/Makefile
    src/zmmf/Makefile
    
    web/Makefile
    web/icons/Makefile
])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

AC_CHECK_HEADER(pthread.h,
    [],
    [AC_MSG_ERROR(pthread.h not found)],
    /* checking for pthread.h */
)

AC_CHECK_HEADER(iconv.h,
    [],
    [AC_MSG_ERROR(iconv.h not found)],
    /* checking for iconv.h */
)

AC_CHECK_HEADER(upnp/upnp.h,
    [],
    [AC_MSG_ERROR(upnp/upnp.h not found. Check libupnp installation)],
    /* checking for upnp/upnp.h */
)

AC_CHECK_HEADER(upnp/ixml.h,
    [],
    [AC_MSG_ERROR(upnp/ixml.h not found. Check libupnp installation)],
    /* checking for upnp/ixml.h */
)

AC_CHECK_HEADER(upnp/ithread.h,
    [],
    [AC_MSG_ERROR(upnp/ithread.h not found. Check libupnp installation)],
    /* checking for upnp/ithread.h */
)

AC_CHECK_HEADER(magic.h,
    [],
    [AC_MSG_ERROR(magic.h not found. Check libmagic installation)],
    /* checking for magic.h */
)

AC_CHECK_HEADER(sqlite3.h,
    [],
    [AC_MSG_ERROR(sqlite3.h not found. Check libsqlite3 installation)],
    /* checking for sqlite3.h */
)

AC_CHECK_HEADER(jsapi.h,
    [],
    [
        AC_CHECK_HEADER(smjs/jsapi.h,
            [js_includes=-I/usr/include/smjs],
            [AC_MSG_ERROR(spidermonkey javascript library not found)],
    	    /* checking for smjs/jsapi.h */
            #define XP_UNIX 1
        )
    ],
    /* checking for jsapi.h */
    #define XP_UNIX 1
)

AC_CHECK_LIB(js, JS_NewObject,
    [js_libs=-ljs],
    [
        AC_CHECK_LIB(smjs, JS_NewObject,
            [js_libs=-lsmjs],
            [AC_MSG_ERROR(spidermonkey javascript library not found)],
        )
    ]
)

AC_LANG([C++])
AC_CHECK_HEADER(id3/tag.h,
    [],
    [AC_MSG_ERROR(id3/tag.h not found. Check libid3 installation)],
    /* checking for id3/tag.h */
)

AC_CHECK_HEADER(id3/misc_support.h,
    [],
    [AC_MSG_ERROR(id3/misc_support.h not found. Check libid3 installation)],
    /* checking for id3/misc_support.h */
)


common_includes=$js_includes
AC_SUBST(common_includes)

AC_SUBST(js_libs)

AC_OUTPUT


