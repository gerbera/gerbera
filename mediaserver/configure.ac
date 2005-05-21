AC_INIT(mediatomb, 0.7.1)
AM_INIT_AUTOMAKE(mediatomb, 0.7.1)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/mxml/Makefile
    src/zmm/Makefile
    src/zmmf/Makefile
    src/web/Makefile
    src/storage/Makefile
    src/storage/sqlite3/Makefile
    src/storage/fs/Makefile
    src/uuid/Makefile
    src/md5/Makefile
    src/scripting/Makefile
    config/Makefile
    doc/Makefile
    scripts/Makefile
    web/Makefile
    web/icons/Makefile
    import_scripts/Makefile
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
    [AC_MSG_ERROR(upnp/upnp.h not found, check libupnp installation)],
    /* checking for upnp/upnp.h */
)

AC_CHECK_HEADER(upnp/ixml.h,
    [],
    [AC_MSG_ERROR(upnp/ixml.h not found, check libupnp installation)],
    /* checking for upnp/ixml.h */
)

AC_CHECK_HEADER(upnp/ithread.h,
    [],
    [AC_MSG_ERROR(upnp/ithread.h not found, check libupnp installation)],
    /* checking for upnp/ithread.h */
)

AC_CHECK_HEADER(magic.h,
    [],
    [AC_MSG_ERROR(magic.h not found, check libmagic installation)],
    /* checking for magic.h */
)

AC_CHECK_HEADER(sqlite3.h,
    [],
    [AC_MSG_ERROR(sqlite3.h not found, check libsqlite3 installation)],
    /* checking for sqlite3.h */
)

AC_CHECK_HEADER(jsapi.h,
    [],
    [
        AC_MSG_ERROR(jsapi.h not found, check JavaScript library installation)
        AC_CHECK_HEADER(smjs/jsapi.h,
            [js_inc=-I/usr/include/smjs],
            [AC_MSG_ERROR(spidermonkey javascript library not found)],
        )
    ],
    /* checking for jsapi.h */
    #define XP_UNIX 1
)

AC_CHECK_LIB(js, JS_NewObject,
    [js_lib=-ljs],
    [
        AC_MSG_RESULT(not found)
        AC_CHECK_LIB(smjs, JS_NewObject,
            [js_lib=-lsmjs],
            [AC_MSG_ERROR(spidermonkey javascript library not found)],
        )
    ]
)
AC_SUBST(js_lib)

AC_OUTPUT

