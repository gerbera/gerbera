#*MT*
#    
#    MediaTomb - http://www.mediatomb.org/
#    
#    configure.ac - this file is part of MediaTomb.
#    
#    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
#                       Sergey Bostandzhyan <jin@mediatomb.org>
#    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
#                       Sergey Bostandzhyan <jin@mediatomb.org>,
#                       Leonhard Wimmer <leo@mediatomb.org>
#    
#    MediaTomb is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#    
#    MediaTomb is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#    
#    You should have received a copy of the GNU General Public License
#    along with MediaTomb; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#    
#    $Id$
#
#*MT*

AC_PREREQ(2.59)
AC_INIT([MediaTomb], [0.9.0_pre], [jin@mediatomb.org])
AM_CONFIG_HEADER([autoconfig.h tombupnp/upnp/inc/upnpconfig.h])
AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_SRCDIR([src/common.h])
AM_INIT_AUTOMAKE([1.9 -Wall])


SEARCH_DIR="/usr/local"
   
DARWIN_OS=0

AC_CANONICAL_HOST
case $host in
    *-*-darwin*)
        DARWIN_OS=1
        SEARCH_DIR="/opt/local"
        ;;
esac

AC_ARG_WITH(search, 
        AC_HELP_STRING([--with-search=DIR], [Additionally search for packages in DIR]),
        [
            SEARCH_DIR=$withval
            AC_MSG_NOTICE([Will also search for packages in $SEARCH_DIR])
        ]
)

SEARCH_DIR_HEADERS="$SEARCH_DIR/include"
SEARCH_DIR_LIBS="$SEARCH_DIR/lib"
SEARCH_DIR_PROGS="$SEARCH_DIR/bin"
    
# per package header and library search paths
AC_ARG_WITH(sqlite3-h, 
        AC_HELP_STRING([--with-sqlite3-h=DIR], [search for sqlite3 headers only in DIR]),
        [
            SQLITE3_SEARCH_HEADERS="$withval"
        ]
)

AC_ARG_WITH(sqlite3-libs, 
        AC_HELP_STRING([--with-sqlite3-libs=DIR], [search for sqlite3 libraries only in DIR]),
        [
            SQLITE3_SEARCH_LIBS="$withval"
        ]
)

# mysql_config should deal with everything
#
#AC_ARG_WITH(mysql-h, 
#        AC_HELP_STRING([--with-mysql-h=DIR], [search for mysql headers only in DIR]),
#        [
#            MYSQL_SEARCH_HEADERS="$withval"
#            AC_MSG_NOTICE([Will search for mysql headers in $withval])
#        ]
#)
#
#AC_ARG_WITH(mysql-libs, 
#        AC_HELP_STRING([--with-mysql-libs=DIR], [search for mysql libraries in DIR]),
#        [
#            MYSLQ_SEARCH_LIBS="$withval"
#            AC_MSG_NOTICE([Will search for mysql libraries in $withval])
#        ]
#)

AC_ARG_WITH(mysql-cfg,
        AC_HELP_STRING([--with-mysql-cfg=mysql_config], [absolute path/name of mysql_config]),
        [
            MYSQL_SEARCH_CONFIG="$withval"
        ]
)

AC_ARG_WITH(js-h,
        AC_HELP_STRING([--with-js-h=DIR], [search for js (spidermonkey) headers in DIR]),
        [
            JS_SEARCH_HEADERS="$withval"
        ]
)

AC_ARG_WITH(js-libs,
        AC_HELP_STRING([--with-js-libs=DIR], [search for js (spidermonkey) libraries in DIR]),
        [
            JS_SEARCH_LIBS="$withval"
        ]
)

AC_ARG_WITH(magic-h,
        AC_HELP_STRING([--with-magic-h=DIR], [search for filemagic headers in DIR]),
        [
            MAGIC_SEARCH_HEADERS="$withval"
        ]
)

AC_ARG_WITH(magic-libs,
        AC_HELP_STRING([--with-magic-libs=DIR], [search for filemagic headers in DIR]),
        [
            MAGIC_SEARCH_LIBS="$withval"
        ]
)


AC_ARG_WITH(exif-h,
        AC_HELP_STRING([--with-exif-h=DIR], [search for libexif headers in DIR]),
        [
            EXIF_SEARCH_HEADERS="$withval"
            AC_MSG_NOTICE([Will search for libexif headers in $withval])
        ]
)

AC_ARG_WITH(exif-libs,
        AC_HELP_STRING([--with-exif-libs=DIR], [search for libexif libraries in DIR]),
        [
            EXIF_SEARCH_LIBS="$withval"
            AC_MSG_NOTICE([Will search for libexif libraries in $withval])
        ]
)

#AC_ARG_WITH(taglib-h,
#        AC_HELP_STRING([--with-taglib-h=DIR], [search for taglib headers in DIR]),
#        [
#            TAGLIB_SEARCH_HEADERS="$withval"
#            AC_MSG_NOTICE([Will search for taglib headers in $withval])
#        ]
#)
#
#AC_ARG_WITH(taglib-libs,
#        AC_HELP_STRING([--with-taglib-libs=DIR], [search for taglib libraries in DIR]),
#        [
#            TAGLIB_SEARCH_LIBS="$withval"
#            AC_MSG_NOTICE([Will search for libexif libraries in $withval])
#        ]
#)

AC_ARG_WITH(taglib-cfg,
        AC_HELP_STRING([--with-taglib-cfg=taglib-config], [absolute path/name of taglib-config]),
        [
            TAGLIB_SEARCH_CONFIG="$withval"
            AC_MSG_NOTICE([Will search for taglib-config in $withval])
        ]
)

AC_ARG_WITH(id3lib-h,
        AC_HELP_STRING([--with-id3lib-h=DIR], [search for id3lib headers in DIR]),
        [
            ID3_SEARCH_HEADERS="$withval"
            AC_MSG_NOTICE([Will search for id3lib headers in $withval])
        ]
)

