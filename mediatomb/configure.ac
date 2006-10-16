AC_PREREQ(2.59)
AC_INIT([MediaTomb], [0.9.0_pre], [jin@mediatomb.org])
AM_CONFIG_HEADER([autoconfig.h tombupnp/upnp/inc/upnpconfig.h])
AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_SRCDIR([src/common.h])
AM_INIT_AUTOMAKE([1.9 -Wall])

###############################################################################
# The following block is derived from the configure.ac script from the 
# libupnp-1.3.1 package, we just set the version number to 0.1.0
# (C) Copyright 2005-2006 RИmi Turboult <r3mi@users.sourceforge.net>
upnpmaj=0
upnpmin=4
upnppatch=1
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
AC_HEADER_STAT
AC_LANG_C
AC_CANONICAL_HOST
   
DARWIN_OS=0
case $host in
    *-*-darwin*)
        DARWIN_OS=1
        ;;
esac

AC_CHECK_HEADERS([syslog.h stddef.h unistd.h arpa/inet.h fcntl.h], [],
                 [AC_MSG_ERROR(required header not found)]) 

AC_CHECK_HEADERS([limits.h netdb.h netinet/in.h stdlib.h string.h sys/file.h],
                 [],
                 [AC_MSG_ERROR(required header not found)]) 
AC_CHECK_HEADERS([sys/ioctl.h sys/socket.h sys/time.h sys/types.h pthread.h],
                 [],
                 [AC_MSG_ERROR(required header not found)]) 

AC_CHECK_HEADERS([langinfo.h],[],[AC_MSG_WARN(langinfo.h not found, will not be able to retrieve locale)])

AC_CHECK_HEADERS([sched.h],[],[])
AC_CHECK_FUNCS([sched_getparam sched_setparam],[],[])
    
AC_CHECK_HEADERS([getopt.h],
        [
            AC_CHECK_FUNCS([getopt_long],[],
                           [AC_MSG_WARN(
             getopt_long not found - all command line options disabled)
                           ])

        ],
        [
    AC_MSG_WARN(getopt.h not found - all command line options disabled)
        ])

AC_CACHE_SAVE

CPPFLAGS_SAVE="$CPPFLAGS"
CFLAGS_SAVE="$CFLAGS"
CXXFLAGS_SAVE="$CXXFLAGS"
LDFLAGS_SAVE="$LDFLAGS"

if test $DARWIN_OS -eq 1 ; then
    AC_ARG_WITH(opendarwin, 
        AC_HELP_STRING([--with-opendarwin=DIR], [opendarwin installation directoryi (default: /opt/local)]),
        [DARWIN_DIR=$withval],[DARWIN_DIR="/opt/local"]
    )
fi

AC_CHECK_HEADER([iconv.h],
        [
            AC_CHECK_FUNCS([iconv],[],
                [
                    AC_CHECK_LIB(iconv, iconv, [], 
                        [
                            AC_CHECK_LIB(iconv, libiconv, [],
                                [AC_MSG_ERROR(shit not found)]
                            )
                        ])
                ])
        ],
        [
            AC_CHECK_HEADER([/usr/local/include/iconv.h],
                [
                    ICONV_CXXFLAGS="-I/usr/local/include"
                    AC_SUBST(ICONV_CXXFLAGS)
                ],
                [
                    AC_CHECK_FUNCS([iconv],[],[AC_MSG_ERROR(iconv library not found on your system)])
                ])
        ])
        
CFLAGS="$CFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"

AC_CHECK_HEADERS(execinfo.h,[],[])

