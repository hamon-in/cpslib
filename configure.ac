#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cpslib], [0.1], [noufal@nibrahim.net.in])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([pslib_linux.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [round])

# Checks for header files.
AC_CHECK_HEADERS([mntent.h stdlib.h string.h sys/statvfs.h unistd.h utmp.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_GETMNTENT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getmntent memset strdup strncasecmp strndup strtol strtoul strtoull sysinfo])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
