AC_INIT([Moates Burn and Ostrich C++], [1.0], [kd.github@gmail.com],
	[burn_cc], [https://github.com/keith-daigle/moates])
AC_PREREQ([2.59])
	AM_INIT_AUTOMAKE([1.10 -Wall no-define])
	AC_CONFIG_HEADERS([config.h])
	AC_PROG_CXX
	AC_PROG_CC
	AC_CONFIG_FILES([Makefile])
	AC_CANONICAL_HOST
# Check for which host we are on and setup a few things
# specifically based on the host
	case $host_os in
	darwin* )
	AC_DEFINE(FREEBSD,1,[Description: Make #define FREEBSD 1])
	;;
	linux*)
        #This isn't really used, but here just in case
	AC_DEFINE(LINUX,1,[Description: Make #define LINUX 1])
	;;
	*bsd*)
	AC_DEFINE(FREEBSD,1,[Description: Make #define FREEBSD 1])
	;;
	*)
	#Default Case
	AC_MSG_ERROR([Your platform is not currently supported by this projects autoconf ])
	;;
	esac

# Checks for header files.
	AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
	AC_HEADER_STDBOOL

# Checks for library functions.
	AC_FUNC_MALLOC
	AC_CHECK_FUNCS([getpagesize memset])

	AC_OUTPUT
