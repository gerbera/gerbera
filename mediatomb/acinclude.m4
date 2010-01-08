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


# $1 with parameter/package name
# $2 library name
# $3 function name
# $4 "pass" if requested option failed
# 
# returns:
#   mt_$1_library_status
#   mt_$1_libs
#   mt_$1_ldflags

AC_DEFUN([MT_CHECK_LIBRARY_INTERNAL],
[
    mt_$1_arg_default=yes
    mt_$1_library_status=yes

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

    if test "$mt_$1_search_libs" ; then
        unset ac_cv_lib_$2_$3
        LDFLAGS="$LDFLAGS -L$mt_$1_search_libs"
        AC_CHECK_LIB($2, $3,
            [
                mt_$1_libs="-l$2"
                mt_$1_ldflags="-L$mt_$1_search_libs"
            ],
            [
                mt_$1_library_status=missing
                if test "$4" = "pass"; then
                    AC_MSG_NOTICE([$1 library not found in requested location $mt_$1_search_libs])
                else
                    AC_MSG_ERROR([$1 library not found in requested location $mt_$1_search_libs])
                fi
            ]
        )
    else
        unset ac_cv_lib_$2_$3
        AC_CHECK_LIB($2, $3,
            [
                mt_$1_libs="-l$2"
            ],
            [
                LDFLAGS="$LDFLAGS -L$MT_SEARCHPATH_LIBS"
                unset ac_cv_lib_$2_$3
                AC_CHECK_LIB($2, $3,
                    [
                        mt_$1_libs="-l$2"
                        mt_$1_ldflags="-L$MT_SEARCHPATH_LIBS" 
                    ],
                    [
                        mt_$1_library_status=missing
                    ]
                )
            ]
        )
    fi

    if test "x$mt_$1_library_status" != xyes; then
        mt_$1_libs=""
        mt_$1_ldflags=""
    fi

    LIBS=$LIBS_SAVE
    LDFLAGS=$LDFLAGS_SAVE
    CFLAGS=$CFLAGS_SAVE
    CXXFLAGS=$CXXFLAGS_SAVE
    CPPFLAGS=$CPPFLAGS_SAVE
])

# $1 with parameter / library name
# $2 header without .h extension
# $3 fail / pass
#
# returns:
#   mt_$1_header_status
#   mt_$1_cxxflags

