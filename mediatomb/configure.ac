#*MT*
#    
#    MediaTomb - http://www.mediatomb.cc/
#    
#    configure.ac - this file is part of MediaTomb.
#    
#    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
#                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
#    
#    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
#                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
#                            Leonhard Wimmer <leo@mediatomb.cc>
#    
#    MediaTomb is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License version 2
#    as published by the Free Software Foundation.
#    
#    MediaTomb is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#    
#    You should have received a copy of the GNU General Public License
#    version 2 along with MediaTomb; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
#    
#    $Id$
#
#*MT*

AC_PREREQ(2.61)
AC_INIT([MediaTomb], [0.11.0], [jin@mediatomb.cc])
AM_CONFIG_HEADER([autoconfig.h tombupnp/upnp/inc/upnpconfig.h])
AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_SRCDIR([src/common.h])
AM_INIT_AUTOMAKE([1.9 -Wall])

if test "x${prefix}" = "xNONE"; then
    prefix="${ac_default_prefix}"
fi

SEARCH_DIR="/usr/local"
   
DARWIN_OS=0
CYGWIN_OS=0
FREEBSD_OS=0

LIBS_SAVE="$LIBS"

AC_CANONICAL_HOST

case $host in
    *-*-darwin*)
        DARWIN_OS=1
        SEARCH_DIR="/opt/local"
        ;;
    *-*-solaris*)
        AC_DEFINE([SOLARIS], [1], [we are on solaris])
        ;;
    *-*-cygwin*)
        CYGWIN_OS=1
        ;;
    *-*-freebsd*)
        FREEBSD_OS=1
	;;
esac

MT_SET_SEARCHPATH

#AC_ARG_WITH(search, 
#        AC_HELP_STRING([--with-search=DIR], [Additionally search for packages in DIR]),
#        [
#            SEARCH_DIR=$withval
#            AC_MSG_NOTICE([Will also search for packages in $SEARCH_DIR])
#        ]
#)

SEARCH_DIR_HEADERS="$SEARCH_DIR/include"
SEARCH_DIR_LIBS="$SEARCH_DIR/lib"
SEARCH_DIR_PROGS="$SEARCH_DIR/bin"
    
# per package header and library search paths

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

AC_ARG_WITH(ffmpeg-h,
        AC_HELP_STRING([--with-ffmpeg-h=DIR], [search for ffmpeg headers in DIR]),
        [
            FFMPEG_SEARCH_HEADERS="$withval"
            AC_MSG_NOTICE([Will search for ffmpeg headers in $withval])
        ]
)

AC_ARG_WITH(ffmpeg-libs,
        AC_HELP_STRING([--with-ffmpeg-libs=DIR], [search for ffmpeg libraries in DIR]),
        [
            FFMPEG_SEARCH_LIBS="$withval"
            AC_MSG_NOTICE([Will search for ffmpeg libraries in $withval])
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

MT_OPTION([static], [enable],
          [build a static version of MediaTomb],
          [
            CFLAGS="$CFLAGS -static"
            CXXFLAGS="$CXXFLAGS -static"
            AC_MSG_NOTICE([Building a static executable.])
          ],
          [])

MT_OPTION([db-autocreate], [disable],
          [automatic creation of the database],[],[])

MT_OPTION([debug-malloc0], [enable],
          [only for debugging purposes: abort when we try to call malloc or realloc with a value of zero],
          [
            AC_DEFINE([DEBUG_MALLOC_0], [1], 
                      [debug: abort on malloc/realloc of zero bytes])
          ],
          [])


MT_OPTION([pthread-lib], [enable],
          [if this option is set we will try to link with -lpthread, else the flag for pthread will be autodetected], [], [])

MT_OPTION([iconv-lib], [enable],
          [if this option is set we will try to link with -liconv, else we will first try to use iconv built into glibc and only then search for libiconv],
          [],[])


ATOMIC_X86_SMP=0
ATOMIC_X86_SMP_REQ=0
X86=0
case $host_cpu in
    *86)
        ATOMIC_X86_SMP=1
        X86=1
    ;;
    *86*64)
        ATOMIC_X86_SMP=1
        X86=1
    ;;
esac

ATOMIC_X86=0

MT_OPTION([atomic-x86-single], [enable], 
          [use x86 singleprocessor code for atomic arithmetic operations, this will increase performance but you must not use the compiled binary on an SMP system. Doing so might result in crashes and unexpected behaviour],
          [
              if test "$X86" -eq 1; then
                    ATOMIC_X86=1
                    ATOMIC_X86_SMP=0
              else
                    AC_MSG_ERROR([Tried to activate x86 specific option for a non x86 host!])
              fi
          ],
          [])

