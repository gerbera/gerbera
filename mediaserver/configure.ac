AC_INIT(mediatomb, 0.8.0)
AM_INIT_AUTOMAKE(mediatomb, 0.8.0)

AC_CONFIG_FILES([
    Makefile

    doc/Makefile
    config/Makefile
    scripts/Makefile
    scripts/js/Makefile

    src/Makefile
    src/build/Makefile
    
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



######### SQLite3

SQLITE3_OK=yes
AC_ARG_ENABLE(sqlite3,
  [
The following options accept 'yes'(default) 'no' and 'auto' as an argument,
meaning ensure, disable and autodetect respectively.
If the corresponding --enable/disable option is not given for a particular
feature autodetect is assumed.

  --enable-sqlite3         sqlite3 support],
  [
     SQLITE3_EN=$enableval 
     if test "x$enableval" = xno; then
        SQLITE3_OK=disabled
     fi
  ]
)
if test "x$SQLITE3_OK" = xyes; then
    AC_CHECK_HEADER(sqlite3.h,
        [],
        [SQLITE3_OK=missing],
        /* checking for sqlite3.h */
    )
fi
if test "x$SQLITE3_OK" = xyes; then
    AC_CHECK_LIB(sqlite3, sqlite3_open,
        [],
        [SQLITE3_OK=missing]
    )
fi
if test "x$SQLITE3_OK" = xyes; then
    AC_PATH_PROG(sqlite3prog, sqlite3, no)
    if test "$sqlite3prog" = no; then
        AC_MSG_RESULT([sqlite3 command line client not found])
	SQLITE3_OK=missing
    fi
fi
if test "x$SQLITE3_OK" = xyes; then
    LDFLAGS="$LDFLAGS -lsqlite3"
    AC_DEFINE(HAVE_SQLITE3)
else
    if test "x$SQLITE3_EN" = xyes; then
        AC_MSG_ERROR(unable to configure sqlite3 support)
    fi
fi

########## MySQL

MYSQL_OK=yes
AC_ARG_ENABLE(mysql,
  [  --enable-mysql           mysql support],
  [
     MYSQL_EN=$enableval 
     if test "x$enableval" = xno; then
        MYSQL_OK=disabled
     fi
  ]
)
if test "x$MYSQL_OK" = xyes; then
    AC_PATH_PROG(mysqlconfig, mysql_config, no)
    if test "$mysqlconfig" = no; then
        AC_MSG_RESULT([mysql_config script not found])
        MYSQL_OK=missing
    else
        AC_MSG_CHECKING(mysql includes)
        MYSQL_CFLAGS=`${mysqlconfig} --cflags | sed s/\'//g`
        AC_MSG_RESULT($MYSQL_CFLAGS)

        AC_MSG_CHECKING(mysql libraries)
        MYSQL_LIBS=`${mysqlconfig} --libs | sed s/\'//g`
        AC_MSG_RESULT($MYSQL_LIBS)

        CXXFLAGS="$CXXFLAGS $MYSQL_CFLAGS"
        LDFLAGS="$LDFLAGS $MYSQL_LIBS"

    fi
fi
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_HEADER(mysql.h,
        [],
        [MYSQL_OK=missing],
        /* checking for mysql.h */
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_LIB(mysqlclient, mysql_init,
        [],
        [MYSQL_OK=missing]
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_DEFINE(HAVE_MYSQL)
else
    if test "x$MYSQL_EN" = xyes; then
        AC_MSG_ERROR(unable to configure mysql support)
    fi
fi

#########  check if at least one database available

if test "x$SQLITE3_OK" != xyes; then
    if test "x$MYSQL_OK" != xyes; then
        AC_MSG_ERROR(Support of at least one of mysql or sqlite3 must be configured)
    fi
fi


######### javascript

JS_OK=yes
AC_ARG_ENABLE(javascript,
  [  --enable-javascript      javascript support],
  [
     JS_EN=$enableval 
     if test "x$enableval" = xno; then
        JS_OK=disabled
     fi
  ]
)
if test "x$JS_OK" = xyes; then
    AC_CHECK_HEADER(jsapi.h,
        [],
        [JS_OK=missing],
        /* checking for jsapi.h */
        #define XP_UNIX 1
    )
fi
if test "x$JS_OK" = xyes; then
    AC_CHECK_LIB(js, JS_NewObject,
        [LDFLAGS="$LDFLAGS -ljs"],
        [
            AC_CHECK_LIB(smjs, JS_NewObject,
                [LDFLAGS="$LDFLAGS -lsmjs"],
                [JS_OK=missing]
            )
        ]
    )
fi
if test "x$JS_OK" = xyes; then
   AC_DEFINE(HAVE_JS)
else
    if test "x$JS_EN" = xyes; then
        AC_MSG_ERROR(unable to configure javascript support)
    fi
fi


########### libmagic
MAGIC_OK=yes

AC_ARG_ENABLE(libmagic,
  [  --enable-libmagic        libmagic support],
  [
     MAGIC_EN=$enableval 
     if test "x$enableval" = xno; then
        MAGIC_OK=disabled
     fi
  ]
)

if test "x$MAGIC_OK" = xyes; then
    AC_CHECK_HEADER(magic.h,
        [],
        [MAGIC_OK=missing],
        /* checking for magic.h */
    )
fi

if test "x$MAGIC_OK" = xyes; then
    LDFLAGS="$LDFLAGS -lmagic"
    AC_DEFINE(HAVE_MAGIC)
else
    if test "x$MAGIC_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libmagic support)
    fi
fi



######### id3tag

ID3_OK=yes
AC_ARG_ENABLE(libid3,
  [  --enable-libid3          libid3 support],
  [
     ID3_EN=$enableval 
     if test "x$enableval" = xno; then
        ID3_OK=disabled
     fi
  ]
)
if test "x$ID3_OK" = xyes; then
    AC_CHECK_HEADER(id3/tag.h,
        [],
        [ID3_OK=missing],
        /* checking for id3/tag.h */
    )
fi
if test "x$ID3_OK" = xyes; then
    AC_CHECK_HEADER(id3/misc_support.h,
        [],
        [ID3_OK=missing],
        /* checking for id3/misc_support.h */
    )
fi
if test "x$ID3_OK" = xyes; then
   LDFLAGS="$LDFLAGS -lid3"
   AC_DEFINE(HAVE_ID3)
else
    if test "x$ID3_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libid3 support)
    fi
fi

######## exiv2

EXIV2_OK=yes
AC_ARG_ENABLE(libexiv2,
  [  --enable-libexiv2        libexiv2 support],
  [
     EXIV2_EN=$enableval 
     if test "x$enableval" = xno; then
        EXIV2_OK=disabled
     fi
  ]
)
if test "x$EXIV2_OK" = xyes; then
    AC_CHECK_HEADER(exiv2/exif.hpp,
        [],
        [EXIV2_OK=missing],
        /* checking for exiv2/exif.h */
    )
fi
if test "x$EXIV2_OK" = xyes; then
   LDFLAGS="$LDFLAGS -lexiv2"
   AC_DEFINE(HAVE_EXIV2)
else
    if test "x$EXIV2_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libexiv2 support)
    fi
fi


######## extractor

EXTRACTOR_OK=yes
AC_ARG_ENABLE(libextractor,
  [  --enable-libextractor    libextractor support],
  [
     EXTRACTOR_EN=$enableval 
     if test "x$enableval" = xno; then
        EXTRACTOR_OK=disabled
     fi
  ]
)
if test "x$EXTRACTOR_OK" = xyes; then
    AC_CHECK_HEADER(extractor.h,
        [],
        [EXTRACTOR_OK=missing],
        /* checking for extractor.h */
    )
fi
if test "x$EXTRACTOR_OK" = xyes; then
   LDFLAGS="$LDFLAGS -lextractor"
   AC_DEFINE(HAVE_EXTRACTOR)
else
    if test "x$EXTRACTOR_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libextractor support)
    fi
fi


###############

AC_OUTPUT

echo
echo "======== CONFIGURATION SUMMARY ========="
echo "  sqlite3      : $SQLITE3_OK"
echo "  mysql        : $MYSQL_OK"
echo "  javascript   : $JS_OK"
echo "  libmagic     : $MAGIC_OK"
echo "  libid3       : $ID3_OK"
echo "  libexiv2     : $EXIV2_OK"
echo "  libextractor : $EXTRACTOR_OK"
echo

