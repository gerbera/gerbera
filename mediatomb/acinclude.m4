# ====================================================================
# ACX_PTHREAD
# ====================================================================
#
# This script has been downloaded from
# http://autoconf-archive.cryp.to/acx_pthread.html
#
# Copyright © 2006 Steven G. Johnson <stevenj@alum.mit.edu>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
# USA.
#
# As a special exception, the respective Autoconf Macro's copyright
# owner gives unlimited permission to copy, distribute and modify the
# configure scripts that are the output of Autoconf when processing the
# Macro. You need not follow the terms of the GNU General Public
# License when using or distributing such scripts, even though portions
# of the text of the Macro appear in them. The GNU General Public
# License (GPL) does govern all other use of the material that
# constitutes the Autoconf Macro.
#
# This special exception to the GPL applies to versions of the
# Autoconf Macro released by the Autoconf Macro Archive. When you make
# and distribute a modified version of the Autoconf Macro, you may
# extend this special exception to the GPL to apply to your modified
# version as well.

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
#      ... -mt is also the pthreads flag for HP/aCC
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthreads/-mt/
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthreads pthread -mt -pthread $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                pthread-config)
                AC_CHECK_PROG(acx_pthread_config, pthread-config, yes, no)
                if test x"$acx_pthread_config" = xno; then continue; fi
                PTHREAD_CFLAGS="`pthread-config --cflags`"
                PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
        AC_MSG_CHECKING([for joinable pthread attribute])
        attr_name=unknown
        for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
            AC_TRY_LINK([#include <pthread.h>], [int attr=$attr; return attr;],
                        [attr_name=$attr; break])
        done
        AC_MSG_RESULT($attr_name)
        if test "$attr_name" != PTHREAD_CREATE_JOINABLE; then
            AC_DEFINE_UNQUOTED(PTHREAD_CREATE_JOINABLE, $attr_name,
                               [Define to necessary symbol if this constant
                                uses a non-standard name on your system.])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
            *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
            *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
            PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with xlc_r or cc_r
        if test x"$GCC" != xyes; then
          AC_CHECK_PROGS(PTHREAD_CC, xlc_r cc_r, ${CC})
        else
          PTHREAD_CC=$CC
        fi
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD

# sets the search path which is used by MT_HAVE_PACKAGE
AC_DEFUN([MT_SET_SEARCHPATH],
[
    AC_REQUIRE([AC_CANONICAL_HOST])

    MT_SEARCHPATH="/usr/local"
    case $host in
        *-*-darwin*)
            MT_SEARCHPATH="/opt/local"
        ;;
    esac

    AC_ARG_WITH(search,
        AC_HELP_STRING([--with-search=DIR], [Additionally search for packages in DIR]),
        [
            MT_SEARCHPATH=$withval
            AC_MSG_NOTICE([Will also search for packages in ${MT_SEARCHPATH}])
        ]
    )

    MT_SEARCHPATH_HEADERS="${MT_SEARCHPATH}/include"
    MT_SEARCHPATH_LIBS="${MT_SEARCHPATH}/lib"
    MT_SEARCHPATH_PROGS="${MT_SEARCHPATH}/bin"

    AC_SUBST(MT_SEARCHPATH)
    AC_SUBST(MT_SEARCHPATH_HEADERS)
    AC_SUBST(MT_SEARCHPATH_LIBS)
    AC_SUBST(MT_SEARCHPATH_PROGS)
])


# $1 library name
# $2 function name