AC_ARG_ENABLE([atomic-pthread],
              [
AC_HELP_STRING([--enable-atomic-pthread],
[use pthreads for atomic arithmetic operations, this will deliver the worst performance but is the only portable option (default: automatic, depending on architecture])
            ],
            [
                if test "x$enableval" = xyes; then
                    if test "$ATOMIC_X86" -eq 1; then
                        AC_MSG_ERROR([You can not use atomic-x86-single and atomic-pthread code at the same time!])
                    else
                        ATOMIC_X86=0 
                        ATOMIC_X86_SMP=0 
                    fi
                fi
            ]
)

MT_OPTION([atomic], [disable],
          [NEVER disable this! This is only for devel/debugging - disables all atomic arithmetics code],
          [],
          [
              ATOMIC_X86=0
              ATOMIC_X86_SMP=0
              AC_MSG_WARN(You disabled the use of atomic arithmetics! You have been warned!)
               AC_DEFINE([ATOMIC_TORTURE], [1], [NEVER use this! This is only for devel/debugging - disables all atomic arithmetics code.])
          ])

if ((test $ATOMIC_X86_SMP -eq 1) && (test $ATOMIC_X86 -eq 1)); then
    AC_MSG_ERROR([Cannot use atomic-x86-smp and atomic-x86 options at the same time!])
fi

if (((test $ATOMIC_X86_SMP -eq 1) || (test $ATOMIC_X86 -eq 1)) && 
     (test "$X86" -eq 0)); then
    AC_MSG_ERROR([Cannot use x86 specific code on a non a $host_cpu system!])
fi

if test $ATOMIC_X86_SMP -eq 1; then
    AC_DEFINE([ATOMIC_X86_SMP], [1], [use x86 assembler code for atomic arithmetic operations - default for SMP and UP systems])
elif test $ATOMIC_X86 -eq 1; then
    AC_DEFINE([ATOMIC_X86], [1], [use x86 assembler code for atomic arithmetic operations that will only work oni singleprocessor systems - compiled binary will be buggy on SMP])
    AC_MSG_WARN(You specified to use atomic arithmetics code for x86 single processor systems! The resulting binary must not be used on SMP machines!)
fi


SIGHUP=0
if test $X86 -eq 1; then
    SIGHUP=1
fi

AC_ARG_ENABLE([sighup],
              [
AC_HELP_STRING([--enable-sighup],
[enable SIGHUP handling, by default this is only enabled for x86 platforms, consult the README for more information (default: auto)])
            ],
            [
                if test "x$enableval" = xno; then
                    SIGHUP=0
                else
                    SIGHUP=1
                fi
            ]
)

if test $SIGHUP -eq 1; then
    AC_DEFINE([ENABLE_SIGHUP], [1], [enable SIGHUP handling])
fi

MT_OPTION([mrreg-service], [enable],
          [Enable the X_MS_MediaReceiverRegistrar UPnP service. This is for future XBox 360 support, other renderers will probably not need it],
          [
            AC_DEFINE([ENABLE_MRREG], [1], [Enable the X_MS_MediaReceiverRegistrar UPnP service. This is for future XBox 360 support, other renderers will probably not need it])
          ],[])

MT_OPTION([external-transcoding], [disable],
          [external transcoding support],
          [],[EXTERNAL_TRANSCODING_OPTION_ENABLED=disabled])

MT_OPTION([sopcast], [enable],
          [SopCast service support],
          [
            AC_DEFINE([SOPCAST], [1], [SopCast service support])
          ],[SOPCAST_OPTION_ENABLED=disabled])


MT_OPTION([youtube], [disable],
          [YouTube service support],
          [],[YOUTUBE_OPTION_ENABLED=disabled])

MT_OPTION([sqlite-backup-defaults], [enable],
          [enable sqlite database backup settings in config.xml by default],
          [],[])

MT_OPTION([protocolinfo-extension], [disable],
          [This is required for Playstation 3 support, it adds certain tags to the protocolInfo attribute],
          [
            AC_DEFINE([EXTEND_PROTOCOLINFO], [1], [This is required for Playstation 3 support, it adds certain tags to the protocolInfo attribute])
          ],[])

USE_CURL=1
CURL_REQUESTED=0
AC_ARG_ENABLE([curl],
              [
AC_HELP_STRING([--enable-curl],
[Enable curl support (required for YouTube support, provides additional functionality for the external transcoding feature (default: auto)])
              ],
              [
                if test "x$enableval" = xno; then
                    USE_CURL=0
                    CURL_REQUESTED=1
                else
                    CURL_REQUESTED=1
                    USE_CURL=1
                fi 
              ]
)

AC_ARG_WITH([curl-cfg],
            [
AC_HELP_STRING([--with-curl-cfg=curl-config],
                    [absolute path/name of curl-config])
            ],
            [
                CURL_SEARCH_CONFIG="$withval"
                AC_MSG_NOTICE([Will search for curl-config in $withval])
            ]
)

MT_OPTION([fseeko-check], [disable],
          [This is a workaround for a bug on some Debian distros, use this if configure fails complaining about off_t],
          [],[])

###############################################################################
# The following block is derived from the configure.ac script from the 
# libupnp-1.3.1 package
# (C) Copyright 2005-2006 Remi Turboult <r3mi@users.sourceforge.net>
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
AC_PROG_GCC_TRADITIONAL
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_HEADER_STAT
AC_LANG_C
AM_PROG_CC_C_O

AC_CHECK_HEADERS([syslog.h stddef.h unistd.h arpa/inet.h fcntl.h], [],
                 [AC_MSG_ERROR(required header not found)]) 

AC_CHECK_HEADERS([limits.h netdb.h netinet/in.h stdlib.h string.h sys/file.h],
                 [],
                 [AC_MSG_ERROR(required header not found)]) 
AC_CHECK_HEADERS([sys/ioctl.h sys/socket.h sys/time.h sys/types.h sys/wait.h],
                 [],
                 [AC_MSG_ERROR(required header not found)]) 

AC_CHECK_HEADERS([langinfo.h locale.h],[],[AC_MSG_WARN(Header that is required to retrieve system locale not found, will use default value)])

AC_CHECK_HEADERS([sched.h ctype.h],[],[])
AC_CHECK_FUNCS([sched_getparam sched_setparam],[],[])
   
AC_CHECK_FUNCS([mkdir], [],
              [AC_MSG_ERROR(required function not found)])

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

if test "x$EXTERNAL_TRANSCODING_OPTION_ENABLED" = xyes; then
    AC_CHECK_FUNCS([mkfifo],
       [
            AC_DEFINE([EXTERNAL_TRANSCODING], [1], [external transcoding feature])
       ],
       [
           if test "x$EXTERNAL_TRANSCODING_OPTION_REQUESTED" = xyes; then
               AC_MSG_ERROR([mkfifo is required by the external transcoding feature])
           else
               AC_MSG_WARN([mkfifo is required by the external transcoding feature])
               EXTERNAL_TRANSCODING_OPTION_ENABLED=disabled

           fi
       ]
    )
fi

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

LIBICONV=0
if test -n "$ICONV_SEARCH_LIBS"; then
    if test "x$ICONV_LIB_OPTION_ENABLED" = xno; then
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
                        LIBICONV=1
                    ],
                    [
                        AC_CHECK_LIB(iconv, libiconv,
                            [
                                ICONV_LIBS="-L$ICONV_SEARCH_LIBS -liconv"
                                LIBICONV=1
                            ],
                            [AC_MSG_ERROR(required library iconv not found in requested location $ICONV_SEARCH_LIBS)]
                        )
                    ]
                ) 
            ]
        ) 
    else
        LDFLAGS="-L$ICONV_SEARCH_LIBS -liconv"
        AC_CHECK_LIB(iconv, iconv,
                [
                    ICONV_LIBS="-L$ICONV_SEARCH_LIBS -liconv"
                    LIBICONV=1
                ],
                [AC_MSG_ERROR(You specified to use libiconv but it was not found in the requested location $ICONV_SEARCH_LIBS)]
        )
    fi
else
    if test "x$ICONV_LIB_OPTION_ENABLED" = xno; then
        AC_CHECK_FUNC([iconv],
            [
                ICONV_LIBS="$LDFLAGS"
            ],
            [
        
                LDFLAGS="$LDFLAGS -liconv"
                AC_CHECK_LIB(iconv, iconv, 
                    [
                        ICONV_LIBS="$LDFLAGS"
                        LIBICONV=1
                    ], 
                    [
                        AC_CHECK_LIB(iconv, libiconv,
                            [
                                ICONV_LIBS="$LDFLAGS"
                                LIBICONV=1
                            ],
                            [AC_MSG_ERROR(required library iconv not found on your system)]
                        )
                    ]
                )
            ]
        )
    else
        LDFLAGS="$LDFLAGS -liconv"
        AC_CHECK_LIB(iconv, iconv,
                [
                    ICONV_LIBS="$LDFLAGS"
                    LIBICONV=1
                ],
                [
                    AC_MSG_ERROR(You specified to use libiconv but the library was not found)
                ]
       )
    fi
fi

if test $LIBICONV -eq 1; then
    AC_DEFINE([HAVE_LIBICONV], [1], [we are using the libiconv library])
fi

AC_SUBST(ICONV_CXXFLAGS)
AC_SUBST(ICONV_LIBS)

