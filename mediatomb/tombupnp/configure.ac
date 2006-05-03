AC_INIT(tombupnp, 0.1, jin@mediatomb.org)
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(tombupnp, 0.1)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_PROG_CXX
AC_HEADER_STDC
AC_LANG_CPLUSPLUS

AC_LANG([C++])
# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_mutex_create])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ALLOCA
AC_PROG_GCC_TRADITIONAL
#AM_PROG_LIBTOOL
AC_PROG_RANLIB
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([ftime gethostname gettimeofday inet_ntoa memmove memset select socket strcasecmp strchr strdup strncasecmp strrchr strstr strtol uname])

AC_CONFIG_FILES([Makefile
                 build/Makefile
                ])

INCLUDE_DEVICE_APIS=1
AC_DEFINE([INCLUDE_DEVICE_APIS], [1], [Compile device API])
CFLAGS="$CFLAGS -DINCLUDE_DEVICE_APIS -DINTERNAL_WEB_SERVER"
AC_OUTPUT
