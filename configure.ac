dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_INIT([etisnoop], [2.0.0], [http://opendigitalradio.org/])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall tar-ustar foreign subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADER([config.h])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P

AX_CXX_COMPILE_STDCXX_11(noext,mandatory)

AC_PROG_CC_STDC
# Added for automake 1.13 on CentOS 7, obsolecent starting with automake 1.14
AM_PROG_CC_C_O

# std::thread requires pthread
AX_PTHREAD( [
            AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.])
            CLIBS="$PTHREAD_LIBS $LIBS"
            CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
            LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"
            CC="$PTHREAD_CC"], [AC_MSG_ERROR([requires pthread])] )

AC_CHECK_LIB([m], [sin])

AC_SEARCH_LIBS([NeAACDecDecode], [faad], [], [
  AC_MSG_ERROR([unable to find libfaad])
])

AM_CONDITIONAL([IS_GIT_REPO], [test -d '.git'])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo
echo "***********************************************"
echo " Configured etisnoop"
echo " Please run make"
echo