CFLAGS="$CFLAGS $ICONV_CXXFLAGS -Werror"
AC_MSG_CHECKING([if iconv declaration requires const char cast])
AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
         [
             #include <stdlib.h>
             #include <iconv.h>
         ],
         [
            char **ptr = NULL;
            size_t len;
            iconv_t cd = NULL;
            (void)iconv(cd, ptr, &len, ptr, &len);
         ]
     )],
     [
         AC_MSG_RESULT([no])
     ],
     [
         AC_DEFINE([ICONV_CONST], [1], 
                   [iconv needs const char cast])

         AC_MSG_RESULT([yes])
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

if test "x$FSEEKO_CHECK_OPTION_ENABLED" = xyes; then
    AC_FUNC_FSEEKO

    if test "$ac_cv_func_fseeko" = no || test "$ac_cv_sys_largefile_source" = unknown; then
        AC_MSG_WARN([fseeko has not been found on your system, will use fseek instead.])
        AC_MSG_WARN([This has not been tested, please report if you have any problems.])
        AC_DEFINE([fseeko], [fseek], [fseeko not present])
    fi
else
    AC_MSG_WARN([Skipping fseeko test! (requested by user)])
fi

AC_CHECK_SIZEOF(off_t, 4)
AC_CHECK_SIZEOF(size_t, 4)
AC_CHECK_SIZEOF(time_t, 4)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_FUNCS(strtoll nl_langinfo setlocale,[],[])
AC_CHECK_FUNCS(backtrace backtrace_symbols,[],[])
# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_TYPE_SIGNAL
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_INT64_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_STRUCT_TM
AC_C_BIGENDIAN

dnl This needs improvement!
AC_CHECK_DECL(LONG_MAX,
        [
            AC_DEFINE([HAVE_LONG_MAX], [1], [LONG_MAX definition available])
            have_long_max=1
        ], , [#include <limits.h>])


AC_CHECK_DECL(LLONG_MAX,
        [
            AC_DEFINE([MAXLLONG], [LLONG_MAX], [LLONG_MAX definition])
        ],
        [
            AC_CHECK_DECL(__LONG_LONG_MAX__, 
                [
                    AC_DEFINE([MAXLLONG], [__LONG_LONG_MAX__], 
                    [LLONG_MAX definition])],
                [
                    if test "$have_long_max" -eq 1; then
                        AC_DEFINE([MAXLLONG], [LONG_MAX], [LLONG_MAX not available])
                    fi
                ], [#include <limits.h>])
        ]
        , [#include <limits.h>])

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
AC_FUNC_STRERROR_R
AC_FUNC_STRFTIME

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
 
AC_CHECK_FUNCS([ioctl tolower toupper],[],
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
elif test $CYGWIN_OS -eq 1; then
        AC_MSG_NOTICE([Your are building under Cygwin, assuming threadsafe gethostbyname implementation])
fi

AC_CHECK_FUNCS([gethostbyname_r],[],
        [
            AC_CHECK_HEADERS([lwres/netdb.h],
                [
                    AC_CHECK_LIB(lwres, lwres_gethostbyname_r,
                                 [
                                    LWRES_LIBS="-llwres"
                                    AC_DEFINE(HAVE_LIBLWRES, 1, [lwres library presence])
                                    AC_SUBST(LWRES_LIBS)
                                 ],
                                 [AC_MSG_ERROR(required library function not found)])
                ],
                [
                    if ((test $DARWIN_OS -eq 0) && (test $CYGWIN_OS -eq 0)) ; then
                        AC_MSG_ERROR([required header not found])
                    fi
                ])
        ])

AC_CHECK_FUNCS([if_nameindex],[],
        [
            AC_CHECK_LIB(nsl, if_nameindex,
                [
                    NSL_LIBS="-lnsl"
                    AC_SUBST(NSL_LIBS)
                ],
                [
                    if test $CYGWIN_OS -eq 0; then
                        AC_MSG_ERROR(required library function not found)
                    fi
                ]
            )
        ]
)

AC_CHECK_FUNCS([if_freenameindex],[],
        [
            AC_CHECK_LIB(nsl, if_freenameindex,[],
                [
                    if test $CYGWIN_OS -eq 0; then
                        AC_MSG_ERROR(required library function not found)
                    fi
                ]
            )
        ]
)


# this is not optional, we need the device and the internal webserver
AC_DEFINE([UPNP_HAVE_DEVICE], [1], [Compile device API])
AC_DEFINE([UPNP_HAVE_WEBSERVER], [1], [Compile internal web server])    

    
ADD_PTHREAD_CFLAGS=

# see acinclude.m4 for the definition of ACX_PTHREAD
if test "x$PTHREAD_LIB_OPTION_ENABLED" = xyes; then
    MT_CHECK_LIBRARY([pthread], [pthread], [pthread_create])
    if test "x$PTHREAD_STATUS" != xyes; then
        AC_MSG_ERROR([pthread library not found, try running configure without the --enable-pthread-lib option])
    fi
else
    ACX_PTHREAD(,AC_MSG_ERROR(POSIX threads missing))

    if test $FREEBSD_OS -eq 1; then
        ADD_PTHREAD_CFLAGS="$PTHREAD_CFLAGS $PTHREAD_LIBS"
    fi

    if test "x$STATIC_OPTION_ENABLED" = xyes; then
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        LIBS="$LIBS $PTHREAD_LIBS"
        CFLAGS_SAVE="$CFLAGS_SAVE $PTHREAD_CFLAGS"
        LIBS_SAVE="$LIBS_SAVE $PTHREAD_LIBS"
    fi

    AC_SUBST(PTHREAD_LIBS)
    AC_SUBST(PTHREAD_CFLAGS)
fi

unset LIBS

# we might need librt for sqlite
MT_CHECK_LIBRARY([rt], [rt], [clock_gettime])

if test "x$RT_STATUS" != xyes; then
    AC_CHECK_FUNC(clock_gettime)
fi

############ zlib

MT_CHECK_OPTIONAL_PACKAGE([zlib], [disable],
                 [zlib support required for automated database creation],
                 [zlib], [z], [uncompress])
    

if test "x$ZLIB_STATUS" = xyes; then
    if test "x$DB_AUTOCREATE_OPTION_ENABLED" = xyes; then
        AC_DEFINE([AUTO_CREATE_DATABASE], [1], [create database automatically if it does not exist])
    fi
else
    if test "x$DB_AUTOCREATE_OPTION_ENABLED" = xyes; then
        if test "x$DB_AUTOCREATE_OPTION_REQUESTED" = xyes; then
            AC_MSG_ERROR([Automatic database creation not possible due to missing zlib headers/libraries])
        else
            AC_MSG_WARN([Automatic database creation not possible due to missing zlib headers/libraries])
            DB_AUTOCREATE_OPTION_ENABLED=disabled
        fi
    fi
fi

AC_CACHE_SAVE

######### SQLite3

if test "x$RT_STATUS" = xyes; then
    LDFLAGS="$LDFLAGS $RT_LDFLAGS $RT_LIBS"
fi

MT_CHECK_OPTIONAL_PACKAGE([sqlite3], [disable], [sqlite3 database supprot],
                          [sqlite3], [sqlite3], [sqlite3_open])

if test "x$SQLITE3_STATUS" = xyes; then
    if test "x$RT_STATUS" = xyes; then
        SQLITE_LDFLAGS="$SQLITE_LDFLAGS $RT_LDFLAGS"
        SQLITE_LIBS="$SQLITE_LIBS $RT_LIBS"
    fi

    AC_SUBST(SQLITE3_LDFLAGS)
    AC_SUBST(SQLITE3_LIBS)

    if test "x$SQLITE_BACKUP_OPTION_ENABLED" = xyes; then
        AC_DEFINE([SQLITE_BACKUP_ENABLED], [1], [enable sqlite db backup settings in config.xml by default])
    fi
else
    if (test "x$SQLITE_BACKUP_OPTION_ENABLED" = xyes) &&
       (test "x$SQLITE_BACKUP_OPTION_REQUESTED" = xyes); then
        AC_MSG_ERROR(You specified the option to use sqlite backup settings but the sqlite support could not be configured)
    fi

fi

LDFLAGS="$LDFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"

########## MySQL

MT_CHECK_OPTIONAL_PACKAGE_CFG([mysql], 
                              [disable],
                              [mysql database support],
                              [mysql_config],
                              [mysql.h],
                              [], [mysql_init],[--libs_r])

if test "x$MYSQL_STATUS" = xyes; then
    AC_MSG_CHECKING(mysql version)
    MYSQL_VERSION_MAJOR=`echo $MYSQL_VERSION | sed 's/^\([[0-9]]\{1,\}\)\.\([[0-9]]\{1,\}\).\([[0-9]]\{1,\}\).*$/\1/'`
    MYSQL_VERSION_MINOR=`echo $MYSQL_VERSION | sed 's/^\([[0-9]]\{1,\}\)\.\([[0-9]]\{1,\}\).\([[0-9]]\{1,\}\).*$/\2/'`
    AC_MSG_RESULT($MYSQL_VERSION)
    if test -z $MYSQL_VERSION_MAJOR || test -z $MYSQL_VERSION_MINOR; then
        AC_MSG_WARN([could not parse mysql version string])
        MYSQL_STATUS=missing
    elif test $MYSQL_VERSION_MAJOR -le 3 ; then
       AC_MSG_WARN(MySQL version too old, we only support version 4.0.x or later)
       MYSQL_STATUS=missing
    fi
fi

LDFLAGS="$LDFLAGS_SAVE"
LIBS="$LIBS $MYSQL_LIBS"
CFLAGS="$CFLAGS $MYSQL_CFLAGS"
CXXFLAGS="$CXXFLAGS $MYSQL_CFLAGS"
CPPFLAGS="$CFLAGS $MYSQL_CFLAGS"

if test "x$MYSQL_STATUS" = xyes; then
    AC_DEFINE([HAVE_MYSQL], [1], [MySQL library presence])
    AC_SUBST(MYSQL_LIBS)
    AC_SUBST(MYSQL_CFLAGS)
    AC_CHECK_FUNCS([mysql_stmt_init],[],[])
    
    AC_MSG_CHECKING([MYSQL_OPT_RECONNECT])
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [
                #include <mysql.h>
            ],
            [
                (int)mysql_options(0, MYSQL_OPT_RECONNECT, 0);
            ]
        )],
        [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_MYSQL_OPT_RECONNECT], [1], [MYSQL_OPT_RECONNECT parameter available])
        ],
        [
            AC_MSG_RESULT([no])
        ]
    )
else
    MYSQL_CFLAGS=
    MYSQL_LIBS=
fi

CPPFLAGS="$CPPFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
CFLAGS="$CFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"
LIBS="$LIBS_SAVE"

#########  check if at least one database available

if ((test "x$SQLITE3_STATUS" != xyes) && (test "x$MYSQL_STATUS" != xyes)); then
    AC_MSG_ERROR(Support of at least one of mysql or sqlite3 must be configured)
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
dnl    if test $DARWIN_OS -eq 1 ; then
dnl        XP_ADD="-DXP_MAC"
dnl    else
        XP_ADD="-DXP_UNIX"
dnl    fi

    CXXFLAGS="$CXXFLAGS $XP_ADD"
    CPPFLAGS="$CPPFLAGS $XP_ADD"

    if test -n "$JS_SEARCH_HEADERS" ; then
        AC_MSG_NOTICE([Will search for js (spidermonkey) headers in $JS_SEARCH_HEADERS])
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
                           /usr/include/mozjs \
                           /usr/local/include/js \
                           /usr/local/include/smjs \
                           /usr/local/include/mozjs \
                           /usr/include/spidermonkey \
                           /usr/local/include/spidermonkey \
                           $SEARCH_DIR_HEADERS/js \
                           $SEARCH_DIR_HEADERS/smjs \
                           $SEARCH_DIR_HEADERS/mozjs \
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

MOZLIB=
if test "x$JS_OK" = xyes; then
    if test -n "$JS_SEARCH_LIBS" ; then
        AC_MSG_NOTICE([Will search for js (spidermonkey) libraries in $JS_SEARCH_LIBS])
        LDFLAGS="-L$JS_SEARCH_LIBS $LDFLAGS_SAVE -ljs $ADD_PTHREAD_CFLAGS"
        AC_CHECK_LIB(js, JS_NewObject,
                [
                    JS_LIBS="-L$JS_SEARCH_LIBS -ljs"
                    MOZLIB=js
                ],
                [
                    LDFLAGS="-L$JS_SEARCH_LIBS $LDFLAGS_SAVE -lsmjs $ADD_PTHREAD_CFLAGS"
                    AC_CHECK_LIB(smjs, JS_NewObject,
                        [
                            JS_LIBS="-L$JS_SEARCH_LIBS $LDFLAGS_SAVE -lsmjs $ADD_PTHREAD_CFLAGS"
                            MOZLIB=smjs
                        ],
                        [
                            LDFLAGS="-L$JS_SEARCH_LIBS $LDFLAGS_SAVE -lmozjs $ADD_PTHREAD_CFLAGS"
                            AC_CHECK_LIB(mozjs, JS_NewObject,
                                [
                                    JS_LIBS="-L$JS_SEARCH_LIBS -lmozjs"
                                    MOZLIB=mozjs
                                ],
                                [
                                    AC_MSG_ERROR([Spidermonkey not found in requested location $JS_SEARCH_LIBS])
                                ]
                            )
                        ]
                    )
                ]
        )
    else
    LDFLAGS="$LDFLAGS_SAVE $ADD_PTHREAD_CFLAGS"
    AC_CHECK_LIB(js, JS_NewObject,
        [
            MOZLIB=js
            JS_LIBS="-ljs"
        ],
        [
            unset ac_cv_lib_js_JS_NewObject
            AC_CHECK_LIB(smjs, JS_NewObject,
                [
                    JS_LIBS="-lsmjs"
                    MOZLIB=smjs
                ],
                [
                    unset ac_cv_lib_smjs_JS_NewObject
                    AC_CHECK_LIB(mozjs, JS_NewObject,
                        [
                            MOZLIB=mozjs
                            JS_LIBS="-lmozjs"
                        ],
                        [
                            LDFLAGS="-L$SEARCH_DIR_LIBS $LDFLAGS_SAVE $ADD_PTHREAD_CFLAGS"
                            unset ac_cv_lib_js_JS_NewObject
                            AC_CHECK_LIB(js, JS_NewObject,
                                [
                                    JS_LIBS="-L$SEARCH_DIR_LIBS -ljs"
                                    MOZLIB=js
                                ],
                                [
                                    unset ac_cv_lib_smjs_JS_NewObject
                                    AC_CHECK_LIB(smjs, JS_NewObject,
                                        [
                                            JS_LIBS="-L$SEARCH_DIR_LIBS -lsmjs"
                                            MOZLIB=smjs
                                        ],
                                        [
                                            LDFLAGS="-L$SEARCH_DIR_LIBS $LDFLAGS_SAVE -lmozjs"
                                            unset ac_cv_lib_mozjs_JS_NewObject
                                            AC_CHECK_LIB(mozjs, JS_NewObject,
                                                [
                                                    JS_LIBS="-L$SEARCH_DIR_LIBS -lmozjs"
                                                    MOZLIB=mozjs
                                                ],
                                                [
                                                    JS_OK=missing
                                                    AC_MSG_RESULT(libjs/libsmjs link failed, giving up)
                                                ]
                                            )
                                        ]
                                    )
                                ]
                            )
                        ]
                    )
                ]
            )
       ]
    )
    fi
fi
if test "x$JS_OK" = xyes; then
    AC_DEFINE([HAVE_JS], [1], [libjs presence])
    AC_SUBST(JS_CXXFLAGS)
    AC_SUBST(JS_LIBS)

    LDFLAGS="$LDFLAGS $JS_LIBS"
    CFLAGS="$CFLAGS $JS_CXXFLAGS"
    CXXFLAGS="$CXXFLAGS $JS_CXXFLAGS"

    AC_CHECK_LIB($MOZLIB, JS_ClearContextThread,
            [
                AC_DEFINE([JS_THREADSAFE], [1],
                          [this version of spidermonkey is threadsafe])
            ])
else
    if test "x$JS_EN" = xyes; then
        AC_MSG_ERROR(unable to configure libjs support)
    fi
fi

CXXFLAGS="$CXXFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"

########### libmagic
MT_OPTION([libmagic], [disable],
          [filemagic for automatic mimetype recognition],[],[])

if test "x$LIBMAGIC_OPTION_ENABLED" = xyes; then
    MT_CHECK_PACKAGE([libmagic], [magic], [magic], [magic_load])

    if test "x$LIBMAGIC_STATUS" != xyes; then
        if test "x$ZLIB_STATUS" != xyes; then
            MT_CHECK_LIBRARY([zlib], [z], [compress])
            LDFLAGS="$LDFLAGS $ZLIB_LDFLAGS $ZLIB_LIBS"
            AC_MSG_NOTICE([retrying libmagic check with zlib flags])
            MT_CHECK_PACKAGE([libmagic], [magic], [magic], [magic_load])
            if test "x$LIBMAGIC_STATUS" = xyes; then
                LIBMAGIC_LIBS="$LIBMAGIC_LIBS $ZLIB_LIBS"
                LIBMAGIC_LDFLAGS="$LIBMAGIC_LDFLAGS $ZLIB_LDFLAGS"
            fi
        else
            LDFLAGS="$LDFLAGS $ZLIB_LDFLAGS $ZLIB_LIBS"
            AC_MSG_NOTICE([retrying libmagic check with zlib flags])
            MT_CHECK_PACKAGE([libmagic], [magic], [magic], [magic_load])
            if test "x$LIBMAGIC_STATUS" = xyes; then
                LIBMAGIC_LIBS="$LIBMAGIC_LIBS $ZLIB_LIBS"
                LIBMAGIC_LDFLAGS="$LIBMAGIC_LDFLAGS $ZLIB_LDFLAGS"
            fi
        fi

        AC_SUBST(LIBMAGIC_LIBS)
        AC_SUBST(LIBMAGIC_LDFLAGS)
    fi
fi

if test "x$LIBMAGIC_STATUS" = xyes; then
    AC_MSG_CHECKING([if libmagic is broken (Slackware check)])
    CFLAGS="$CFLAGS $LIBMAGIC_CXXLAGS"
    CXXFLAGS="$CXXFLAGS $LIBMAGIC_CFLAGS"
    LDFLAGS="$LDFLAGS $LIBMAGIC_LDFLAGS $LIBMAGIC_LIBS"
    AC_LANG_SAVE
    AC_LANG_C
    AC_RUN_IFELSE(
        AC_LANG_PROGRAM([[ #include <magic.h> ]],
         [[
            struct magic_set *ms;
            ms = magic_open(MAGIC_MIME);
            if (ms == 0)
                return 0;
            magic_load(ms, 0);
            magic_close(ms);
            return 0;
         ]]
        ),
        [
            AC_MSG_RESULT([no]);
        ],
        [
            AC_MSG_RESULT([yes])
            MAGIC_OK=broken
            AC_MSG_WARN([your magic library is broken, if you are using Slackware install the 'file' package manually])
        ],
        [
            AC_MSG_RESULT([passing for cross compiling])
        ])

   AC_LANG_RESTORE
fi

if test "x$LIBMAGIC_STATUS" = xyes; then
    AC_DEFINE([HAVE_MAGIC], [1], [filemagic library presence])
else
    if (test "x$LIBMAGIC_OPTION_REQUESTED" = xyes) && 
       (test "x$LIBMAGIC_OPTION_ENABLED" = xyes); then
        AC_MSG_ERROR(unable to configure libmagic support)
    fi
fi

CFLAGS="$CFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
CPPFLAFS="$CPPFLAFS_SAVE"
LDFLGAS="$LDFLAGS_SAVE"

######### taglib or id3lib selection

TAGLIB_STATUS=
ID3LIB_STATUS=

MT_OPTION([id3lib], [enable],
          [id3 metadata extraction with the help of id3lib],[],[])

MT_OPTION([taglib], [disable],
          [id3 metadata extraction with the help of taglib],[],[])

if ((test "x$TAGLIB_OPTION_ENABLED" = xno) && (test "x$ID3LIB_OPTION_ENABLED" = xyes)) ; then
    TAGLIB_STATUS=disabled
    TAGLIB_OPTION_ENABLED=no
    AC_MSG_WARN([taglib disabled])
elif ((test "x$TAGLIB_OPTION_ENABLED" = xyes) && (test "x$ID3LIB_OPTION_ENABLED" = xno)) ; then
    ID3LIB_STATUS=disabled
    ID3LIB_OPTION_ENABLED=no
    AC_MSG_WARN([id3lib disabled])
elif ((test "x$TAGLIB_OPTION_ENABLED" = xyes) && (test "x$ID3LIB_OPTION_ENABLED" = xyes)) ; then
    AC_MSG_ERROR([Please select either taglib or id3lib, but not both.])
fi

if test "x$TAGLIB_OPTION_ENABLED" = xyes; then
    MT_CHECK_PACKAGE_CFG([taglib],  
                         [taglib-config], 
                         [taglib.h fileref.h tag.h audioproperties.h tstring.h textidentificationframe.h attachedpictureframe.h],
                         [tag], [main])
else
    TAGLIB_STATUS=disabled
    if test "x$ID3_OPTION_REQUESTED" = no; then
        ID3_OPTION_ENABLED=yes
    fi
fi

if test "x$TAGLIB_STATUS" = xyes; then
    AC_DEFINE([HAVE_ID3_ALBUMART], [1], [not aware of taglib versions that do not support album art])
    ID3LIB_STATUS=disabled
    ID3LIB_OPTION_ENABLED=no
else
    if (test "x$TAGLIB_OPTION_REQUESTED" = xyes) && 
       (test "x$TAGLIB_OPTION_ENABLED" = xyes); then
        AC_MSG_ERROR([unable to configure taglib support])
    fi
fi

######### id3lib

if test "x$ID3LIB_OPTION_ENABLED" = xyes; then
    MT_CHECK_PACKAGE([id3lib], [id3/tag], [id3], [main])

    if test "x$ID3LIB_STATUS" != xyes; then
        if test "x$ZLIB_STATUS" != xyes; then
            MT_CHECK_LIBRARY([zlib], [z], [compress])
            LDFLAGS="$LDFLAGS $ZLIB_LDFLAGS $ZLIB_LIBS"
            AC_MSG_NOTICE([retrying id3lib check with zlib flags])
            MT_CHECK_PACKAGE([id3lib], [id3/tag], [id3], [main])
            if test "x$ID3LIB_STATUS" = xyes; then
                ID3LIB_LIBS="$ID3LIB_LIBS $ZLIB_LIBS"
                ID3LIB_LDFLAGS="$ID3LIB_LDFLAGS $ZLIB_LDFLAGS"
            fi
        else
            LDFLAGS="$LDFLAGS $ZLIB_LDFLAGS $ZLIB_LIBS"
            AC_MSG_NOTICE([retrying id3lib check with zlib flags])
            MT_CHECK_PACKAGE([id3lib], [id3/tag], [id3], [main])
            if test "x$ID3LIB_STATUS" = xyes; then
                ID3LIB_LIBS="$ID3LIB_LIBS $ZLIB_LIBS"
                ID3LIB_LDFLAGS="$ID3LIB_LDFLAGS $ZLIB_LDFLAGS"
            fi
        fi

        AC_SUBST(ID3LIB_LIBS)
        AC_SUBST(ID3LIB_LDFLAGS)
    fi
fi

if test "x$ID3LIB_STATUS" = xyes; then
   CFLAGS="$CFLAGS $ID3_CFLAGS"
   CXXFLAGS="$CXXFLAGS $ID3_CFLAGS"
   LDFLAGS="$LDFLAGS $ID3LIB_LDFLAGS $ID3LIB_LIBS"
   AC_LANG_SAVE
   AC_LANG_CPLUSPLUS
   AC_MSG_CHECKING([for album art support in id3lib])
   AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [
                #include <id3/tag.h>
                #include <id3/misc_support.h>
            ],
            [
                (void)ID3_HasPicture(0);
            ]
        )],
        [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_ID3_ALBUMART], [1], 
                      [this version of id3lib has album art support])
        ],
        [
            AC_MSG_RESULT([no])
        ])
   AC_LANG_RESTORE