AC_ARG_WITH(id3lib-libs,
        AC_HELP_STRING([--with-id3lib-libs=DIR], [search for id3lib libraries in DIR]),
        [
            ID3_SEARCH_LIBS="$withval"
            AC_MSG_NOTICE([Will search for id3 libraries in $withval])
        ]
)

AC_ARG_WITH(zlib-h,
        AC_HELP_STRING([--with-zlib-h=DIR], [search for zlib headers in DIR]),
        [
            Z_SEARCH_HEADERS="$withval"
        ]
)

AC_ARG_WITH(zlib-libs,
        AC_HELP_STRING([--with-zlib-libs=DIR], [search for zlib libraries in DIR]),
        [
            Z_SEARCH_LIBS="$withval"
        ]
)

AC_ARG_WITH(iconv-h,
        AC_HELP_STRING([--with-iconv-h=DIR], [search for iconv headers in DIR]),
        [
            ICONV_SEARCH_HEADERS="$withval"
            AC_MSG_NOTICE([Will search for iconv headers in $withval])
        ]
)

AC_ARG_WITH(iconv-libs,
        AC_HELP_STRING([--with-iconv-libs=DIR], [search for iconv libraries in DIR]),
        [
            ICONV_SEARCH_LIBS="$withval"
            AC_MSG_NOTICE([Will search for iconv libraries in $withval])
        ]
)

AC_ARG_WITH(extractor-h,
        AC_HELP_STRING([--with-extractor-h=DIR], [search for extractor headers in DIR]),
        [
            EXTRACTOR_SEARCH_HEADERS="$withval"
            AC_MSG_NOTICE([Will search for extractor headers in $withval])
        ]
)

AC_ARG_WITH(extractor-libs,
        AC_HELP_STRING([--with-extractor-libs=DIR], [search for extractor libraries in DIR]),
        [
            EXTRACTOR_SEARCH_LIBS="$withval"
            AC_MSG_NOTICE([Will search for extractor libraries in $withval])
        ]
)

STATIC="no"

AC_ARG_ENABLE(static, 
        AC_HELP_STRING([--enable-static], [build a static version of MediaTomb (default: no)]),
        [
            if test "x$enableval" = xyes; then
                CFLAGS="$CFLAGS -static"
                CXXFLAGS="$CXXFLAGS -static"
                AC_MSG_NOTICE([Building a static executable.])
                STATIC="yes"
            fi
        ]
)

AUTOCREATE=1
AUTOCREATE_REQUESTED=0
AC_ARG_ENABLE(db-autocreate,
        AC_HELP_STRING([--enable-db-autocreate], [enable automatic creation of the media database (default: yes)]),
        [
            if test "x$enableval" = xno; then
                AUTOCREATE=0
            else
                AUTOCREATE_REQUESTED=1
                AUTOCREATE=1
            fi
        ],
        [
            AUTOCREATE=1
        ]
)

###############################################################################
# The following block is derived from the configure.ac script from the 
# libupnp-1.3.1 package
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

ICONV_CXXFLAGS=
ICONV_LIBS=

if test -n "$ICONV_SEARCH_HEADERS"; then
AC_CHECK_HEADER([$ICONV_SEARCH_HEADERS/iconv.h],
        [
            ICONV_CXXFLAGS="-I$ICONV_SEARCH_HEADERS"
        ],
        [
            AC_MSG_ERROR(iconv.h not found in requested location $ICONV_SEARCH_HEADERS)
        ]
)
        
else
AC_CHECK_HEADER([iconv.h],
        [ICONV_OK=yes],
        [
            unset ac_cv_header_iconv_h
            AC_CHECK_HEADER([/usr/local/include/iconv.h],
                [
                    ICONV_CXXFLAGS="-I/usr/local/include"
                    if test -z "$ICONV_SEARCH_LIBS"; then
                        LDFLAGS="-L/usr/local/lib"
                    fi
                ],
                [
                    unset ac_cv_header_iconv_h
                    AC_CHECK_HEADER([$SEARCH_DIR_HEADERS/iconv.h],
                        [
                            ICONV_CXXFLAGS="-I$SEARCH_DIR_HEADERS"
                            if test -z "$ICONV_SEARCH_LIBS"; then
                                LDFLAGS="-L$SEARCH_DIR_LIBS"
                            fi
                        ],
                        [AC_MSG_ERROR(required header iconv.h not found on your system)]
                    )
                ]
            )
        ]
)
fi

if test -n "$ICONV_SEARCH_LIBS"; then
    LDFLAGS="-L$ICONV_SEARCH_LIBS"
    AC_CHECK_FUNC([iconv],
        [
            ICONV_LIBS="-L$ICONV_SEARCH_LIBS"
        ],
        [
            LDFLAGS="-L$ICONV_SEARCH_LIBS -liconv"
            AC_CHECK_LIB(iconv, iconv,
                [
                    ICONV_LIBS="-L$ICONV_SEARCH_LIBS -liconv"
                ],
                [
                    AC_CHECK_LIB(iconv, libiconv,
                        [
                            ICONV_LIBS="-L$ICONV_SEARCH_LIBS -liconv"
                        ],
                        [AC_MSG_ERROR(required library iconv not found in requested location $ICONV_SEARCH_LIBS)]
                    )
                ]
            ) 
        ]
    ) 

else
    AC_CHECK_FUNC([iconv],
        [
            ICONV_LIBS="$LDFLAGS"
        ],
        [
        
            LDFLAGS="$LDFLAGS -liconv"
            AC_CHECK_LIB(iconv, iconv, 
                [
                    ICONV_LIBS="$LDFLAGS"
                ], 
                [
                    AC_CHECK_LIB(iconv, libiconv,
                        [
                            ICONV_LIBS="$LDFLAGS"
                        ],
                        [AC_MSG_ERROR(required library iconv not found on your system)]
                    )
                ]
            )
        ]
    )
