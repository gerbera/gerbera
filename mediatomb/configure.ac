AC_PREREQ(2.59)
AC_INIT([MediaTomb], [0.9.0], [jin@mediatomb.org])
AC_CONFIG_HEADER([autoconfig.h tombupnp/upnp/inc/upnpconfig.h])
AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_SRCDIR([src/common.h])
AM_INIT_AUTOMAKE([1.9 -Wall])

###############################################################################
# The following block is derived from the configure.ac script from the 
# libupnp-1.3.1 package, we just set the version number to 0.1.0
# (C) Copyright 2005-2006 RИmi Turboult <r3mi@users.sourceforge.net>
upnpmaj=0
upnpmin=1
upnppatch=0
AC_DEFINE_UNQUOTED([UPNP_VERSION_STRING], "$PACKAGE_VERSION",
    [see upnpconfig.h])
AC_DEFINE_UNQUOTED([UPNP_VERSION_MAJOR], $upnpmaj, [see upnpconfig.h])
AC_DEFINE_UNQUOTED([UPNP_VERSION_MINOR], $upnpmin, [see upnpconfig.h])
AC_DEFINE_UNQUOTED([UPNP_VERSION_PATCH], $upnppatch, [see upnpconfig.h])
###############################################################################
 
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_SYS_LARGEFILE
AC_LANG_CPLUSPLUS

   
AC_CHECK_HEADERS([syslog.h unistd.h alloca.h iconv.h arpa/inet.h fcntl.h], [],
                 [AC_MSG_ERROR(required header not found)]) 

AC_CHECK_HEADERS([limits.h netdb.h netinet/in.h stdlib.h string.h sys/file.h],
                 [],
                 [AC_MSG_ERROR(required header not found)]) 
AC_CHECK_HEADERS([sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h pthread.h],
                 [],
                 [AC_MSG_ERROR(required header not found)]) 
    
  
# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIGNAL
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_FUNC_FSEEKO

AC_CHECK_FUNCS([ftime gethostname gettimeofday inet_ntoa localtime_r memmove memset],
               [],
               [AC_MSG_ERROR(required library function not found)])
AC_CHECK_FUNCS([regcomp select socket strcasecmp strchr strdup strerror strncasecmp],
               [],
               [AC_MSG_ERROR(required library function not found)])
AC_CHECK_FUNCS([strrchr strstr strtol strtoul uname pthread_mutex_lock], [],
               [AC_MSG_ERROR(required library function not found)])

# this is not optional, we need the device and the internal webserver
AC_DEFINE([UPNP_HAVE_DEVICE], [1], [Compile device API])
AC_DEFINE([UPNP_HAVE_WEBSERVER], [1], [Compile internal web server])    

AC_CHECK_LIB(pthread, pthread_mutex_lock,[], [AC_MSG_ERROR(required library function not found)])    
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
    AC_DEFINE([HAVE_SQLITE3], [1], [sqlite3 library presence])
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
    if test "x$mysqlconfig" = xno; then
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
    AC_DEFINE([HAVE_MYSQL], [1], [MySLQ library presence])
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
        [
            CXXFLAGS_SAVE=$CXXFLAGS
            CXXFLAGS="$CXXFLAGS -I/usr/include/smjs"
            
            AC_MSG_CHECKING([alternative libjs header location (debian)])
            AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM(
                    [
                        #define XP_UNIX 1
                        #include <jsapi.h>
                    ],
                    [
                        (void)JS_NewContext(0, 0);
                    ]
                )],
                [
                    AC_MSG_RESULT([found under /usr/include/smjs])
                ],
                [
                    CXXFLAGS_SAVE=$CXXFLAGS
                    CXXFLAGS="$CXXFLAGS -I/usr/include/js"
            
                    AC_MSG_CHECKING([alternative libjs header location (SuSE)])
                    AC_COMPILE_IFELSE(
                        [AC_LANG_PROGRAM(
                            [
                                #define XP_UNIX 1
                                #include <jsapi.h>
                            ],
                            [
                                (void)JS_NewContext(0, 0);
                            ]
                        )],
                        [
                            AC_MSG_RESULT([found under /usr/include/js])
                        ],
                        [
                            CXXFLAGS="$CXXFLAGS_SAVE"
                            JS_OK=missing
                            AC_MSG_RESULT([not found, giving up])
                        ]
                    )
                ]
            )
        ],
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
   AC_DEFINE([HAVE_JS], [1], [libjs presence])
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
    AC_DEFINE([HAVE_MAGIC], [1], [filemagic library presence])
else
    if test "x$MAGIC_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libmagic support)
    fi
fi



######### id3tag