else
    if (test "x$ID3LIB_OPTION_REQUESTED" = xyes) &&
       (test "x$ID3LIB_OPEION_ENABLED" = xyes); then
        if test "x$TAGLIB_STATUS" = xyes; then
            AC_MSG_ERROR([Please select either taglib or id3lib, but not both.])
        else
            AC_MSG_ERROR(unable to configure id3lib support)
        fi
    else
        ID3LIB_STATUS=disabled
    fi
fi

LDFLAGS="$LDFLAGS_SAVE"
LIBS="$LIBS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"
######## curl

CURL_PROG_FOUND=0
CURL_LIBS=
CURL_CFLAGS=
CURL_OK=no

if ((test "x$YOUTUBE_OPTION_ENABLED" = xyes) && (test "x$YOUTUBE_OPTION_REQUESTED" = xyes) && (test $USE_CURL -eq 0) && (test $CURL_REQUESTED -eq 1)); then
    AC_MSG_ERROR([You enabled YouTube but disabled curl, however curl is required by the YouTube feature])
fi

if ((test "x$YOUTUBE_OPTION_ENABLED" != xyes) && (test "x$SOPCAST_OPTION_ENABLED" != xyes) && (test "x$EXTERNAL_TRANSCODING_OPTION" != xyes)); then
    if test $USE_CURL -eq 1; then
        if test $CURL_REQUESTED -eq 1; then
            AC_MSG_ERROR([You enabled curl support but the external transcoding, YouTube and SopCast features are disabled - curl is not needed without it])
        else
            CURL_OK=no
            USE_CURL=0
        fi
    fi