fi


AC_SUBST(ICONV_CXXFLAGS)
AC_SUBST(ICONV_LIBS)

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

AC_ARG_ENABLE(rpl-malloc, 
        AC_HELP_STRING([--disable-rpl-malloc], [disable redefinition of malloc to rpl_malloc and realloc to rpl_realloc. Use this if you are sure that you are compiling for the GNU C library (default: enabled)]),
        [
            if test "x$enableval" = xno; then
                ac_cv_func_malloc_0_nonnull=yes
                ac_cv_func_realloc_0_nonnull=yes
                AC_MSG_NOTICE([Disabling redefinition of malloc to rpl_malloc])
                AC_MSG_NOTICE([Disabling redefinition of realloc to rpl_realloc])
                AC_MSG_WARN([Use this feature only if you are sure that you are building for GNU C!])
            fi
        ]
)


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
AC_CHECK_FUNCS([strrchr strstr strtol strtoul uname], [],
               [AC_MSG_ERROR(required library function not found)])

AC_CHECK_FUNCS([sigaction sigprocmask],[],
               [AC_MSG_ERROR(required library function not found)]) 
 
AC_CHECK_FUNCS([ioctl],[],
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

AC_CHECK_FUNCS([if_nameindex],[],
        [
            AC_CHECK_LIB(nsl, if_nameindex,[],
                [AC_MSG_ERROR(required library function not found)]
            )
        ]
)

AC_CHECK_FUNCS([if_freenameindex],[],
        [
            AC_CHECK_LIB(nsl, if_freenameindex,[],
                [AC_MSG_ERROR(required library function not found)]
            )
        ]
)


# this is not optional, we need the device and the internal webserver
AC_DEFINE([UPNP_HAVE_DEVICE], [1], [Compile device API])
AC_DEFINE([UPNP_HAVE_WEBSERVER], [1], [Compile internal web server])    

AC_CHECK_LIB(pthread, pthread_mutex_lock,[], [AC_MSG_ERROR(required library function not found)])    

# check librt stuff - clock_gettime
AC_CHECK_FUNC([clock_gettime],
        [
            AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [clock_gettime presense])
        ],
        [
            AC_CHECK_LIB(rt, clock_gettime,
                [
                    RT_LIBS="-lrt"
                    AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [clock_gettime presense])
                ],
                [
                    LDFLAGS="-L$SEARCH_DIR_LIBS"
                    unset ac_cv_lib_rt_clock_gettime
                    AC_CHECK_LIB(rt, clock_gettime,
                        [
                            RT_LIBS="-L$SEARCH_DIR_LIBS -lrt"
                            AC_DEFINE([HAVE_CLOCK_GETTIME], [1], [clock_gettime presense])
                        ]
                    )
                ]
            )
        ]
)

LDFLAGS="$LDFLAGS_SAVE"
AC_SUBST(RT_LIBS)

    
ZLIB_OK=yes
Z_CXXFLAGS=
Z_LIBS=
if test -n "$Z_SEARCH_HEADERS" ; then
    AC_MSG_NOTICE([Will search for zlib headers in $Z_SEARCH_HEADERS])
    AC_CHECK_HEADER([$Z_SEARCH_HEADERS/zlib.h],
            [
                Z_CXXFLAGS="-I$Z_SEARCH_HEADERS"
            ],
            [
                AC_MSG_ERROR([zlib.h not found in requested location $Z_SEARCH_HEADERS])
            ]
    )
else
   AC_CHECK_HEADER([zlib.h], [], 
           [
                unset ac_cv_header_zlib_h
                AC_CHECK_HEADER([$SEARCH_DIR_HEADERS/zlib.h],
                    [
                        Z_CXXFLAGS="-I$SEARCH_DIR_HEADERS"
                    ],
                    [
                        ZLIB_OK=missing
                    ])
           ])
fi

if test -n "$Z_SEARCH_LIBS" ; then     
    AC_MSG_NOTICE([Will search for zlib libraries in $Z_SEARCH_LIBS])
    LDFLAGS="-L$Z_SEARCH_LIBS"
    AC_CHECK_LIB(z, uncompress,
            [
                Z_LIBS="-L$Z_SEARCH_LIBS -lz"
            ],
            [
                AC_MSG_ERROR([zlib libraries not found in requested location $Z_SEARCH_LIBS])
            ]
    )
else
    AC_CHECK_LIB(z,uncompress,
            [],
            [
                LDFLAGS="-L$SEARCH_DIR_LIBS"
                unset ac_cv_lib_z_uncompress
                AC_CHECK_LIB(z, uncompress,
                    [
                        Z_LIBS="-L$SEARCH_DIR_LIBS -lz"
                        ZLIB_OK=missing
                    ]
                )
                    
            ]
    )
fi

if test "x$ZLIB_OK" = xyes; then
    AC_SUBST(Z_CXXFLAGS)
    AC_SUBST(Z_LIBS)

    if test $AUTOCREATE -eq 1; then
        AC_DEFINE([AUTO_CREATE_DATABASE], [1], [create database automatically if it does not exist])
    fi
else
    if test $AUTOCREATE_REQUESTED -eq 1; then
        AC_MSG_ERROR([Automatic database creation not possible due to missing zlib headers/libraries])
    fi