ID3_OK=yes
AC_ARG_ENABLE(id3lib,
  [  --enable-id3lib          id3lib support],
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
   AC_DEFINE([HAVE_ID3], [1], [id3lib presence])
else
    if test "x$ID3_EN" = xyes; then
        AC_MSG_ERROR(unable to configure id3lib support)
    fi
fi


######## extractor

EXTRACTOR_OK=disabled
AC_ARG_ENABLE(libextractor,
  [  --enable-libextractor    libextractor support],
  [
     EXTRACTOR_EN=$enableval 
     if test "x$enableval" = xno; then
        EXTRACTOR_OK=disabled
     else
        EXTRACTOR_OK=yes
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

#disabling version check, 0.5.2 is broken!!!
#if test "x$EXTRACTOR_OK" = xyes; then
#    AC_MSG_CHECKING([extractor version])
#    AC_CHECKING prints newline
#    AC_RUN_IFELSE(
#        [AC_LANG_PROGRAM(
#            [
#                #include <extractor.h>
#            ],
#            [
#                if(EXTRACTOR_VERSION < 0x00050200)
#                    return 1;
#            ]
#        )],
#        [
#            AC_MSG_RESULT([>= 0.5.2])
#            EXTRACTOR_GE_0_5_2=yes
#        ],
#        [
#            AC_MSG_RESULT([< 0.5.2])
#        ]
#    )
#fi
if test "x$EXTRACTOR_OK" = xyes; then
    LDFLAGS="$LDFLAGS -lextractor"
    AC_DEFINE([HAVE_EXTRACTOR], [1], [libextractor library presence])
#    if test "x$EXTRACTOR_GE_0_5_2" = xyes; then
#        AC_DEFINE(EXTRACTOR_GE_0_5_2)
#    fi
else
    if test "x$EXTRACTOR_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libextractor support)
    fi
fi


######## libexif

EXIF_OK=yes
#if test "x$EXTRACTOR_OK" = xyes -a "x$EXTRACTOR_GE_0_5_2" = xyes; then
#    EXIF_OK="disabled (obsoleted by libextractor >= v0.5.2)"
#fi
AC_ARG_ENABLE(libexif,
    [  --enable-libexif         libexif support],
    [
#        if test "x$EXTRACTOR_OK" != xyes -o "x$EXTRACTOR_GE_0_5_2" != xyes; then
            EXIF_EN=$enableval 
            if test "x$enableval" = xno; then
                EXIF_OK=disabled
            fi
#        fi
    ]
)
if test "x$EXIF_OK" = xyes; then
    AC_CHECK_HEADER(libexif/exif-data.h,
        [],
        [EXIF_OK=missing],
        /* checking for libexif/exif-data.h */
    )
fi
if test "x$EXIF_OK" = xyes; then
    AC_CHECK_HEADER(libexif/exif-content.h,
        [],
        [EXIF_OK=missing],
        /* checking for libexif/exif-content.h */
    )
fi
if test "x$EXIF_OK" = xyes; then
    # try 1 argument signature
    AC_MSG_CHECKING([exif_entry_get_value signature])
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [
                #include <libexif/exif-data.h>
                #include <libexif/exif-content.h>
            ],
            [
                (void)exif_entry_get_value(0);
            ]
        )],
        [
            AC_MSG_RESULT([requires 1 argument])
            EXIF_EGV_1=yes
        ],
        [
            # try 3 argument signature
            AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM(
                    [
                        #include <libexif/exif-data.h>
                        #include <libexif/exif-content.h>
                    ],
                    [
                        (void)exif_entry_get_value(0, 0, 0);
                    ]
                )],
                [
                    AC_MSG_RESULT([requires 3 arguments])
                    EXIF_EGV_3=yes
                ],
                [
                    AC_MSG_RESULT([unsupported, disabling])
                    EXIF_OK="disabled (unsupported version)"
                ]
            )
        ]
    )

fi
if test "x$EXIF_OK" = xyes; then
    LDFLAGS="$LDFLAGS -lexif"
    AC_DEFINE([HAVE_EXIF], [1], [EXIF library presence])
    if test "x$EXIF_EGV_1" = xyes; then
        AC_DEFINE([EXIF_EGV_1], [1], [exif_entry_get_value() has 1 parameter])
    fi
    if test "x$EXIF_EGV_3" = xyes; then
        AC_DEFINE([EXIF_EGV_3], [1], [exif_entry_get_value() has 3 parameters])
    fi
else
    if test "x$EXIF_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libexif support)
    fi
fi



######## exiv2
# TEMPORARILY DISABLED BECAUSE EXIV2 HAS MEMORY LEAKS

#EXIV2_OK="disabled (force with --enable-exiv2)"
#AC_ARG_ENABLE(exiv2,
#  [  --enable-exiv2           exiv2 support],
#  [
#     EXIV2_EN=$enableval 
#     if test "x$enableval" = xno; then
#        EXIV2_OK=disabled
#     fi
#     if test "x$enableval" = xyes; then
#        EXIV2_OK=yes
#     fi
#  ]
#)
#if test "x$EXIV2_OK" = xyes; then
#    AC_CHECK_HEADER(exiv2/exif.hpp,
#        [],
#        [EXIV2_OK=missing],
#        /* checking for exiv2/exif.h */
#    )
#fi
#if test "x$EXIV2_OK" = xyes; then
#   LDFLAGS="$LDFLAGS -lexiv2"
#   AC_DEFINE(HAVE_EXIV2)
#else
#    if test "x$EXIV2_EN" = xyes; then
#        AC_MSG_ERROR(unable to configure exiv2 support)
#    fi
#fi


LFS_OK="no"
if test "$_FILE_OFFSET_BITS" = "64"; then
    AC_DEFINE([HAVE_LFS], [1], 
              [Enable largefile support, if not defined keep old implementation])
    LFS_OK="yes"
fi
    
#AC_DEFINE(UPNP_HAVE_DEBUG, 1, [see upnpconfig.h])
#AC_DEFINE(DEBUG, 1, [Define to 1 to compile debug code])

###############
AC_CONFIG_FILES([
    Makefile
    build/Makefile
    doc/Makefile
    scripts/Makefile
    scripts/js/Makefile
    tombupnp/Makefile
    tombupnp/build/Makefile
    web/Makefile
    config/Makefile
])

AC_OUTPUT

echo
echo "======== CONFIGURATION SUMMARY ========="
echo "  sqlite3         : $SQLITE3_OK"
echo "  mysql           : $MYSQL_OK"
echo "  javascript      : $JS_OK"
echo "  libmagic        : $MAGIC_OK"
echo "  id3lib          : $ID3_OK"
echo "  libextractor    : $EXTRACTOR_OK"
echo "  libexif         : $EXIF_OK"
# echo "  libjpeg      : $LIBJPEG_OK"
# echo "  exiv2        : $EXIV2_OK"
echo