fi

if (((test "x$YOUTUBE_OPTION_ENABLED" = xyes) || (test "x$SOPCAST_OPTION_ENABLED" = xyes) || (test "x$EXTERNAL_TRANSCODING_OPTION" = xyes)) && (test $USE_CURL -eq 1)); then
    AC_MSG_NOTICE([Checking for curl (needed for extended external transcoding/YouTube/SopCast support)])
    CURL_OK=yes
    if test -n "$CURL_SEARCH_CONFIG"; then
        AC_MSG_NOTICE([You specified $CURL_SEARCH_CONFIG for curl-config])
        
        if test -f "$CURL_SEARCH_CONFIG"; then 
            curlconfig="$CURL_SEARCH_CONFIG"
        else
            curlconfig="no"
        fi
        
        if test "x$curlconfig" = xno; then
            AC_MSG_ERROR([$CURL_SEARCH_CONFIG not found])
        else
            CURL_TEST=`${curlconfig} --version 2>/dev/null`
            if test -z "$CURL_TEST"; then
                AC_MSG_ERROR([${curlconfig} could not be executed or returned invalid values])
            fi
            CURL_PROG_FOUND=1
        fi
    else
        AC_PATH_PROG(curlconfig, curl-config, no)
        if test "x$curlconfig" = xno; then
            unset ac_cv_path_curlconfig
            AC_PATH_PROG(curlconfig, curl-config, no, $SEARCH_DIR_PROGS)
            if test "x$curlconfig" = xno; then
                CURL_OK=missing
                AC_MSG_RESULT([curl-config not found, please install the curl-devel package])
            else
                CURL_PROG_FOUND=1
            fi
        else
            CURL_PROG_FOUND=1
        fi
   fi 

   if test $CURL_PROG_FOUND -eq 1 ; then
        AC_MSG_CHECKING(curl cflags)
        CURL_CFLAGS=`${curlconfig} --cflags`
        AC_MSG_RESULT($CURL_CFLAGS)

        AC_MSG_CHECKING(curl libraries)
        CURL_LIBS=`${curlconfig} --libs`
        AC_MSG_RESULT($CURL_LIBS)
    fi