fi

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
    if test -n "$SQLITE3_SEARCH_HEADERS"; then
        AC_MSG_NOTICE([Will search for sqlite3 headers in $withval])
        AC_CHECK_HEADER([$SQLITE3_SEARCH_HEADERS/sqlite3.h],
                [
                    SQLITE3_CXXFLAGS="-I$SQLITE3_SEARCH_HEADERS"
                    SQLITE3_OK=yes
                ],
                [
                    AC_MSG_ERROR(sqlite3.h not found in requested locatoin $SQLITE3_SEARCH_HEADERS)
                ]
        )
    else
    
    AC_CHECK_HEADER(sqlite3.h, [], 
            [
                SQLITE3_OK=missing
                for sqlite3_path in $SEARCH_DIR_HEADERS; do
                    if test "x$ac_cv_header_sqlite3_h" != xyes ; then
                        unset ac_cv_header_sqlite3_h
                        AC_CHECK_HEADER($sqlite3_path/sqlite3.h, 
                            [
                                SQLITE3_CXXFLAGS="-I$sqlite3_path"
                                SQLITE3_OK=yes
                            ]
                        )
                    fi
                done
            ])
    fi
fi

if test "x$SQLITE3_OK" = xyes; then
    if test -n "$SQLITE3_SEARCH_LIBS"; then
        LDFLAGS="-L$SQLITE3_SEARCH_LIBS"
        AC_MSG_NOTICE([Will search for sqlite3 libraries in $withval])
        AC_CHECK_LIB(sqlite3, sqlite3_open,
                [
                    SQLITE3_LIBS="-L$SQLITE3_SEARCH_LIBS -lsqlite3"
                    SQLITE3_OK=yes
                ],
                [
                    AC_MSG_ERROR(sqlite3 library not found in requested location $SQLITE3_SEARCH_LIBS)
                ],
                [-lrt]
        )
    else
    AC_CHECK_LIB(sqlite3, sqlite3_open, [],
        [
            SQLITE3_OK=missing
            for sqlite3_path in $SEARCH_DIR_LIBS; do
                if test "x$ac_cv_lib_sqlite3_sqlite3_open" != xyes ; then
                    unset ac_cv_lib_sqlite3_sqlite3_open
                    LDFLAGS="-L$sqlite3_path"
                    AC_CHECK_LIB(sqlite3, sqlite3_open,
                        [
                            SQLITE3_LIBS="-L$sqlite3_path -lsqlite3"
                            SQLITE3_OK=yes
                        ],
                        [-lrt]
                    )
                fi
            done
        ],
        [-lrt]
    )
    fi
fi 

if test "x$SQLITE3_OK" = xyes; then
    AC_DEFINE([HAVE_SQLITE3], [1], [sqlite3 library presence])
    AC_SUBST(SQLITE3_CXXFLAGS)
    AC_SUBST(SQLITE3_LIBS)
else
    if test "x$SQLITE3_EN" = xyes; then
        AC_MSG_ERROR(unable to configure sqlite3 support)
    fi
fi

LDFLAGS="$LDFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
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

MYSQL_PROG_FOUND=0

if test "x$MYSQL_OK" = xyes; then
    if test -n "$MYSQL_SEARCH_CONFIG"; then
        AC_MSG_NOTICE([You specified $MYSQL_SEARCH_CONFIG for mysql_config])
        
        if test -f "$MYSQL_SEARCH_CONFIG"; then 
            mysqlconfig="$MYSQL_SEARCH_CONFIG"
        else
            mysqlconfig="no"
        fi
        
        if test "x$mysqlconfig" = xno; then
            AC_MSG_ERROR([$MYSQL_SEARCH_CONFIG not found])
        else
            MYSQL_TEST=`${mysqlconfig} --version 2>/dev/null`
            if test -z "$MYSQL_TEST"; then
                AC_MSG_ERROR([${mysqlconfig} could not be executed or returned invalid values])
            fi

            MYSQL_PROG_FOUND=1
        fi
    else
        AC_PATH_PROG(mysqlconfig, mysql_config, no)
        if test "x$mysqlconfig" = xno; then
            unset ac_cv_path_mysqlconfig
            
            AC_PATH_PROG(mysqlconfig, mysql_config, no, $SEARCH_DIR_PROGS)
            if test "x$mysqlconfig" = xno; then
                MYSQL_OK=missing
                AC_MSG_RESULT([mysql_config script not found])
            else
                MYSQL_PROG_FOUND=1
            fi
        else
            MYSQL_PROG_FOUND=1
        fi
    fi
 
    if test $MYSQL_PROG_FOUND -eq 1 ; then 
        AC_MSG_CHECKING(mysql version)
        MYSQL_VERSION_MAJOR=`${mysqlconfig} --version 2>/dev/null | sed 's/^\([[0-9]]\{1,\}\)\.\([[0-9]]\{1,\}\).\([[0-9]]\{1,\}\).*$/\1/'`
        MYSQL_VERSION_MINOR=`${mysqlconfig} --version 2>/dev/null | sed 's/^\([[0-9]]\{1,\}\)\.\([[0-9]]\{1,\}\).\([[0-9]]\{1,\}\).*$/\2/'` 
        
        MYSQL_PRINT_VERSION=`${mysqlconfig} --version`
        AC_MSG_RESULT([$MYSQL_PRINT_VERSION])

        if test -z $MYSQL_VERSION_MAJOR || test -z $MYSQL_VERSION_MINOR; then
            AC_MSG_WARN(${mysqlconfig} could not be executed or returned invalid values)
            MYSQL_OK=missing
        elif test $MYSQL_VERSION_MAJOR -le 3 ; then
                AC_MSG_WARN(MySQL version too old, we only support version 4.0.x or later)
                MYSQL_OK=missing
        else
            if test "$MYSQL_VERSION_MAJOR" -gt 3 ; then
                AC_MSG_CHECKING(mysql includes)
                MYSQL_CFLAGS=`${mysqlconfig} --include`
                AC_MSG_RESULT($MYSQL_CFLAGS)
 
                AC_MSG_CHECKING(mysql libraries)
                MYSQL_LIBS=`${mysqlconfig} --libs_r`
                AC_MSG_RESULT($MYSQL_LIBS)
            else
                AC_MSG_WARN(Unsupported mysql version)
                MYSQL_OK=missing
            fi
        fi
 
        CXXFLAGS="$CXXFLAGS $MYSQL_CFLAGS"
        CFLAGS="$CFLAGS $MYSQL_CFLAGS"
        CPPFLAGS="$CPPFLAGS $MYSQL_CFLAGS"
        LDFLAGS="$LDFLAGS $MYSQL_LIBS"
    fi
