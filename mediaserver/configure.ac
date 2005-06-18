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
    src/storage/mysql/Makefile
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

AC_LANG([C++])

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


########### libmagic
MAGIC_OK=yes
AC_CHECK_HEADER(magic.h,
    [],
    [MAGIC_OK=no],
    /* checking for magic.h */
)
if test "x$MAGIC_OK" = xyes; then
    LDFLAGS="$LDFLAGS -lmagic"
    AC_DEFINE(HAVE_MAGIC)
fi

######### SQLite3

SQLITE3_OK=yes
AC_CHECK_HEADER(sqlite3.h,
    [],
    [SQLITE3_OK=no],
    /* checking for sqlite3.h */
)
if test "x$SQLITE3_OK" = xyes; then
    AC_CHECK_LIB(sqlite3, sqlite3_open,
        [],
        [SQLITE3_OK=no]
    )
fi
if test "x$SQLITE3_OK" = xyes; then
    AC_PATH_PROG(sqlite3prog, sqlite3, no)
    if test "$sqlite3prog" = no; then
        AC_MSG_RESULT([sqlite3 command line client not found])
	SQLITE3_OK=no
    fi
fi
if test "x$SQLITE3_OK" = xyes; then
    LDFLAGS="$LDFLAGS -lsqlite3"
    AC_DEFINE(HAVE_SQLITE3)
fi

########## MySQL

MYSQL_OK=yes

AC_PATH_PROG(mysqlconfig, mysql_config, no)
if test "$mysqlconfig" = no; then
    AC_MSG_RESULT([mysql_config script not found])
    MYSQL_OK=no
else
    AC_MSG_CHECKING(mysql includes)
    MYSQL_CFLAGS=`${mysqlconfig} --cflags`
    AC_MSG_RESULT($MYSQL_CFLAGS)

    AC_MSG_CHECKING(mysql libraries)
    MYSQL_LIBS=`${mysqlconfig} --libs`
    AC_MSG_RESULT($MYSQL_LIBS)

    CXXFLAGS="$CXXFLAGS $MYSQL_CFLAGS"
    LDFLAGS="$LDFLAGS $MYSQL_LIBS"

fi
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_HEADER(mysql.h,
        [],
        [MYSQL_OK=no],
        /* checking for mysql.h */
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_LIB(mysqlclient, mysql_init,
        [],
        [MYSQL_OK=no]
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_DEFINE(HAVE_MYSQL)
fi

# check if at least one database library present
if test "x$SQLITE3_OK" = xno; then
    if test "x$MYSQL_OK" = xno; then
        AC_MSG_ERROR(No suitable database libraries found)
    fi
fi


######### libjs

JS_OK=yes
AC_CHECK_HEADER(jsapi.h,
    [],
    [JS_OK=no],
    /* checking for jsapi.h */
    #define XP_UNIX 1
)
if test "x$JS_OK" = xyes; then
    AC_CHECK_LIB(js, JS_NewObject,
        [LDFLAGS="$LDFLAGS -ljs"],
        [
            AC_CHECK_LIB(smjs, JS_NewObject,
                [LDFLAGS="$LDFLAGS -lsmjs"],
                [JS_OK=no]
            )
        ]
    )
fi
if test "x$JS_OK" = xyes; then
   AC_DEFINE(HAVE_JS)
fi

######### id3tag

ID3_OK=yes
AC_CHECK_HEADER(id3/tag.h,
    [],
    [ID3_OK=no],
    /* checking for id3/tag.h */
)
if test "x$ID3_OK" = xyes; then
    AC_CHECK_HEADER(id3/misc_support.h,
        [],
        [ID3_OK=no],
        /* checking for id3/misc_support.h */
    )
fi
if test "x$ID3_OK" = xyes; then
   LDFLAGS="$LDFLAGS -lid3"
   AC_DEFINE(HAVE_ID3)
fi

######## exiv2

EXIV2_OK=yes
AC_CHECK_HEADER(exiv2/exif.hpp,
    [],
    [EXIV2_OK=no],
    /* checking for exiv2/exif.h */
)
if test "x$EXIV2_OK" = xyes; then
   LDFLAGS="$LDFLAGS -lexiv2"
   AC_DEFINE(HAVE_EXIV2)
fi


###############

AC_OUTPUT

echo
echo "======== CONFIGURATION SUMMARY ========="
echo "sqlite3 support:     $SQLITE3_OK"
echo "mysql support:       $MYSQL_OK"
echo "javascript support:  $JS_OK"
echo "libmagic support:    $MAGIC_OK"
echo "libid3 support:      $ID3_OK"
echo "libexiv2 support:    $EXIV2_OK"
echo