AC_DEFUN([MT_CHECK_HEADER_INTERNAL],
[
    LIBS_SAVE=$LIBS
    LDFLAGS_SAVE=$LDFLAGS
    CFLAGS_SAVE=$CFLAGS
    CXXFLAGS_SAVE=$CXXFLAGS
    CPPFLAGS_SAVE=$CPPFLAGS

    mt_$1_header_status=yes

    AC_ARG_WITH($1-h,
        AC_HELP_STRING([--with-$1-h=DIR], [search for $1 headers in DIR]),
        [
            mt_$1_search_headers="$withval"
            AC_MSG_NOTICE([Will search for $1 headers in $withval])
        ]
    )

    if test "$mt_$1_search_headers" ; then
        unset translit(ac_cv_header_$2_h, `/.-', `___')
        CFLAGS="$CFLAGS -I${mt_$1_search_headers}"
        CXXFLAGS="$CXXFLAGS -I${mt_$1_search_headers}"
        CPPFLAGS="$CPPFLAGS -I${mt_$1_search_headers}"
        AC_CHECK_HEADER($mt_$1_search_headers/$2.h,
            [
                mt_$1_cxxflags="-I${mt_$1_search_headers}"
            ],
            [
                mt_$1_header_status=missing
                if test "$3" = "pass"; then
                    AC_MSG_NOTICE([$1 headers not found in requested location $mt_$1_search_headers])
                else
                    AC_MSG_ERROR([$1 headers not found in requested location $mt_$1_search_headers])
                fi
            ]
        )
    else
        unset translit(ac_cv_header_$2_h, `/.-', `___')
        AC_CHECK_HEADER($2.h,
            [],
            [
                CFLAGS="$CFLAGS -I$MT_SEARCHPATH_HEADERS"
                CXXFLAGS="$CXXFLAGS -I$MT_SEARCHPATH_HEADERS"
                CPPFLAGS="$CPPFLAGS -I$MT_SEARCHPATH_HEADERS"
                unset translit(ac_cv_header_$2_h, `/.-', `___')
                AC_CHECK_HEADER($MT_SEARCHPATH_HEADERS/$2.h,
                    [
                        mt_$1_cxxflags="-I${MT_SEARCHPATH_HEADERS}"
                    ],
                    [
                        mt_$1_header_status=missing
                    ]
                )
            ]
        )
    fi

    if test "x$mt_$1_header_status" != xyes; then
        mt_$1_cxxflags=""
    fi

    LIBS=$LIBS_SAVE
    LDFLAGS=$LDFLAGS_SAVE
    CFLAGS=$CFLAGS_SAVE
    CXXFLAGS=$CXXFLAGS_SAVE
    CPPFLAGS=$CPPFLAGS_SAVE
])


# $1 package name
# $2 header name (without .h extension)
# $3 library name
# $4 function name
# $5 "pass" if requestd options failed
#
# returns:
#   mt_$1_package_status
#   $1_CFLAGS
#   $1_LIBS
#   $1_LDFLAGS
   
AC_DEFUN([MT_CHECK_PACKAGE_INTERNAL],
[
    MT_CHECK_HEADER_INTERNAL([$1], [$2], [$5])
    mt_$1_package_status=${mt_$1_header_status}
  
    if test "x$mt_$1_package_status" = xyes; then 
        MT_CHECK_LIBRARY_INTERNAL([$1], [$3], [$4], [$5])
        mt_$1_package_status=${mt_$1_library_status}
    fi
    
    if test "x$mt_$1_package_status" = xyes; then
        translit($1, `a-z/.-', `A-Z___')_CFLAGS=${mt_$1_cxxflags}
        translit($1, `a-z/.-', `A-Z___')_LIBS=${mt_$1_libs}
        translit($1, `a-z/.-', `A-Z___')_LDFLAGS=${mt_$1_ldflags}
    fi 
])

# $1 option name
# $2 enable/disable
# $3 help text
# $4 action if enabled
# $5 action if disabled
#
# returns:
#   $1_OPTION_ENABLED
#   $1_OPTION_REQUESTED

AC_DEFUN([MT_OPTION],
[

    translit(mt_$1_option_enabled, `/.-', `___')=
    translit(mt_$1_option_requested, `/.-', `___')=no
    if test "x$2" = xdisable; then
        translit(mt_$1_option_enabled, `/.-', `___')=yes
    else
        translit(mt_$1_option_enabled, `/.-', `___')=no
    fi

    AC_ARG_ENABLE([$1],
        AC_HELP_STRING([--$2-$1], [$3]),
        [
            translit(mt_$1_option_enabled, `/.-', `___')=$enableval
            translit(mt_$1_option_requested, `/.-', `___')=yes
        ]
    )

    translit($1, `a-z/.-', `A-Z___')_OPTION_ENABLED=${translit(mt_$1_option_enabled,`/.-', `___')}
    translit($1, `a-z/.-', `A-Z___')_OPTION_REQUESTED=${translit(mt_$1_option_requested, `/.-', `___')}

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_OPTION_ENABLED)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_OPTION_REQUESTED)

    AS_IF([test "x${translit(mt_$1_option_enabled,`/.-', `___')}" = xyes], [$4], [$5])[]dnl
])

# $1 package name
# $2 enable/disable
# $3 enable/disable help string
# $4 header name (without .h)
# $5 library name
# $6 function name
# $7 "pass" on requested options
# returns substed:
#   $1_STATUS
#   $1_LDFLAGS
#   $1_LIBS
#   $1_CFLAGS

AC_DEFUN([MT_CHECK_OPTIONAL_PACKAGE], 
[
    mt_$1_status=yes

    MT_OPTION([$1], [$2], [$3],[],[])

    if test "x${translit($1, `a-z/.-', `A-Z___')_OPTION_ENABLED}" = xyes; then
        MT_CHECK_PACKAGE_INTERNAL([$1], [$4], [$5], [$6], [$7])
        mt_$1_status=${mt_$1_package_status}
    else
        mt_$1_status=disabled
    fi
    
    if ((test "x${translit($1, `a-z/.-', `A-Z___')_OPTION_ENABLED}" = xyes) &&
        (test "x${translit($1, `a-z/.-', `A-Z___')_OPTION_REQUESTED}" = xyes) &&
        (test "x$mt_$1_status" != xyes) && (test "$7" != "pass")); then
        AC_MSG_ERROR([unable to configure $1 support])
    fi

    if test "x$mt_$1_status" = xyes; then
        AC_DEFINE(translit(HAVE_$1, `a-z/.-', `A-Z___'), [1], [$1 library presence])
    fi
    
    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_status}

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LIBS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LDFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_CFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)
])

# $1 package name
# $2 header name (without .h)
# $3 library name
# $4 function name
#
# returns substed:
#   $1_STATUS
#   $1_LDFLAGS
#   $1_LIBS
#   $1_CFLAGS


AC_DEFUN([MT_CHECK_REQUIRED_PACKAGE],
[
    MT_CHECK_PACKAGE_INTERNAL($1, $2, $3, $4)
    if test "x$mt_$1_package_status" != xyes; then
        AC_MSG_ERROR([unable to configure required package $1])
    fi

    AC_DEFINE(translit(HAVE_$1, `a-z/.-', `A-Z___'), [1], [$1 library presence])
    
    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_package_status}

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_CFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LIBS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LDFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)
])

# $1 with parameter name
# $2 library name
# $3 function to check
AC_DEFUN([MT_CHECK_LIBRARY],
[
    MT_CHECK_LIBRARY_INTERNAL([$1], [$2], [$3])

    translit($1, `a-z/.-', `A-Z___')_LIBS=${mt_$1_libs}
    translit($1, `a-z/.-', `A-Z___')_LDFLAGS=${mt_$1_ldflags}
    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_library_status}
    
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LIBS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LDFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)
])

# $1 with parameter name
# $2 header to check
# $3 fail or pass in case a path parameter was specified and the header missing
#    empty: fail
AC_DEFUN([MT_CHECK_HEADER],
[
    MT_CHECK_HEADER_INTERNAL($1, $2, $3)
    
    translit($1, `a-z/.-', `A-Z___')_CFLAGS=${mt_$1_cxxflags}
    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_header_status}

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_CFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)
])

# $1 package name
# $2 config file name
# $3 headers
# $4 library name
# $5 function name
# $6 optional library command
#
#  returns substed:
#    $mt_$1_package_status
#    $1_LDFLAGS
#    $1_LIBS
#    $1_CFLAGS

AC_DEFUN([MT_CHECK_BINCONFIG_INTERNAL],
[
    LIBS_SAVE=$LIBS
    LDFLAGS_SAVE=$LDFLAGS
    CFLAGS_SAVE=$CFLAGS
    CXXFLAGS_SAVE=$CXXFLAGS
    CPPFLAGS_SAVE=$CPPFLAGS


    mt_$1_config=none
    mt_$1_package_status=yes

    AC_ARG_WITH($1-cfg,
        AC_HELP_STRING([--with-$1-cfg=$2], [absolute path/name of $2]),
        [
            mt_$1_search_config="$withval"
            AC_MSG_NOTICE([Will search for $1 config in $withval])
        ]
    )

    if test -n "$mt_$1_search_config"; then
        AC_MSG_NOTICE([You specified ${mt_$1_search_config} for $2])
        if test -f "$mt_$1_search_config"; then
            mt_$1_config=${mt_$1_search_config}
        else
            AC_MSG_ERROR([${mt_$1_search_config} not found])
        fi

        mt_$1_version=`${mt_$1_config} --version 2>/dev/null`
        if test -z "$mt_$1_version"; then
            AC_MSG_ERROR([${mt_$1_search_config} could not be executed or returned invalid values])
        fi
    else
        AC_PATH_PROG(mt_$1_config, $2, none)
        if test "x$mt_$1_config" = xnone; then
            unset ac_cv_path_mt_$1_config
            AC_PATH_PROG(mt_$1_config, $2, none, $MT_SEARCHPATH_PROGS)
            if test "x$mt_$1_config" = xnone; then
                mt_$1_package_status=missing
                AC_MSG_RESULT([$2 not found, please install the $1 devel package])
            fi
        fi
       
        mt_$1_version=`${mt_$1_config} --version 2>/dev/null`
        if test -z "$mt_$1_version"; then
            AC_MSG_NOTICE([${mt_$1_config} could not be executed or returned invalid values])
            mt_$1_package_status=missing
        fi
    fi
    if test "x$mt_$1_package_status" = xyes; then
        AC_MSG_CHECKING([$1 cflags])
        mt_$1_cxxflags=`${mt_$1_config} --cflags`
        AC_MSG_RESULT([$mt_$1_cxxflags])
        mt_$1_libs=
        AC_MSG_CHECKING([$1 libs])
        if test -z "$6";  then
            mt_$1_libs=`${mt_$1_config} --libs`
        else
            mt_$1_libs=`${mt_$1_config} $6`
        fi
        AC_MSG_RESULT([$mt_$1_libs])
    fi

    if test "x$mt_$1_package_status" = xyes; then
        CPPFLAGS="$CPPFLAGS $mt_$1_cxxflags"
        CXXFLAGS="$CXXFLAGS $mt_$1_cxxflags"
        CFLAGS="$CFLAGS $mt_$1_cxxflags"
        for mt_u_header in translit($3, `/.-', `___'); do
            unset ac_cv_header_${mt_u_header}
        done
        AC_CHECK_HEADERS($3, [], [mt_$1_package_status=missing])
    fi

    if test "x$mt_$1_package_status" = xyes; then
        LIBS="$mt_$1_libs $LIBS"
        if test -z "$4"; then
            unset ac_cv_func_$5
            AC_CHECK_FUNCS($5, [], [mt_$1_package_status=missing])
        else
            unset ac_cv_lib_$4_$5
            AC_CHECK_LIB($4, $5, [], [mt_$1_package_status=missing])
        fi
    fi

    if test "x$mt_$1_package_status" = xyes; then
        translit($1, `a-z/.-', `A-Z___')_CFLAGS=${mt_$1_cxxflags}
        translit($1, `a-z/.-', `A-Z___')_LIBS=${mt_$1_libs}
        translit($1, `a-z/.-', `A-Z___')_VERSION=${mt_$1_version}
    fi 


    LIBS=$LIBS_SAVE
    LDFLAGS=$LDFLAGS_SAVE
    CFLAGS=$CFLAGS_SAVE
    CXXFLAGS=$CXXFLAGS_SAVE
    CPPFLAGS=$CPPFLAGS_SAVE
])

# $1 package name
# $2 enable/disable
# $3 enable/disable help string
# $4 config file name
# $5 headers
# $6 library name
# $7 function name
# $8 custom lib parameter
#
# returns:
#   mt_$1_package_status
#   $1_CFLAGS
#   $1_LIBS
#   $1_VERSION

AC_DEFUN([MT_CHECK_OPTIONAL_PACKAGE_CFG],
[
    mt_$1_status=yes
    mt_$1_requested=no

    MT_OPTION([$1], [$2], [$3],[],[])

    if test "x${translit($1, `a-z/.-', `A-Z___')_OPTION_ENABLED}" = xyes; then
        MT_CHECK_BINCONFIG_INTERNAL($1, $4, $5, $6, $7, $8)
        mt_$1_status=${mt_$1_package_status}
    else
        mt_$1_status=disabled
    fi
 
    if ((test "x${translit($1, `a-z/.-', `A-Z___')_OPTION_ENABLED}" = xyes) &&
        (test "x${translit($1, `a-z/.-', `A-Z___')_OPTION_REQUESTED}" = xyes) &&
        (test "x$mt_$1_status" != xyes)); then
        AC_MSG_ERROR([unable to configure $1 support])
    fi
   
    if test "x$mt_$1_status" = xyes; then
        AC_DEFINE(translit(HAVE_$1, `a-z/.-', `A-Z___'), [1], [$1 library presence])
    fi

    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_status}

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_CFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LIBS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_VERSION)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)


#    AS_IF([test x"$mt_$1_status" = xyes], [$8], [$9])[]dnl
])

# $1 package name
# $2 config file name
# $3 headers
# $4 library name
# $5 function name
#
#  returns substed:
#    $mt_$1_package_status
#    $1_LDFLAGS
#    $1_LIBS
#    $1_CFLAGS

AC_DEFUN([MT_CHECK_REQUIRED_PACKAGE_CFG],
[
    MT_CHECK_BINCONFIG_INTERNAL($1, $2, $3, $4, $5)
    if test "x$mt_$1_package_status" != xyes; then
        AC_MSG_ERROR([unable to configure required package $1])
    fi
    
    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_package_status}
        
    AC_DEFINE(translit(HAVE_$1, `a-z/.-', `A-Z___'), [1], [$1 library presence])

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_CFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LIBS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_VERSION)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)
])

# $1 package name
# $2 config file name
# $3 headers
# $4 library name
# $5 function name
#
#  returns substed:
#    $mt_$1_package_status
#    $1_LDFLAGS
#    $1_LIBS
#    $1_CFLAGS


AC_DEFUN([MT_CHECK_PACKAGE_CFG],
[
    mt_$1_status=yes

    if test "x$mt_$1_status" = xyes; then
        MT_CHECK_BINCONFIG_INTERNAL($1, $2, $3, $4, $5)
        mt_$1_status=${mt_$1_package_status}
    fi
    
    if test "x$mt_$1_status" = xyes; then
        AC_DEFINE(translit(HAVE_$1, `a-z/.-', `A-Z___'), [1], [$1 library presence])
    fi

    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_status}

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_CFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LIBS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_VERSION)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)
])

# $1 package name
# $2 headers
# $3 library name
# $4 function name
#
#  returns substed:
#    $mt_$1_package_status
#    $1_LDFLAGS
#    $1_LIBS
#    $1_CFLAGS


AC_DEFUN([MT_CHECK_PACKAGE],
[
    mt_$1_status=yes

    if test "x$mt_$1_status" = xyes; then
        MT_CHECK_PACKAGE_INTERNAL($1, $2, $3, $4)
        mt_$1_status=${mt_$1_package_status}
    fi

    if test "x$mt_$1_status" = xyes; then
        AC_DEFINE(translit(HAVE_$1, `a-z/.-', `A-Z___'), [1], [$1 library presence])
    fi

    translit($1, `a-z/.-', `A-Z___')_STATUS=${mt_$1_status}

    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_CFLAGS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_LIBS)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_VERSION)
    AC_SUBST(translit($1, `a-z/.-', `A-Z___')_STATUS)
])