fi

dnl we are not doing an extra search in $SEARCH_DIR because the
dnl mysql_config set the correct flags already, if not - something
dnl is anyway wrong with the mysql installation
dnl we will however accept custom user flags
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_HEADER(mysql.h,
        [],
        [
            AC_MSG_RESULT(mysql.h failed)
            MYSQL_OK=missing
        ]
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_CHECK_FUNCS(mysql_init,
        [],
        [
	    MYSQL_OK=missing
            AC_MSG_RESULT(mysqlclient link failed)
        ]
    )
fi
if test "x$MYSQL_OK" = xyes; then
    AC_DEFINE([HAVE_MYSQL], [1], [MySLQ library presence])
    AC_SUBST(MYSQL_LIBS)
    AC_SUBST(MYSQL_CFLAGS)
    AC_CHECK_FUNCS([mysql_stmt_init],[],[])
else
    if test "x$MYSQL_EN" = xyes; then
        AC_MSG_ERROR(unable to configure mysql support)
    fi
fi

CPPFLAGS="$CPPFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
CFLAGS="$CFLAGS_SAVE"
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

XP_ADD=""

if test "x$JS_OK" = xyes; then
    if test $DARWIN_OS -eq 1 ; then
        XP_ADD="-DXP_MAC"
    else
        XP_ADD="-DXP_UNIX"
    fi

    CXXFLAGS="$CXXFLAGS $XP_ADD"
    CPPFLAGS="$CPPFLAGS $XP_ADD"

    if test -n "$JS_SEARCH_HEADERS" ; then
        AC_MSG_NOTICE([Will search for js (spidermonkey) headers in $withval])
        AC_CHECK_HEADER($JS_SEARCH_HEADERS/jsapi.h,
                [
                    JS_CXXFLAGS="-I$JS_SEARCH_HEADERS"
                ],
                [
                    AC_MSG_ERROR([jsapi.h not found in requested location $JS_SEARCH_HEADERS])
                ]
        )
    
    else
    AC_CHECK_HEADER(jsapi.h,
        [],
        [
            for sm_path in $SEARCH_DIR_HEADERS \
                           /usr/include/js \
                           /usr/include/smjs \
                           /usr/local/include/js \
                           /usr/local/include/smjs \
                           /usr/include/spidermonkey \
                           /usr/local/include/spidermonkey \
                           $SEARCH_DIR_HEADERS/js \
                           $SEARCH_DIR_HEADERS/smjs \
                           $SEARCH_DIR_HEADERS/spidermonkey; do 
                if test "x$ac_cv_header_jsapi_h" != xyes ; then
                    AC_MSG_CHECKING([checking alternative libjs header location: $sm_path])
                    CXXFLAGS="$XP_ADD -I$sm_path"
                    CPPFLAGS="$XP_ADD -I$sm_path"
                    unset ac_cv_header_jsapi_h
                    AC_CHECK_HEADER(jsapi.h,
                                    [
                                        JS_CXXFLAGS="-I$sm_path"
                                        AC_MSG_RESULT([jsapi.h found in $sm_path])
                                    ],
                                    [])
               fi
            done
        ])
   
        if test "x$ac_cv_header_jsapi_h" != xyes ; then
                JS_OK=missing
                AC_MSG_RESULT([not found, giving up])
        fi
    fi
fi

if test "x$JS_OK" = xyes; then
    if test -n "$JS_SEARCH_LIBS" ; then
        AC_MSG_NOTICE([Will search for js (spidermonkey) libraries in $withval])
        LDFLAGS="-L$JS_SEARCH_LIBS -ljs"
        AC_CHECK_LIB(js, JS_NewObject,
                [
                    JS_LIBS="-L$JS_SEARCH_LIBS -ljs"
                ],
                [
                    LDFLAGS="-L$JS_SEARCH_LIBS -lsmjs"
                    AC_CHECK_LIB(js, JS_NewObject,
                        [
                            JS_LIBS="-L$JS_SEARCH_LIBS -lsmjs"
                        ],
                        [
                            AC_MSG_ERROR([Spidermonkey not found in requested location $JS_SEARCH_LIBS])
                        ]
                    )
                ]
        )
    else
    
    AC_CHECK_LIB(js, JS_NewObject,
        [],
        [
            unset ac_cv_lib_js_JS_NewObject
            AC_CHECK_LIB(smjs, JS_NewObject,
                [],
                [
                    LDFLAGS="-L$SEARCH_DIR_LIBS"
                    unset ac_cv_lib_smjs_JS_NewObject
                    AC_CHECK_LIB(js, JS_NewObject,
                        [JS_LIBS="-L$SEARCH_DIR_LIBS -ljs"],
                        [
                            unset ac_cv_lib_js_JS_NewObject
                            AC_CHECK_LIB(smjs, JS_NewObject,
                                [JS_LIBS="-L$SEARCH_DIR_LIBS -lsmjs"],
                                [
                                    JS_OK=missing
                                    AC_MSG_RESULT(libjs/libsmjs link failed, giving up)
                                ])
                        ])
                ])
       ])
    fi
fi
if test "x$JS_OK" = xyes; then
    AC_DEFINE([HAVE_JS], [1], [libjs presence])
    AC_SUBST(JS_CXXFLAGS)
    AC_SUBST(JS_LIBS)
else
    if test "x$JS_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libjs support)
    fi
fi

CXXFLAGS="$CXXFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"

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
    if test -n "$MAGIC_SEARCH_HEADERS" ; then
        AC_MSG_NOTICE([Will search for filemagic headers in $MAGIC_SEARCH_HEADERS])
        AC_CHECK_HEADER($MAGIC_SEARCH_HEADERS/magic.h,
                [
                    MAGIC_CXXFLAGS="-I$MAGIC_SEARCH_HEADERS"
                ],
                [
                    AC_MSG_ERROR([filemagic headers not found in requested location $MAGIC_SEARCH_HEADERS])
                ]
        )
    else
    AC_CHECK_HEADER(magic.h,
        [],
        [
            unset ac_cv_header_magic_h
            AC_CHECK_HEADER($SEARCH_DIR_HEADERS/magic.h,
                [
                    MAGIC_CXXFLAGS="-I$SEARCH_DIR_HEADERS"
                ],
                [MAGIC_OK=missing]
            )
        ]
    )
    fi
fi

if test "x$MAGIC_OK" = xyes; then
    if test -n "$MAGIC_SEARCH_LIBS" ; then
        LDFLAGS="$LDFLAGS $Z_LIBS"
        AC_CHECK_LIB(z,compress,
           [  
              LDFLAGS="-L$MAGIC_SEARCH_LIBS $Z_LIBS"
              AC_MSG_NOTICE([Will search for filemagic libraries in $MAGIC_SEARCH_LIBS])
              AC_CHECK_LIB(magic,magic_load,
                  [
                      MAGIC_LIBS="-L$MAGIC_SEARCH_LIBS -lmagic $Z_LIBS"
                  ],
                  [
                      AC_MSG_ERROR([filemagic libraries not found in requested location $MAGIC_SEARCH_LIBS])
                  ],
                  [-lz]
              )
           ],
           [
              LDFLAGS="-L$MAGIC_SEARCH_LIBS"
              AC_CHECK_LIB(magic,magic_load,
                  [
                      MAGIC_LIBS="-L$MAGIC_SEARCH_LIBS -lmagic"
                  ],
                  [
                      AC_MSG_ERROR([filemagic libraries not found in requested location $MAGIC_SEARCH_LIBS])
                  ]
              )
           ]
       )
    else
    LDFLAGS="$LDFLAGS $Z_LIBS"
    AC_CHECK_LIB(z,compress,
         [
            AC_CHECK_LIB(magic,magic_load,
                [MAGIC_LIBS="$Z_LIBS -lmagic"],
                [
                    LDFLAGS="$LDFLAGS -L$SEARCH_DIR_LIBS"
                    unset ac_cv_lib_magic_magic_load
                    AC_CHECK_LIB(magic,magic_load,
                    [MAGIC_LIBS="-L$SEARCH_DIR_LIBS $Z_LIBS -lmagic"],
                    [MAGIC_OK=missing],[-lz])
                ],[-lz]
            )
         ],
         [
            AC_CHECK_LIB(magic,magic_load,
                [],
                [
                    LDFLAGS="-L$SEARCH_DIR_LIBS"
                    unset ac_cv_lib_magic_magic_load
                    AC_CHECK_LIB(magic,magic_load,
                    [MAGIC_LIBS="-L$SEARCH_DIR_LIBS -lmagic"],
                    [MAGIC_OK=missing])
                ]
            )
        ] 
     )
    fi
fi

if test "x$MAGIC_OK" = xyes; then
    AC_DEFINE([HAVE_MAGIC], [1], [filemagic library presence])
    AC_SUBST(MAGIC_CXXFLAGS)
    AC_SUBST(MAGIC_LIBS)
else
    if test "x$MAGIC_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libmagic support)
    fi
fi

LDFLGAS="$LDFLAGS_SAVE"

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

TAGLIB_PROG_FOUND=0
if test "x$TAGLIB_OK" = xyes; then
    if test -n "$TAGLIB_SEARCH_CONFIG"; then
        AC_MSG_NOTICE([You specified $TAGLIB_SEARCH_CONFIG for taglib-config])
        
        if test -f "$TAGLIB_SEARCH_CONFIG"; then 
            taglibconfig="$TAGLIB_SEARCH_CONFIG"
        else
            taglibconfig="no"
        fi
        
        if test "x$taglibconfig" = xno; then
            AC_MSG_ERROR([$TAGLIB_SEARCH_CONFIG not found])
        else
            TAGLIB_TEST=`${taglibconfig} --version 2>/dev/null`
            if test -z "$TAGLIB_TEST"; then
                AC_MSG_ERROR([${taglibconfig} could not be executed or returned invalid values])
            fi
            TAGLIB_PROG_FOUND=1
        fi
    else
        AC_PATH_PROG(taglibconfig, taglib-config, no)
        if test "x$taglibconfig" = xno; then
            unset ac_cv_path_taglibconfig
            AC_PATH_PROG(taglibconfig, taglib-config, no, $SEARCH_DIR_PROGS)
            if test "x$taglibconfig" = xno; then
                TAGLIB_OK=missing
                AC_MSG_RESULT([taglib-config not found, please install the taglib-devel package])
            else
                TAGLIB_PROG_FOUND=1
            fi
        else
            TAGLIB_PROG_FOUND=1
        fi
   fi 

   if test "$TAGLIB_PROG_FOUND" -eq 1 ; then
        AC_MSG_CHECKING(taglib cflags)
        TAGLIB_CFLAGS=`${taglibconfig} --cflags`
        AC_MSG_RESULT($TAGLIB_CFLAGS)

        AC_MSG_CHECKING(taglib libraries)
        TAGLIB_LIBS=`${taglibconfig} --libs`
        AC_MSG_RESULT($TAGLIB_LIBS)
        TAGLIB_LIBS="$TALIB_LIBS $Z_LIBS"
    fi
fi

if test "x$TAGLIB_OK" = xyes; then
    CPPFLAGS="$CPPFLAGS $TAGLIB_CFLAGS"
    CXXFLAGS="$CXXFLAGS $TAGLIB_CFLAGS"
    AC_CHECK_HEADERS([taglib.h fileref.h tag.h audioproperties.h tstring.h],
        [],
        [TAGLIB_OK=missing]
    )
fi

if test "x$TAGLIB_OK" = xyes; then
    LDFLAGS="$TAGLIB_LIBS"
    AC_CHECK_LIB(tag, main, [],
        [TAGLIB_OK=missing]
    )
fi

if test "x$TAGLIB_OK" = xyes; then
    AC_DEFINE([HAVE_TAGLIB], [1], [taglib library presence])
    AC_SUBST(TAGLIB_CFLAGS)
    AC_SUBST(TAGLIB_LIBS)
    ID3_OK=disabled
else
    if test "x$TAGLIB_EN" = xyes; then
        AC_MSG_ERROR(unable to configure taglib support)
    fi
fi
CPPFLAGS="$CPPFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"

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
    if test -n "$ID3_SEARCH_HEADERS" ; then
     CPPFLAGS="-I$ID3_SEARCH_HEADERS"
     AC_CHECK_HEADER($ID3_SEARCH_HEADERS/id3/tag.h,
        [
            ID3_CXXFLAGS="-I$ID3_SEARCH_HEADERS"
        ],
        [
            AC_MSG_ERROR([id3lib headers not found in requested location $ID3_SEARCH_HEADERS])
        ]
    )
   else
    AC_CHECK_HEADER(id3/tag.h,
        [],
        [
            unset ac_cv_header_id3_tag_h
            CPPFLAGS="-I$SEARCH_DIR_HEADERS"
            AC_CHECK_HEADER(id3/tag.h,
                [
                    ID3_CXXFLAGS="-I$SEARCH_DIR_HEADERS"
                ],
                [
                    ID3_OK=missing
                ]
            )
                
        ]
    )
    fi
fi

if test "x$ID3_OK" = xyes; then
    if test -n "$ID3_SEARCH_HEADERS" ; then
     CPPFLAGS="-I$ID3_SEARCH_HEADERS"
     AC_CHECK_HEADER(id3/misc_support.h,
        [
            ID3_CXXFLAGS="-I$ID3_SEARCH_HEADERS"
        ],
        [
            AC_MSG_ERROR([id3lib headers not found in requested location $ID3_SEARCH_HEADERS])
        ]
    )
   else
    AC_CHECK_HEADER(id3/misc_support.h,
        [],
        [
            unset ac_cv_header_id3_misc_support_h
            CPPFLAGS="-I$SEARCH_DIR_HEADERS"
            AC_CHECK_HEADER(id3/misc_support.h,
                [
                    ID3_CXXFLAGS="-I$SEARCH_DIR_HEADERS"
                ],
                [
                    ID3_OK=missing
                ]
            )
                
        ]
    )
    fi
fi

if test "x$ID3_OK" = xyes; then
    if test -n "$ID3_SEARCH_LIBS" ; then
        LDFLAGS="$LDFLAGS $Z_LIBS"
        AC_CHECK_LIB(z,compress,
           [
              LDFLAGS="-L$ID3_SEARCH_LIBS $Z_LIBS"
              AC_MSG_NOTICE([Will search for id3lib libraries in $ID3_SEARCH_LIBS])
              AC_CHECK_LIB(id3, main
                  [
                      ID3_LIBS="-L$ID3_SEARCH_LIBS -lid3 $Z_LIBS"
                  ],
                  [
                      AC_MSG_ERROR([id3lib libraries not found in requested location $ID3_SEARCH_LIBS])
                  ],
                  [-lz]
              )
           ],
           [
              LDFLAGS="-L$ID_SEARCH_LIBS"
              AC_CHECK_LIB(id3, main,
                  [
                      ID3_LIBS="-L$ID3_SEARCH_LIBS -lid3"
                  ],
                  [
                      AC_MSG_ERROR([id3lib libraries not found in requested location $ID3_SEARCH_LIBS])
                  ]
              )
           ]
       )
    else
    LDFLAGS="$LDFLAGS $Z_LIBS"
    AC_CHECK_LIB(z,compress,
         [
            AC_CHECK_LIB(id3, main,
                [
                    ID3_LIBS="$Z_LIBS -lid3"
                ],
                [
                    LDFLAGS="$LDFLAGS -L$SEARCH_DIR_LIBS"
                    unset ac_cv_lib_id3_main
                    AC_CHECK_LIB(id3, main,
                        [
                            ID3_LIBS="$Z_LIBS -L$SEARCH_DIR_LIBS -lid3"
                        ],
                        [
                            MAGIC_OK=missing
                        ]
                    )
                ],
                [-lz])
         ],
         [
            AC_CHECK_LIB(id3,main,[],
                [
                    LDFLAGS="-L$SEARCH_DIR_LIBS"
                    unset ac_cv_lib_id3_main
                    AC_CHECK_LIB(id3, main,
                        [
                            ID3_LIBS="-L$SEARCH_DIR_LIBS -lid3"
                        ],
                        [
                            MAGIC_OK=missing
                        ]
                    )
                ]
            )
         ] 
     )
    fi
    
fi

if test "x$ID3_OK" = xyes; then
   AC_DEFINE([HAVE_ID3], [1], [id3lib presence])
   AC_SUBST(ID3_CXXFLAGS)
   AC_SUBST(ID3_LIBS)
else
    if test "x$ID3_EN" = xyes; then
        if test "x$TAGLIB_OK" = xyes; then
            AC_MSG_ERROR([Please select either taglib or id3lib, but not both.])
        else
            AC_MSG_ERROR(unable to configure id3lib support)
        fi
    fi
fi

LDFLAGS="$LDFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"

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
    if test -n "$EXTRACTOR_SEARCH_HEADERS" ; then
        AC_CHECK_HEADER($EXTRACTOR_SEARCH_HEADERS/extractor.h,
                [ EXTRACTOR_CXXFLAGS="-I$EXTRACTOR_SEARCH_HEADERS" ],
                [ AC_MSG_ERROR([extractor headers not found in requested location $EXTRACTOR_SEARCH_HEADERS])]
        )
    else
        AC_CHECK_HEADER(extractor.h,
            [],
            [
                unset ac_cv_header_extractor_h
                AC_CHECK_HEADER($SEARCH_DIR_INCLUDES/extrator.h,
                    [ EXTRACTOR_CXXFLAGS="-I$SEARCH_DIR_INCLUDES" ],
                    [ EXTRACTOR_OK=missing ]
                )
            ]
        )
    fi
fi

if test "x$EXTRACTOR_OK" = xyes; then
    if test -n "$EXTRACTOR_SEARCH_HEADERS" ; then
        LDFLAGS="-L$EXTRACTOR_SEARCH_LIBS"
        AC_CHECK_LIB(extractor, EXTRACTOR_getKeywords,
                [LDFLAGS="-L$EXTRACTOR_SEARCH_LIBS -lextractor"],
                [
                    AC_MSG_ERROR([extractor libraries not found in requested location $EXTRACTOR_SEARCH_LIBS])
                ]
        )
    else
        AC_CHECK_LIB(extractor, EXTRACTOR_getKeywords,
            [], 
            [
                LDFLAGS="-L$SEARCH_DIR_LIBS"
                unset ac_cv_lib_extractor_EXTRACTOR_getKeywords
                AC_CHECK_LIB(extractor, EXTRACTOR_getKeywords,
                    [ EXTRACTOR_LIBS="-L$SEARCH_DIR_LIBS -lextractor" ],
                    [ EXTRACTOR_OK=missing ]
                )
            ]
        )
    fi
fi

if test "x$EXTRACTOR_OK" = xyes; then
    AC_DEFINE([HAVE_EXTRACTOR], [1], [libextractor library presence])
    AC_SUBST(EXTRACTOR_CXXFLAGS)
    AC_SUBST(EXTRACTOR_LIBS)
else
    if test "x$EXTRACTOR_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libextractor support)
    fi
fi


######## libexif

AC_ARG_ENABLE(libexif,
        AC_HELP_STRING([--enable-libexif], [compile with libexif support (default: yes)]),
    [
        EXIF_EN=$enableval 
        if test "x$enableval" = xno; then
            EXIF_OK=disabled
        else
            EXIF_OK=yes
        fi
    ],
    [
        EXIF_OK=yes
    ]
)

if test "x$EXIF_OK" = xyes; then
    if test -n "$EXIF_SEARCH_HEADERS" ; then
        CPPFLAGS="-I$EXIF_SEARCH_HEADERS"
        AC_CHECK_HEADER($EXIF_SEARCH_HEADERS/libexif/exif-content.h,
                [
                    EXIF_CXXFLAGS="-I$EXIF_SEARCH_HEADERS"
                ],
                [
                    AC_MSG_ERROR([libexif headers not found in requested location $EXIF_SEARCH_HEADERS])
                ]
        )
    else 
        AC_CHECK_HEADER(libexif/exif-content.h,
            [],
            [
                CPPFLAGS="-I$SEARCH_DIR_HEADERS"
                unset ac_cv_header_libexif_exif_content_h
                AC_CHECK_HEADER($SEARCH_DIR_HEADERS/libexif/exif-content.h,
                    [
                        EXIF_CXXFLAGS="-I$SEARCH_DIR_HEADERS"
                    ],
                    [
                        EXIF_OK=missing
                    ]
                )
            ]
        )
    fi
fi

LIBS_SAVE="$LIBS"
unset LIBS
if test "x$EXIF_OK" = xyes; then
    if test -n "$EXIF_SEARCH_LIBS" ; then
        LDFLAGS="-L$EXIF_SEARCH_LIBS"
        AC_CHECK_LIB(exif, exif_data_new_from_file,
                [
                    EXIF_LIBS="-L$EXIF_SEARCH_LIBS -lexif"
                ],
                [
                    AC_MSG_ERROR([libexif libraries not found in requested path $EXIF_SEARCH_LIBS])
                ]
        )
    else
        AC_CHECK_LIB(exif, exif_data_new_from_file,
            [],
            [
                LDFLAGS="-L$SEARCH_DIR_LIBS"
                unset ac_cv_lib_exif_exif_data_new_from_file
                AC_CHECK_LIB(exif, exif_data_new_from_file,
                    [
                        EXIF_LIBS="-L$SEARCH_DIR_LIBS -lexif"
                    ],
                    [ EXIF_OK=missing ]
                )
            ]
        )
    fi
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

LIBS="$LIBS_SAVE $LIBS"

if test "x$EXIF_OK" = xyes; then
    AC_DEFINE([HAVE_EXIF], [1], [EXIF library presence])
    AC_SUBST(EXIF_CXXFLAGS)
    AC_SUBST(EXIF_LIBS)
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

LDFLAGS="$LDFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"

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

if (test $AUTOCREATE -eq 1) && (test $AUTOCREATE_REQUESTED -eq 1); then
    echo "db-autocreate : enabled"
fi
        
if test "$LOG_OK" != "yes"; then
    echo "log output    : $LOG_OK"
fi

if test "$LOGDEBUG_OK" != "disabled"; then
    echo "debug log     : $LOGDEBUG_OK"
fi

if test "$UPNPDEBUG_OK" != "disabled"; then
    echo "upnpdebug     : $UPNPDEBUG_OK"
fi

if test "$STATIC" = "yes"; then
    echo "static build  : $STATIC"
fi
# echo "  libjpeg     : $LIBJPEG_OK"
# echo "  exiv2       : $EXIV2_OK"
echo