fi

if test "x$CURL_OK" = xyes; then
    CPPFLAGS="$CPPFLAGS $CURL_CFLAGS"
    CXXFLAGS="$CXXFLAGS $CURL_CFLAGS"
    AC_CHECK_HEADERS([curl/curl.h],
        [],
        [CURL_OK=missing]
    )
fi

if test "x$CURL_OK" = xyes; then
    LDFLAGS="$CURL_LIBS"
    AC_CHECK_LIB(curl, curl_easy_init, [],
        [CURL_OK=missing]
    )
fi

if test "x$CURL_OK" = xyes; then
    AC_DEFINE([HAVE_CURL], [1], [curl library presence])
    AC_SUBST(CURL_CFLAGS)
    AC_SUBST(CURL_LIBS)
else
    if test "x$YOUTUBE_OPTION_REQUESTED" = xyes; then
        AC_MSG_ERROR([unable to configure curl which is required for YouTube support!])
    fi
    if test "x$EXTERNAL_TRANSCODING_OPTION" = xyes; then
        if ((test $USE_CURL -eq 1) && (test $CURL_REQUESTED -eq 0)); then
            AC_MSG_WARN([unable to configure curl, external transcoding will have limited functionality!])
        fi

        if ((test $USE_CURL -eq 1) && (test $CURL_REQUESTED -eq 1)); then
            AC_MSG_ERROR([You enabled curl support but the curl library could not be configured])
        fi

    fi

    if (test "x$YOUTUBE_OPTION_ENABLED" != xyes) && (test "x$EXTERNAL_TRANSCODING_OPTION" = !xyes) && (test "x$SOPCAST_OPTION_ENABLED" != xyes); then
        CURL_CFLAGS=
        CURL_LIBS=
    fi
fi
CPPFLAGS="$CPPFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"

######### extractor or ffmpeg

FFMPEG_REQUESTED=no
EXTRACTOR_REQUESTED=no

AC_ARG_ENABLE(ffmpeg,
        AC_HELP_STRING([--enable-ffmpeg], [compile with ffmpeg support (default: yes, preferred over extractor)]),
  [
     FFMPEG_EN=$enableval 
     if test "x$enableval" = xno; then
        FFMPEG_OK=disabled
     else
        FFMPEG_OK=yes
        FFMPEG_REQUESTED=yes
     fi
  ],
  [
    FFMPEG_OK=yes
  ]
)

AC_ARG_ENABLE(libextractor,
        AC_HELP_STRING([--enable-libextractor], [compile with libextractor support (default: no)]),
  [
     EXTRACTOR_EN=$enableval 
     if test "x$enableval" = xno; then
        EXTRACTOR_OK=disabled
     else
        EXTRACTOR_OK=yes
       EXTRACTOR_REQUESTED=yes
     fi
  ],
  [
    EXTRACTOR_OK=disabled
  ]
)

if ((test "x$EXTRACTOR_REQUESTED" = xno) && (test "x$FFMPEG_REQUESTED" = xyes)) ; then
    EXTRACTOR_OK=disabled
    if test "x$EXTRACTOR_REQUESTED" = xyes; then
        AC_MSG_WARN([libextractor disabled])
    fi
elif ((test "x$EXTRACTOR_REQUESTED" = xyes) && (test "x$FFMPEG_REQUESTED" = xno)) ; then
    FFMPEG_OK=disabled
    AC_MSG_WARN([ffmpeg disabled])
elif ((test "x$EXTRACTOR_REQUESTED" = xyes) && (test "x$FFMPEG_REQUESTED" = xyes)) ; then
    AC_MSG_ERROR([Please select either ffmpeg or libextractor, but not both.])
fi

######## ffmpeg

AC_LANG_SAVE
AC_LANG_C

if test "x$FFMPEG_OK" = xyes; then
    if test -n "$FFMPEG_SEARCH_HEADERS" ; then
        CPPFLAGS="-I$FFMPEG_SEARCH_HEADERS"
        AC_CHECK_HEADER($FFMPEG_SEARCH_HEADERS/ffmpeg/avformat.h,
                [
                    FFMPEG_CXXFLAGS="-I$FFMPEG_SEARCH_HEADERS"
                ],
                [
                    AC_MSG_ERROR([ffmpeg headers not found in requested location $FFMPEG_SEARCH_HEADERS])
                ],
                [
                    #define __STDC_CONSTANT_MACROS
                    #include <stdint.h>
                ]
        )
    else 
        AC_CHECK_HEADER(ffmpeg/avformat.h,
            [],
            [
                CPPFLAGS="-I$SEARCH_DIR_HEADERS"
                unset ac_cv_header_ffmpeg_avformat_h
                AC_CHECK_HEADER($SEARCH_DIR_HEADERS/ffmpeg/avformat.h,
                    [
                        FFMPEG_CXXFLAGS="-I$SEARCH_DIR_HEADERS"
                    ],
                    [
                        FFMPEG_OK=missing
                    ],
                    [
                        #define __STDC_CONSTANT_MACROS
                        #include <stdint.h>
                    ]
                )
            ],
            [
                #define __STDC_CONSTANT_MACROS
                #include <stdint.h>
            ]
        )
    fi
fi
 
unset LIBS
if test "x$FFMPEG_OK" = xyes; then
    if test -n "$FFMPEG_SEARCH_LIBS" ; then
        LDFLAGS="-L$FFMPEG_SEARCH_LIBS $ADD_PTHREAD_CFLAGS"
        AC_CHECK_LIB(avformat, av_register_all,
                [
                    AC_CHECK_LIB(avutil, av_log_set_callback,
                        [
                            FFMPEG_LIBS="-L$FFMPEG_SEARCH_LIBS -lavformat -lavutil -lz"
                        ],
                        [
                            AC_MSG_ERROR([avutil libraries not found in requested path $FFMPEG_SEARCH_LIBS])
                        ],
                        [ -lavcodec -lavutil -lz $ADD_PTHREAD_CFLAGS ])
                ],
                [
                    AC_MSG_ERROR([avformat libraries not found in requested path $FFMPEG_SEARCH_LIBS])
                ],
                [ -lavcodec -lavutil -lz $ADD_PTHREAD_CFLAGS ]
        )
    else
        AC_CHECK_LIB(avformat, av_register_all,
            [
                AC_CHECK_LIB(avutil, av_log_set_callback,
                    [
                        FFMPEG_LIBS="-lavformat -lavutil"
                    ],
                    [
                        LDFLAGS="-L$SEARCH_DIR_LIBS"
                        unset ac_cv_lib_avutil_av_log_set_callback
                        AC_CHECK_LIB(avutil, av_log_set_callback,
                            [
                                FFMPEG_LIBS="-L$SEARCH_DIR_LIBS -lavformat -lavutil -lz"
                            ],
                            [
                               FFMPEG_OK=missing
                            ],
                            [ -lavcodec -lavutil -lz $ADD_PTHREAD_CFLAGS ])
                    ])
            ],
            [
                LDFLAGS="-L$SEARCH_DIR_LIBS $ADD_PTHREAD_CFLAGS"
                unset ac_cv_lib_avformat_av_register_all
                AC_CHECK_LIB(avformat, av_register_all,
                    [
                        unset ac_cv_lib_avutil_av_log_set_callback
                        AC_CHECK_LIB(avutil, av_log_set_callback,
                            [
                                FFMPEG_LIBS="-L$SEARCH_DIR_LIBS -lavformat -lavutil -lz"
                            ],
                            [
                               FFMPEG_OK=missing
                            ],
                            [ -lavcodec -lavutil -lz $ADD_PTHREAD_CFLAGS ])
                    ],
                    [ FFMPEG_OK=missing ],
                    [ -lavcodec -lavutil -lz $ADD_PTHREAD_CFLAGS ]
                )
            ],
            [ -lavcodec -lavutil -lz $ADD_PTHREAD_CFLAGS ]
        )
    fi