AC_CHECK_TYPES([time_t], [], [], [#include <sys/types.h>])

AC_CACHE_SAVE

AC_SYS_LARGEFILE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
#AC_CHECK_TYPES([ssize_t], [], [], [#include <sys/types.h>])
AC_FUNC_FSEEKO


if test "$ac_cv_func_fseeko" != yes; then
    AC_MSG_WARN([fseeko has not been found on your system, will use fseek instead.])
    AC_MSG_WARN([This has not been tested, please report if you have any problems.])
    AC_DEFINE([fseeko], [fseek], [fseeko not present])
fi

AC_CHECK_SIZEOF(off_t, 4)
AC_CHECK_SIZEOF(size_t, 4)
AC_CHECK_SIZEOF(time_t, 4)
AC_CHECK_FUNCS(strtoll nl_langinfo,[],[])
AC_CHECK_FUNCS(backtrace backtrace_symbols,[],[])
# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIGNAL
AC_TYPE_PID_T
AC_TYPE_SIZE_T
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

AC_LANG_CPLUSPLUS

AC_CHECK_FUNCS([gethostname gettimeofday localtime_r memmove memset],
               [],
               [AC_MSG_ERROR(required library function not found)])
AC_CHECK_FUNCS([regcomp select strcasecmp strchr strdup strerror strncasecmp],
               [],
               [AC_MSG_ERROR(required library function not found)])
AC_CHECK_FUNCS([strrchr strstr strtol strtoul uname pthread_mutex_lock], [],
               [AC_MSG_ERROR(required library function not found)])

AC_CHECK_FUNCS([sigaction sigprocmask pthread_self if_nameindex],[],
               [AC_MSG_ERROR(required library function not found)]) 
 
AC_CHECK_FUNCS([if_freenameindex ioctl],[],
               [AC_MSG_ERROR(required library function not found)]) 
               
AC_CACHE_SAVE

AC_CHECK_FUNCS([inet_ntoa],
               [],
               [
                    AC_CHECK_LIB(nsl, inet_ntoa,
                                 [],
                                 [
                                    AC_MSG_ERROR(required library function not found)
                                 ])
               ])

AC_CHECK_FUNCS([socket],
               [],
               [AC_CHECK_LIB(socket, socket,
                                 [],
                                 [
                                    AC_MSG_ERROR(required library function not found)
                                 ])
               ])
               


AC_CHECK_FUNCS([gethostbyname],
               [],
               [
                    AC_CHECK_LIB(nsl, gethostbyname, 
                                 [],
                                 [
                                    AC_MSG_ERROR(required library function not found)
                                 ]) 
               ])

if test $DARWIN_OS -eq 1 ; then
        AC_MSG_NOTICE([You are running OSX, assuming threadsafe gethostbyname version])
fi

AC_CHECK_FUNCS([gethostbyname_r],[],
        [
            AC_CHECK_HEADERS([lwres/netdb.h],
                [
                    AC_CHECK_LIB(lwres, lwres_gethostbyname_r,[],[AC_MSG_ERROR(required library function not found)])
                ],
                [
                    if test $DARWIN_OS -eq 0 ; then
                        AC_MSG_ERROR([required header not found])
                    fi
                ])
        ])

# this is not optional, we need the device and the internal webserver
AC_DEFINE([UPNP_HAVE_DEVICE], [1], [Compile device API])
AC_DEFINE([UPNP_HAVE_WEBSERVER], [1], [Compile internal web server])    

AC_CHECK_LIB(pthread, pthread_mutex_lock,[], [AC_MSG_ERROR(required library function not found)])    

   
AC_CACHE_SAVE

######### SQLite3
AC_ARG_ENABLE(sqlite3, 
        AC_HELP_STRING([--enable-sqlite3], [compile with sqlite3 support (default: yes)]),
        [
            SQLITE3_EN=$enableval
            if test "x$enableval" = xno; then
                SQLITE3_OK=disabled
            else
                SQLITE3_OK=yes
            fi
        ],
        [
            SQLITE3_OK=yes      
        ]
)
if test "x$SQLITE3_OK" = xyes; then
    AC_CHECK_HEADER(sqlite3.h, [], 
            [
                if test $DARWIN_OS -eq 1 ; then
                    AC_CHECK_HEADER( $DARWIN_DIR/include/sqlite3.h, 
                        [
                            SQLITE3_CXXFLAGS="-I$DARWIN_DIR/include/"
                            AC_SUBST(SQLITE3_CXXFLAGS)
                        ],
                        [SQLITE3_OK=missing]
                    )
                else
                    SQLITE3_OK=missing
                fi
            ])
fi

if test "x$SQLITE3_OK" = xyes; then
    AC_CHECK_LIB(sqlite3, sqlite3_open,
        [],
        [
            if test $DARWIN_OS -eq 1 ; then
                LDFLAGS="-L$DARWIN_DIR/lib -lsqlite3"
                unset ac_cv_lib_sqlite3_sqlite3_open
                AC_CHECK_LIB(sqlite3, sqlite3_open,
                    [
                        SQLITE3_LIBS="-L$DARWIN_DIR/lib -lsqlite3"
                        AC_SUBST(SQLITE3_LIBS)
                    ],
                    [SQLITE3_OK=missing]
                )
            else
                SQLITE3_OK=missing
            fi
        ]
    )
fi
if test "x$SQLITE3_OK" = xyes; then
    AC_PATH_PROG(sqlite3prog, sqlite3, no)
    if test "$sqlite3prog" = no; then
        if test $DARWIN_OS -eq 1 ; then
            unset ac_cv_path_sqlite3prog
            AC_PATH_PROG(sqlite3prog, sqlite3, no, $DARWIN_DIR/bin)
            if test "$sqlite3prog" = no; then
                AC_MSG_ERROR([sqlite3 command line client not found])
                SQLITE3_OK=missing
            fi
        else
            AC_MSG_ERROR([sqlite3 command line client not found])
	        SQLITE3_OK=missing
        fi
    fi
fi
if test "x$SQLITE3_OK" = xyes; then
    AC_DEFINE([HAVE_SQLITE3], [1], [sqlite3 library presence])
else
    if test "x$SQLITE3_EN" = xyes; then
        AC_MSG_ERROR(unable to configure sqlite3 support)
    fi
fi

LDFLAGS="$LDFLAGS_SAVE"
########## MySQL

AC_ARG_ENABLE(mysql, 
              AC_HELP_STRING([--enable-mysql], [compile with mysql support (default: yes)]),
              [
                MYSQL_EN=$enableval 
                if test "x$enableval" = xno; then
                    MYSQL_OK=disabled
                else
                    MYSQL_OK=yes
                fi
              ],
              [
                MYSQL_OK=yes
              ]
)
if test "x$MYSQL_OK" = xyes; then
    AC_PATH_PROG(mysqlconfig, mysql_config, no)
    if test "x$mysqlconfig" = xno; then
        MYSQL_OK=missing
        AC_MSG_RESULT([mysql_config script not found])
    else
        AC_MSG_CHECKING(mysql_config version)
        MYSQL_VERSION_MAJOR=`${mysqlconfig} --version | sed 's/\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\1/'`
        MYSQL_VERSION_MINOR=`${mysqlconfig} --version | sed 's/\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\2/'`
        
        if test $MYSQL_VERSION_MAJOR -le 3 ; then
                AC_MSG_WARN(MySQL version too old)
                MYSQL_OK=missing
        else
            if test $MYSQL_VERSION_MAJOR -gt 3 ; then
        
                AC_MSG_CHECKING(mysql includes)
                MYSQL_CFLAGS=`${mysqlconfig} --include | sed s/\'//g`
                AC_MSG_RESULT($MYSQL_CFLAGS)

                AC_MSG_CHECKING(mysql libraries)
                MYSQL_LIBS=`${mysqlconfig} --libs_r | sed s/\'//g`
                AC_MSG_RESULT($MYSQL_LIBS)
            else
                AC_MSG_ERROR(Unsupported mysql version)
                MYSQL_OK=missing
            fi
        fi

        CXXFLAGS="$CXXFLAGS $MYSQL_CFLAGS"
        LDFLAGS="$LDFLAGS $MYSQL_LIBS"
    fi
fi
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_HEADER(mysql.h,
        [],
        [AC_MSG_RESULT(mysql.h failed) MYSQL_OK=missing],
        /* checking for mysql.h */
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_FUNCS(mysql_init,
        [],
        [MYSQL_OK=missing AC_MSG_RESULT(mysqlclient link failed)]
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_DEFINE([HAVE_MYSQL], [1], [MySLQ library presence])
    AC_CHECK_FUNCS([mysql_stmt_init],[],[])
else
    if test "x$MYSQL_EN" = xyes; then
        AC_MSG_ERROR(unable to configure mysql support)
    fi
fi

AC_SUBST(MYSQL_LIBS)
AC_SUBST(MYSQL_CFLAGS)

CXXFLAGS="$CXXFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"

#########  check if at least one database available

if test "x$SQLITE3_OK" != xyes; then
    if test "x$MYSQL_OK" != xyes; then
        AC_MSG_ERROR(Support of at least one of mysql or sqlite3 must be configured)
    fi
fi


######### javascript

AC_ARG_ENABLE(libjs,
              AC_HELP_STRING([--enable-libjs], [compile with spidermonkey support (default: yes)]),
              [
                JS_EN=$enableval 
                if test "x$enableval" = xno; then
                    JS_OK=disabled
                else
                    JS_OK=yes
                fi
              ],
              [
                JS_OK=yes
              ]
)
if test "x$JS_OK" = xyes; then
    AC_CHECK_HEADER(jsapi.h,
        [],
        [
            CXXFLAGS="-I/usr/include/smjs"
            
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
                    JS_CXXFLAGS="-I/usr/include/smjs"
                    AC_MSG_RESULT([found under /usr/include/smjs])
                ],
                [
                    CXXFLAGS="-I/usr/include/js"
            
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
                            JS_CXXFLAGS="-I/usr/include/js"
                            AC_MSG_RESULT([found under /usr/include/js])
                        ],
                        [
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
        [],
        [
            AC_CHECK_LIB(smjs, JS_NewObject,
                [],
                [JS_OK=missing]
            )
        ]
    )
fi
if test "x$JS_OK" = xyes; then
   AC_DEFINE([HAVE_JS], [1], [libjs presence])
else
    if test "x$JS_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libjs support)
    fi
fi

AC_SUBST(JS_CXXFLAGS)

CXXFLAGS="$CXXFLAGS_SAVE"

########### libmagic

AC_ARG_ENABLE(libmagic,
        AC_HELP_STRING([--enable-libmagic], [compile with filemagic support (default: yes)]),
        [
            MAGIC_EN=$enableval 
            if test "x$enableval" = xno; then
                MAGIC_OK=disabled
            else
                MAGIC_OK=yes
            fi
        ],
        [
            MAGIC_OK=yes
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
    AC_CHECK_LIB(magic, magic_load,
            [],
            [MAGIC_OK=missing])
fi

if test "x$MAGIC_OK" = xyes; then
    AC_DEFINE([HAVE_MAGIC], [1], [filemagic library presence])
else
    if test "x$MAGIC_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libmagic support)
    fi
fi


######### taglib

TAGLIB_REQUESTED=no
ID3_REQUESTED=no
AC_ARG_ENABLE(taglib,
        AC_HELP_STRING([--enable-taglib], [compile with taglib support (default: yes, preferred over id3lib)]),
   [
      TAGLIB_EN=$enableval
      if test "x$enableval" = xno; then
        TAGLIB_OK=disabled
      else
        TAGLIB_REQUESTED=yes
        TAGLIB_OK=yes
      fi
   ],
   [
        TAGLIB_OK=yes
   ]

)

AC_ARG_ENABLE(id3lib,
        AC_HELP_STRING([--enable-id3lib], [compile with taglib support (default: yes, if taglib not present)]),
  [
     ID3_EN=$enableval 
     if test "x$enableval" = xno; then
        ID3_OK=disabled
     else
        ID3_OK=yes
        ID3_REQUESTED=yes
     fi
  ],
  [
    ID3_OK=yes
  ]
)

if ((test "x$TAGLIB_REQUESTED" = xno) && (test "x$ID3_REQUESTED" = xyes)) ; then
    TAGLIB_OK=disabled
    AC_MSG_WARN([taglib disabled])
elif ((test "x$TAGLIB_REQUESTED" = xyes) && (test "x$ID3_REQUESTED" = xno)) ; then
    ID3_OK=disabled
    AC_MSG_WARN([id3lib disabled])
elif ((test "x$TAGLIB_REQUESTED" = xyes) && (test "x$ID3_REQUESTED" = xyes)) ; then
    AC_MSG_ERROR([Please select either taglib or id3lib, but not both.])
fi

if test "x$TAGLIB_OK" = xyes; then
    AC_PATH_PROG(taglibconfig, taglib-config, no)
    if test "x$taglibconfig" = xno; then
        AC_MSG_RESULT([taglib-config not found, please install the taglib-devel package])
        TAGLIB_OK=missing
    else
        AC_MSG_CHECKING(taglib cflags)
        TAGLIB_CFLAGS=`${taglibconfig} --cflags`
        AC_MSG_RESULT($TAGLIB_CFLAGS)

        AC_MSG_CHECKING(taglib libraries)
        TAGLIB_LIBS=`${taglibconfig} --libs`
        AC_MSG_RESULT($TAGLIB_LIBS)

    fi
fi

if test "x$TAGLIB_OK" = xyes; then
    CPPFLAGS="$CPPFLAGS $TAGLIB_CFLAGS"

    AC_CHECK_HEADERS([taglib.h fileref.h tag.h audioproperties.h tstring.h],
        [],
        [TAGLIB_OK=missing]
    )
fi

if test "x$TAGLIB_OK" = xyes; then
    AC_CHECK_LIB(tag, main,[],
        [TAGLIB_OK=missing]
    )
fi

if test "x$TAGLIB_OK" = xyes; then
    AC_DEFINE([HAVE_TAGLIB], [1], [taglib library presence])
        ID3_OK=disabled
else
    if test "x$TAGLIB_EN" = xyes; then
        AC_MSG_ERROR(unable to configure taglib support)
    fi
fi

AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)

CPPFLAGS="$CPPFLAGS_SAVE"

######### id3tag

#AC_ARG_ENABLE(id3lib,
#  [  --enable-id3lib          id3lib support],
#  [
#     ID3_EN=$enableval 
#     if test "x$enableval" = xno; then
#        ID3_OK=disabled
#     fi
#  ]
#)

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
    AC_CHECK_LIB(z,compress,
            [AC_CHECK_LIB(id3,main,[],[ID3_OK=missing],[-lz])],
            [AC_CHECK_LIB(id3,main,[],[ID3_OK=missing],[])],
            []
    )
fi

if test "x$ID3_OK" = xyes; then
   AC_DEFINE([HAVE_ID3], [1], [id3lib presence])
else
    if test "x$ID3_EN" = xyes; then
        if test "x$TAGLIB_OK" = xyes; then
            AC_MSG_ERROR([Please select either taglib or id3lib, but not both.])
        else
            AC_MSG_ERROR(unable to configure id3lib support)
        fi
    fi
fi


######## extractor

AC_ARG_ENABLE(libextractor,
        AC_HELP_STRING([--enable-libextractor], [compile with libextractor support (default: no)]),
  [
     EXTRACTOR_EN=$enableval 
     if test "x$enableval" = xno; then
        EXTRACTOR_OK=disabled
     else
        EXTRACTOR_OK=yes
     fi
  ],
  [
    EXTRACTOR_OK=disabled
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
    AC_CHECK_LIB(extractor, EXTRACTOR_getKeywords,
        [],
        [ID3_OK=missing],
        []
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

#if test "x$EXTRACTOR_OK" = xyes -a "x$EXTRACTOR_GE_0_5_2" = xyes; then
#    EXIF_OK="disabled (obsoleted by libextractor >= v0.5.2)"
#fi
AC_ARG_ENABLE(libexif,
        AC_HELP_STRING([--enable-libexif], [compile with libexif support (default: yes)]),
    [
#        if test "x$EXTRACTOR_OK" != xyes -o "x$EXTRACTOR_GE_0_5_2" != xyes; then
            EXIF_EN=$enableval 
            if test "x$enableval" = xno; then
                EXIF_OK=disabled
            else
                EXIF_OK=yes
            fi
#        fi
    ],
    [
        EXIF_OK=yes
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
    AC_CHECK_LIB(exif, exif_data_new_from_file,
        [],
        [EXIF_OK=missing],
        []
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


AC_ARG_ENABLE(tombdebug,
        AC_HELP_STRING([--enable-tombdebug], [enable debug output for MediaTomb (default: no)]),
  [
     LOGDEBUG_EN=$enableval 
     if test "x$enableval" = xno; then
        LOGDEBUG_OK=disabled
     else
        LOGDEBUG_OK=yes
     fi
  ],
  [
     LOGDEBUG_OK=disabled
  ]
)
if test "x$LOGDEBUG_OK" = xyes; then
    AC_DEFINE(LOG_TOMBDEBUG, 1, [if defined compile with debug log output])
fi


AC_ARG_ENABLE(upnpdebug,
        AC_HELP_STRING([--enable-upnpdebug], [enable debug output for TombUPnP (default: no)]),
  [
     UPNPDEBUG_EN=$enableval 
     if test "x$enableval" = xno; then
        UPNPDEBUG_OK=disabled
     else
        UPNPDEBUG_OK=yes
     fi
  ],
  [
    UPNPDEBUG_OK=disabled
  ]
)

if test "x$UPNPDEBUG_OK" = xyes; then
    AC_DEFINE(UPNP_HAVE_DEBUG, 1, [see upnpconfig.h])
    AC_DEFINE(DEBUG, 1, [Define to 1 to compile debug code])
fi

AC_ARG_ENABLE(log,
        AC_HELP_STRING([--disable-log], [disable all output from MediaTomb (default: no)]),
  [
     LOG_OK_EN=$enableval 
     if test "x$enableval" = xno; then
        LOG_OK=disabled
     else
        LOG_OK=yes
     fi
  ],
  [
      LOG_OK=yes
  ]
)

if test "x$LOG_OK" = xyes; then
    AC_DEFINE(LOG_ENABLED, 1, [if defined compile with log output])
fi

eval PACKAGE_DATADIR="$datadir/$PACKAGE"
AC_DEFINE_UNQUOTED(PACKAGE_DATADIR, "$PACKAGE_DATADIR", [MediaTomb data directory])

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
echo "CONFIGURATION SUMMARY ----"
#                        disabled
echo
echo "sqlite3       : $SQLITE3_OK"
echo "mysql         : $MYSQL_OK"
echo "libjs         : $JS_OK"
echo "libmagic      : $MAGIC_OK"
echo "libexif       : $EXIF_OK"
echo "taglib        : $TAGLIB_OK"
echo "id3lib        : $ID3_OK"
echo "libextractor  : $EXTRACTOR_OK"

if test "$LOG_OK" != "yes"; then
    echo "log output    : $LOG_OK"
fi

if test "$LOGDEBUG_OK" != "disabled"; then
    echo "debug log     : $LOGDEBUG_OK"
fi

if test "$UPNPDEBUG_OK" != "disabled"; then
    echo "upnpdebug     : $UPNPDEBUG_OK"
fi
# echo "  libjpeg     : $LIBJPEG_OK"
# echo "  exiv2       : $EXIV2_OK"
echo