AC_DEFUN([MT_CHECK_LIBRARY],
[
    mt_$1_arg_default=yes
    mt_$1_ok=yes

    LIBS_SAVE=$LIBS
    LDFLAGS_SAVE=$LDFLAGS
    CFLAGS_SAVE=$CFLAGS
    CXXFLAGS_SAVE=$CXXFLAGS
    CPPFLAGS_SAVE=$CPPFLAGS

    AC_ARG_WITH($1-libs,
        AC_HELP_STRING([--with-$1-libs=DIR], [search for $1 libraries in DIR]),
        [
            mt_$1_search_libs="$withval"
            AC_MSG_NOTICE([Will search for $1 libs in $withval])
        ]
    )

    unset LIBS

    if test "x$mt_$1_ok" = xyes; then
        if test "$mt_$1_search_libs" ; then
            LDFLAGS="-L$mt_$1_search_libs"
            AC_CHECK_LIB($1, $2,
                [
                    mt_$1_libs="-l$1"
                    mt_$1_ldflags="-L$mt_$1_search_libs"
                ],
                [
                    AC_MSG_ERROR([$1 library not found in requested location $mt_$1_search_libs])
                ]
            )
        else
            AC_CHECK_LIB($1, $2,
                [
                    mt_$1_libs="-l$1"
                ],
                [
                    LDFLAGS="-L$MT_SEARCHPATH_LIBS"
                    unset ac_cv_lib_$1_$2
                    AC_CHECK_LIB($1, $2,
                        [
                            mt_$1_libs="-l$1"
                            mt_$1_ldflags="-L$MT_SEARCHPATH_LIBS" 
                        ],
                        [
                            mt_$1_ok=missing
                        ]
                    )
                ]
            )
        fi
    fi

    translit($1, `a-z', `A-Z')_STATUS=${mt_$1_ok}
    
    if test "x$mt_$1_ok" = xyes; then
        translit($1, `a-z', `A-Z')_LIBS=${mt_$1_libs}
        translit($1, `a-z', `A-Z')_LDFLAGS=${mt_$1_ldflags}
        AC_DEFINE(translit(HAVE_$1, `a-z', `A-Z'), [1], [$1 library presence])
    fi

    AC_SUBST(translit($1, `a-z', `A-Z')_LIBS)
    AC_SUBST(translit($1, `a-z', `A-Z')_LDFLAGS)
    AC_SUBST(translit($1, `a-z', `A-Z')_STATUS)

    LIBS=$LIBS_SAVE
    LDFLAGS=$LDFLAGS_SAVE
    CFLAGS=$CFLAGS_SAVE
    CXXFLAGS=$CXXFLAGS_SAVE
    CPPFLAGS=$CPPFLAGS_SAVE
])

# $1 package name
# $2 required/optional
# $3 enable/disable
# $4 enable/disable help string
# $5 header name (without .h)
# $6 library name
# $7 function name

AC_DEFUN([MT_CHECK_PACKAGE], 
[
    mt_$1_required=0
    mt_$1_arg_default=yes
    mt_$1_ok=yes
    mt_$1_enabled=no
    mt_$1_required=0

    LIBS_SAVE=$LIBS
    LDFLAGS_SAVE=$LDFLAGS
    CFLAGS_SAVE=$CFLAGS
    CXXFLAGS_SAVE=$CXXFLAGS
    CPPFLAGS_SAVE=$CPPFLAGS

    AC_ARG_WITH($1-h,
        AC_HELP_STRING([--with-$1-h=DIR], [search for $1 headers in DIR]),
        [
            mt_$1_search_headers="$withval"
            AC_MSG_NOTICE([Will search for $1 headers in $withval])
        ]
    )

    AC_ARG_WITH($1-libs,
        AC_HELP_STRING([--with-$1-libs=DIR], [search for $1 libraries in DIR]),
        [
            mt_$1_search_libs="$withval"
            AC_MSG_NOTICE([Will search for $1 libs in $withval])
        ]
    )

    if (test -n "$2") && (test "$2" = "required"); then
        mt_$1_required=1
    else
        if test -z "$3"; then
            AC_MSG_ERROR([MT Package macro requires default enable/disable parameter])
        fi
    fi

    AC_MSG_NOTICE([!!!!!!!! PACKAGE $1 is REQUIRED? ${mt_$1_required}])

    if test ${mt_$1_required} -eq 0; then
        AC_MSG_NOTICE([!!!!!!!! -> arg enable for package $1 ])
        AC_ARG_ENABLE([$1],
            AC_HELP_STRING([--$3-$1], [$4]),
            [
                mt_$1_enabled=$enableval
                if test "x$enableval" = xno; then
                    mt_$1_ok=disabled
                fi
            ]
        )
    else
        mt_$1_enabled=yes
    fi

    if test "x$mt_$1_ok" = xyes; then
        if test "$mt_$1_search_headers" ; then
            CFLAGS="$CFLAGS -I${mt_$1_search_headers}"
            CXXFLAGS="$CXXFLAGS -I${mt_$1_search_headers}"
            CPPFLAGS="$CPPFLAGS -I${mt_$1_search_headers}"
            AC_CHECK_HEADER($mt_$1_search_headers/$5.h,
                [
                    mt_$1_cxxflags="-I${mt_$1_search_headers}"
                ],
                [
                    AC_MSG_ERROR([$1 headers not found in requested location $mt_$1_search_headers])
                ]
            )
        else
            AC_CHECK_HEADER($5.h,
                [],
                [
                    CFLAGS="$CFLAGS -I$MT_SEARCHPATH_HEADERS"
                    CXXFLAGS="$CXXFLAGS -I$MT_SEARCHPATH_HEADERS"
                    CPPFLAGS="$CPPFLAGS -I$MT_SEARCHPATH_HEADERS"
                    unset ac_cv_header_$5_h
                    AC_CHECK_HEADER($MT_SEARCHPATH_HEADERS/$5.h,
                        [
                            mt_$1_cxxflags="-I${MT_SEARCHPATH_HEADERS}"
                        ],
                        [
                            mt_$1_ok=missing
                        ]
                    )
                ]
            )
        fi

    fi

    unset LIBS

    if test "x$mt_$1_ok" = xyes; then
        if test "$mt_$1_search_libs" ; then
            LDFLAGS="-L$mt_$1_search_libs"
            AC_CHECK_LIB($6, $7,
                [
                    mt_$1_libs="-l$6"
                    mt_$1_ldflags="-L$mt_$1_search_libs"
                ],
                [
                    AC_MSG_ERROR([$1 libraries not found in requested location $mt_$1_search_libs])
                ]
            )
        else
            AC_CHECK_LIB($6, $7,
                [
                    mt_$1_libs="-l$6"
                ],
                [
                    LDFLAGS="-L$MT_SEARCHPATH_LIBS"
                    unset ac_cv_lib_$6_$7
                    AC_CHECK_LIB($6, $7,
                        [
                            mt_$1_libs="-l$6"
                            mt_$1_ldflags="-L$MT_SEARCHPATH_LIBS" 
                        ],
                        [
                            mt_$1_ok=missing
                        ]
                    )
                ]
            )
        fi
    fi


    translit($1, `a-z', `A-Z')_STATUS=${mt_$1_ok}
    
    if test "x$mt_$1_ok" = xyes; then
        translit($1, `a-z', `A-Z')_CXXFLAGS=${mt_$1_cxxflags}
        translit($1, `a-z', `A-Z')_LIBS=${mt_$1_libs}
        translit($1, `a-z', `A-Z')_LDFLAGS=${mt_$1_ldflags}
        AC_DEFINE(translit(HAVE_$1, `a-z', `A-Z'), [1], [$1 library presence])
    else
        if test "x$mt_$1_enabled" = xyes; then
            AC_MSG_ERROR(unable to configure $1 support)
        fi
    fi

    AC_SUBST(translit($1, `a-z', `A-Z')_CXXFLAGS)
    AC_SUBST(translit($1, `a-z', `A-Z')_LIBS)
    AC_SUBST(translit($1, `a-z', `A-Z')_LDFLAGS)
    AC_SUBST(translit($1, `a-z', `A-Z')_STATUS)

    LIBS=$LIBS_SAVE
    LDFLAGS=$LDFLAGS_SAVE
    CFLAGS=$CFLAGS_SAVE
    CXXFLAGS=$CXXFLAGS_SAVE
    CPPFLAGS=$CPPFLAGS_SAVE
])

dnl
 dnl   if ${mt_$1_required} -eq 0; then
 dnl       AC_ARG_ENABLE([$1],
 dnl           AC_HELP_STRING([--enable-$1], [compile with $1 support (default: yes)]),
 dnl  [
 dnl      EXPAT_EN=$enableval 
 dnl      if test "x$enableval" = xno; then
 dnl          EXPAT_OK=disabled
 dnl      else
 dnl          EXPAT_OK=yes
 dnl      fi
 dnl  ],
 dnl  [
 dnl      EXPAT_OK=yes
 dnl  ]
 dnl   )
 dnl   fi