fi

# last check, my Fedora 7 is somehow broken, and allthough the above tests
# succeed it still has unresolved symbols when linking

CXXFLAGS="$CXXFLAGS $FFMPEG_CXXFLAGS"
LDFLAGS="$LDFLAGS $FFMPEG_LIBS"
LIBS="$LIBS $FFMPEG_LIBS"

AC_LANG_RESTORE

if test "x$FFMPEG_OK" = xyes; then
    AC_MSG_CHECKING([ffmpeg linking / if extern "C" is needed])
    AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [
                #define __STDC_CONSTANT_MACROS
                #include <stdint.h>
                #include <ffmpeg/avformat.h>
            ],
            [
                av_register_all();
            ]
        )],
        [
            AC_MSG_RESULT([ok])
        ],
        [AC_LINK_IFELSE(
            [AC_LANG_PROGRAM(
                [
                    #define __STDC_CONSTANT_MACROS
                    #include <stdint.h>
                    extern "C" {
                        #include <ffmpeg/avformat.h>
                    }
                ],
                [
                    av_register_all();
                ]
            )],
            [
                AC_MSG_RESULT([ok])
                AC_DEFINE([FFMPEG_NEEDS_EXTERN_C], [1],[ffmpeg header include needs to be wrapped into extern "C"])
            ],
            [
                AC_MSG_RESULT([failed, check your ffmpeg installation])
                FFMPEG_OK=missing
            ])
        ]
   )
fi

LIBS="$LIBS_SAVE $LIBS"


if test "x$FFMPEG_OK" = xyes; then
    AC_DEFINE([HAVE_FFMPEG], [1], [FFMPEG library presence])
    AC_SUBST(FFMPEG_CXXFLAGS)
    AC_SUBST(FFMPEG_LIBS)
else
    if test "x$FFMPEG_EN" = xyes; then
        AC_MSG_ERROR(unable to configure ffmpeg support)
    fi
fi

CXXFLAGS="$CXXFLAGS_SAVE"
LDFLAGS="$LDFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"

######## extractor

#AC_ARG_ENABLE(libextractor,
#        AC_HELP_STRING([--enable-libextractor], [compile with libextractor support (default: no)]),
#  [
#     EXTRACTOR_EN=$enableval 
#     if test "x$enableval" = xno; then
#        EXTRACTOR_OK=disabled
#     else
#        EXTRACTOR_OK=yes
#     fi
#  ],
#  [
#    EXTRACTOR_OK=disabled
#  ]
#
#)
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
                AC_CHECK_HEADER($SEARCH_DIR_HEADERS/extractor.h,
                    [ EXTRACTOR_CXXFLAGS="-I$SEARCH_DIR_HEADERS" ],
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
                [ EXTRACTOR_LIBS="-L$EXTRACTOR_SEARCH_LIBS -lextractor" ],
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

MT_CHECK_OPTIONAL_PACKAGE([libexif], [disable], 
             [compile with libexif for exif thumbnail and metadata extraction], 
             [libexif/exif-content], [exif], [exif_data_new_from_file])

if test "x$LIBEXIF_STATUS" = xyes; then
    CFLAGS="$CFLAGS $LIBEXIF_CFLAGS"
    CXXFLAGS="$CXXFLAGS $LIBEXIF_CFLAGS"
    LDFLAGS="$LDFLAGS $LIBEXIF_LDFLAGS $LIBEXIF_LIBS"

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

    if test "x$EXIF_EGV_1" = xyes; then
        AC_DEFINE([EXIF_EGV_1], [1], [exif_entry_get_value() has 1 parameter])
    fi
    if test "x$EXIF_EGV_3" = xyes; then
        AC_DEFINE([EXIF_EGV_3], [1], [exif_entry_get_value() has 3 parameters])
    fi
fi

LDFLAGS="$LDFLAGS_SAVE"
CFLAGS="$CFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"

######## expat

MT_CHECK_REQUIRED_PACKAGE([expat], [expat], [expat], [XML_ParserCreate])

####### libmp4v2

MT_CHECK_OPTIONAL_PACKAGE([libmp4v2], [disable],
                 [libmp4v2 support for mp4 metadata extraction],
                 [mp4], [mp4v2], [MP4Read]) 

########## INOTIFY TOOLS

MT_OPTION([inotify], [disable], [inotify support for imporved autoscan], [], 
          [INOTIFY_STATUS=disabled])

if test "x$INOTIFY_OPTION_ENABLED" = xyes; then
    MT_CHECK_HEADER([inotify], [sys/inotify])
fi

if test "x$INOTIFY_STATUS" = xyes; then
    AC_CHECK_FUNCS([inotify_init], [], [INOTIFY_STATUS=missing])
fi

if test "x$INOTIFY_STATUS" = xyes; then
dnl The check below was inspired by configure.ac from the inotify tools
dnl package, see the "Acknowledgements" section in our README file for more 
dnl information.
    CXXFLAGS="$CXXFLAGS $INOTIFY_CFLAGS"
    CFLAGS="$CFLAGS $INOTIFY_CFLAGS"
    AC_MSG_CHECKING([whether sys/inotify.h works])
    AC_RUN_IFELSE(
        AC_LANG_PROGRAM([[#include <sys/inotify.h>]],
                         [[return (-1 == inotify_init());]]
                       ),
                [
                    AC_MSG_RESULT([yes]); 
                    AC_DEFINE([SYS_INOTIFY_H_OK],[1],[sys/inotify.h exists and works on this system])],
                [
                    AC_MSG_RESULT([no, using own inotify headers])
                    AC_MSG_CHECKING([whether inotify-nosys.h works on this system])
                    AC_RUN_IFELSE(
                        AC_LANG_PROGRAM([[#include "src/inotify-nosys.h"]],
                         [[return (-1 == inotify_init());]]
                        ),
                        [
                            AC_MSG_RESULT([yes]);
                        ],
                        [
                            if ((test "x$INOTIFY_OPTION_REQUESTED" != xyes) &&
                                (test "x$INOTIFY_OPTIEON_ENABLED" = xyes)); then
                                AC_MSG_RESULT([no, disabling inotify support])
                            elif ((test "x$INOTIFY_OPTION_REQUESTED" != xyes) &&
                               (test "x$INOTIFY_OPTIEON_ENABLED" != xyes)); then
                                AC_MSG_RESULT([no])
                            fi
                                INOTIFY_STATUS=no
                        ],
                        [
                            if ((test "x$INOTIFY_OPTION_REQUESTED" = xyes) &&
                                (test "x$INOTIFY_OPTIEON_ENABLED" = xyes)); then
                                AC_MSG_RESULT([selecting yes for cross compiling]) 
                                INOTIFY_STATUS=yes
                            else
                                AC_MSG_RESULT([selecting no for cross compiling]) 
                                INOTIFY_STATUS=no
                            fi
                        ])

                ],
                [
                    if ((test "x$INOTIFY_OPTION_REQUESTED" = xyes) &&
                        (test "x$INOTIFY_OPTIEON_ENABLED" = xyes)); then
                       AC_MSG_RESULT([selecting yes for cross compiling]) 
                       INOTIFY_STATUS=yes
                    else
                       AC_MSG_RESULT([selecting no for cross compiling]) 
                       INOTIFY_STATUS=no
                    fi
                ])
fi

if test "x$INOTIFY_STATUS" = xyes; then
    AC_DEFINE([HAVE_INOTIFY], [1], [inotify presence])
    AC_SUBST(INOTIFY_CFLAGS)
else
    if ((test "x$INOTIFY_OPTION_REQUESTED" = xyes) &&
        (test "x$INOTIFY_OPTIEON_ENABLED" = xyes)); then
        AC_MSG_NOTICE([enabling runtime inotify detection])
        # if system inotify header exists, go with it, otherwise take our own
        if test "x$ac_cv_header_sys_inotify_h" = xyes; then
             AC_DEFINE([SYS_INOTIFY_H_OK],[1],[sys/inotify.h exists])
        fi
        AC_DEFINE([HAVE_INOTIFY], [1], [inotify presence])
        AC_SUBST(INOTIFY_CFLAGS)
        INOTIFY_STATUS=yes
    fi
fi

LDFLAGS="$LDFLAGS_SAVE"
CPPFLAGS="$CPPFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"

FFMPEG_THUMBNAILER_PKGCONFIG_FOUND=0
if test "x$FFMPEG_THUMBNAILER" = xyes; then
    if test -n "$FFMPEG_THUMBNAILER_SEARCH_HEADERS" ; then
        CPPFLAGS="-I$FFMPEG_THUMBNAILER_SEARCH_HEADERS"
        AC_CHECK_HEADER($FFMPEG_THUMBNAILER_SEARCH_HEADERS/libffmpegthumbnailer/videothumbnailerc.h,
                [
                    FFMPEG_THUMBNAILER_CXXFLAGS="-I$EXIF_SEARCH_HEADERS"
                ],
                [
                    AC_MSG_ERROR([ffmpegthumbnailer headers not found in requested location $FFMPEG_THUMBNAILER_SEARCH_HEADERS])
                ]
        )
    else 
        PKG_PROG_PKG_CONFIG
        if test -n "$PKG_CONFIG" ; then
            PKG_CHECK_MODULES(FFMPEG_THUMBNAILER, libffmpegthumbnailer,
                    [
                        FFMPEG_THUMBNAILER_PKGCONFIG_FOUND=1
                        AC_MSG_NOTICE([ffmpegthumbnailer cflags $FFMPEG_THUMBNAILER_CFLAGS])
                        AC_MSG_NOTICE([ffmpegthumbnailer libs $FFMPEG_THUMBNAILER_CFLAGS])
                    ],
                    [
                        FFMPEG_THUMBNAILER_PKGCONFIG_FOUND=0
                    ])
        fi
    fi
fi

if test "x$FFMPEG_THUMBNAILER" = xyes; then
    if test $FFMPEG_THUMBNAILER_PKGCONFIG_FOUND -eq 1; then
            CFLAGS="$CFLAGS $FFMPEG_THUMBNAILER_CFLAGS"
            CPPFLAGS="$CFLAGS $FFMPEG_THUMBNAILER_CFLAGS"
            AC_CHECK_HEADER(libffmpegthumbnailer/videothumbnailerc.h,
                [
                    LDFLAGS="$LDFLAGS $FFMPEG_THUMBNAILER_LDFLAGS"
                    AC_CHECK_LIB(ffmpegthumbnailer, create_thumbnailer,
                        [],
                        [
                            AC_MSG_WARN([could not check libffmpegthumbnailer using pkg-config values])
                        ])

                ],
                [
                    AC_MSG_WARN([could not check videothumbnailerc.h header using pkg-config values])
                ])
    fi
            AC_CHECK_HEADER(libffmpegthumbnailer/videothumbnailerc.h,
                [],
                [
                    unset ac_cv_header_libffmpegthumbnailer_videothumbnailerc_h
                    CPPFLAGS="-I$SEARCH_DIR_HEADERS"
                    AC_CHECK_HEADER(libffmpegthumbnailer/videothumbnailerc.h,
                        [
                            FFMPEG_THUMBNAILER_CXXFLAGS = "-I$SEARCH_DIR_HEADERS"
                        ],
                        [
                            FFMPEG_THUMBNAILER=missing
                        ]) 
                ])

fi

MT_OPTION([tombdebug], [enable], [enable debug output for MediaTomb],
          [
            AC_DEFINE(LOG_TOMBDEBUG, 1, [if defined compile with debug log output])
          ],
          [])

MT_OPTION([upnpdebug], [enable], [enable debug output for TombUPnP/libupnp],
          [
            AC_DEFINE(UPNP_HAVE_DEBUG, 1, [see upnpconfig.h])
            AC_DEFINE(DEBUG, 1, [Define to 1 to compile debug code])
          ],
          [])

if (test "x$TOMBDEBUG_OPTION_ENABLED" != xyes) && 
   (test "x$UPNPDEBUG_OPTION_ENABLED" != xyes); then
    AC_DEFINE(NDEBUG, 1, [make sure there are no asserts in the retail build])
fi

MT_OPTION([log], [disable], [disable all console output],
          [
            AC_DEFINE(LOG_ENABLED, 1, [if defined compile with log output])
          ],
          [])

eval PACKAGE_DATADIR="${datadir}/${PACKAGE}"
eval PACKAGE_DATADIR="${PACKAGE_DATADIR}"
AC_DEFINE_UNQUOTED(PACKAGE_DATADIR, "$PACKAGE_DATADIR", [MediaTomb data directory])

if (test "x$YOUTUBE_OPTION_ENABLED" = xyes) || (test "x$SOPCAST_OPTION_ENABLED" = xyes); then
    AC_DEFINE(ONLINE_SERVICES, 1, [at least one online service is supported])
fi

if (test "x$YOUTUBE_OPTION_ENABLED" = xyes); then
    AC_DEFINE([YOUTUBE], [1], [Enable support for the YouTube service])
fi

if (test "x$SOPCAST_OPTION_ENABLED" = xyes); then
    AC_DEFINE([SOPCAST], [1], [Enable support for the SopCast service])
fi

###############
AC_CONFIG_FILES([
    Makefile
    build/Makefile
    doc/Makefile
    scripts/Makefile
    scripts/js/Makefile
    scripts/mediatomb-service-optware
    tombupnp/Makefile
    tombupnp/build/Makefile
    web/Makefile
    config/Makefile
    artwork/Makefile
    mediatomb.spec
])

AC_OUTPUT

echo
echo "CONFIGURATION SUMMARY ----"
#                        disabled
echo
echo "sqlite3               : $SQLITE3_STATUS"
echo "mysql                 : $MYSQL_STATUS"
##echo "postgresql          : $PGSQL_OK"
echo "libjs                 : $JS_OK"
echo "libmagic              : $LIBMAGIC_STATUS"
echo "inotify               : $INOTIFY_STATUS"
echo "libexif               : $LIBEXIF_STATUS"
echo "expat                 : $EXPAT_STATUS"
echo "id3lib                : $ID3LIB_STATUS"
echo "taglib                : $TAGLIB_STATUS"
echo "ffmpeg                : $FFMPEG_OK"
echo "libmp4v2              : $LIBMP4V2_STATUS"
echo "external transcoding  : $EXTERNAL_TRANSCODING_OPTION_ENABLED"
echo "curl                  : $CURL_OK"
echo "YouTube               : $YOUTUBE_OPTION_ENABLED"
echo "SopCast               : $SOPCAST_OPTION_ENABLED"
echo "libextractor          : $EXTRACTOR_OK"
echo "db-autocreate         : $DB_AUTOCREATE_OPTION_ENABLED"
        
if test "x$TOMBDEBUG_OPTION_ENABLED" = xyes; then
    echo "debug log             : $TOMBDEBUG_OPTION_ENABLED"
fi

if test "x$UPNPDEBUG_OPTIEON_ENABLED" = xyes; then
    echo "upnpdebug             : $UPNPDEBUG_OPTION_ENABLED"
fi

if test "x$STATIC_OPTION_ENABLED" = xyes; then
    echo "static build          : $STATIC_OPTION_ENABLED"
fi

echo

