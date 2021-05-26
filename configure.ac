#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([OL_MAJOR_VER], [0])
m4_define([OL_MINOR_VER], [5])
m4_define([OL_MICRO_VER], [5])
m4_define([OL_BUILD], [-git])
m4_define(OL_BUILD_VER,
          m4_esyscmd_s([if test 'x]OL_BUILD[' = 'x-git'; then (git log -1 --format=format:%ad --date=short 2>/dev/null || date -u +%Y%m%d) | sed 's/-//g'; fi]))

m4_define([OL_VERSION], [OL_MAJOR_VER.OL_MINOR_VER.OL_MICRO_VER[]OL_BUILD[]OL_BUILD_VER])
AC_INIT([osdlyrics], [OL_VERSION], [https://github.com/osdlyrics/osdlyrics/issues])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([src/ol_main.c])
AC_CONFIG_HEADERS([config.h])
AC_DEFINE([PROGRAM_NAME], ["OSD Lyrics"], [The full name of the program to display])
AC_SUBST([PROGRAM_NAME], ["OSD Lyrics"])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h libintl.h limits.h locale.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AM_PROG_CC_C_O
AC_PROG_RANLIB
AM_PATH_PYTHON([2.6])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
# AM_GNU_GETTEXT
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([ftruncate gettimeofday memset pathconf sqrt strcasecmp strchr strdup strerror strrchr strstr])
AM_ICONV

#define paths
OL_ICONDIR='${datadir}/${PACKAGE}/icons'
AC_SUBST(OL_ICONDIR)
OL_GUIDIR='${datadir}/${PACKAGE}'
AC_SUBST(OL_GUIDIR)

# gprof support
AC_ARG_WITH(gprof-support,
[  --with-gprof-support=DIR   Include gmon.out file renaming support
  --without-gprof-support    Don't do gmon.out file renaming (default)],
[ case "$withval" in
  yes)
    AC_MSG_RESULT(yes)
    AC_DEFINE([WITH_GPROF_SUPPORT], [], [Support gprof profiling])
    CFLAGS="-pg $CFLAGS"
    LDFLAGS="-pg $LDFLAGS"
    AC_MSG_WARN(--with-gprof-support called without argument - using default)
    gmondir="/tmp"
    AC_SUBST(gmondir)
    ;;
  no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT(yes)
    AC_DEFINE([WITH_GPROF_SUPPORT], [], [Support gprof profiling])
    CFLAGS="-pg $CFLAGS"
    LDFLAGS="-pg $LDFLAGS"
    gmondir="$withval"
    AC_SUBST(gmondir)
    ;;
  esac
])

# dev restrict support
AC_ARG_WITH(werror,
[  --with-werror   Build with -Wall -Werror],
[ case "$withval" in
  yes)
    AC_MSG_RESULT(yes)
    CFLAGS="-Wall -Werror $CFLAGS"
    ;;
  *)
    AC_MSG_RESULT(no)
    ;;
  esac
])

dnl app indicators in Ubuntu
APPINDICATOR_REQUIRED=0.0.7

AC_ARG_ENABLE(appindicator,
                        AS_HELP_STRING([--enable-appindicator[=@<:@no/auto/yes@:>@]],[Build support for application indicators ]),
                        [enable_appindicator=$enableval],
                        [enable_appindicator="auto"])

# Checks for libraries.
AC_CHECK_LIB([dl], [exp])
PKG_CHECK_MODULES([GTK2], [gtk+-2.0 >= 2.20.0,
                          gthread-2.0,
                          gmodule-export-2.0,
                          gio-2.0 >= 2.26,
                          gio-unix-2.0])
PKG_CHECK_MODULES([X11], [x11])   dnl For XGrabKey
PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1])
PKG_CHECK_MODULES([NOTIFY], [libnotify])
PKG_CHECK_EXISTS([libnotify >= 0.7],
  AC_DEFINE(HAVE_LIBNOTIFY_0_7, 1, [Have libnotify >= 0.7]))

dnl AppIndicator support for Ubuntu
if test x$enable_appindicator = xauto ; then
        PKG_CHECK_EXISTS([appindicator-0.1 >= $APPINDICATOR_REQUIRED],
                         enable_appindicator="yes",
                         enable_appindicator="no")
fi

if test x$enable_appindicator = xyes ; then
        PKG_CHECK_EXISTS([appindicator-0.1 >= $APPINDICATOR_REQUIRED],,
                         AC_MSG_ERROR([appindicator-0.1 is not installed]))
        PKG_CHECK_MODULES(APP_INDICATOR,
                        appindicator-0.1 >= $APPINDICATOR_REQUIRED)
        AC_SUBST(APP_INDICATOR_CFLAGS)
        AC_SUBST(APP_INDICATOR_LIBS)
        AC_DEFINE(HAVE_APP_INDICATOR, 1, [Have AppIndicator])
fi
AM_CONDITIONAL(HAVE_APP_INDICATOR, test x"$enable_appindicator" = xyes)

# i18n support from GNU gettext.
GETTEXT_PACKAGE=osdlyrics
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
IT_PROG_INTLTOOL([0.35.0])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])
m4_ifdef([AM_GNU_GETTEXT_REQUIRE_VERSION],
         [AM_GNU_GETTEXT_REQUIRE_VERSION([0.19])]
)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
        po/Makefile.in
        Makefile
        src/Makefile
        src/tests/Makefile
        data/Makefile
        daemon/Makefile
        python/__init__.py
        players/Makefile
        players/mpris1/Makefile
        players/mpris2/Makefile
        players/http/Makefile
        players/mpd/Makefile
        python/Makefile
        python/dbusext/Makefile
        lyricsources/Makefile
        lyricsources/xiami/Makefile
        lyricsources/megalobiz/Makefile
        lyricsources/netease/Makefile
        lyricsources/subtitles4songs/Makefile
        tools/Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([OSD Lyrics compilation options:])
        AC_MSG_NOTICE([ *        Version: $PACKAGE_VERSION])
        AC_MSG_NOTICE([ *         Prefix: $prefix])
if test "x$enable_appindicator" = "xyes"; then
        AC_MSG_NOTICE([ *   AppIndicator: yes])
else
        AC_MSG_NOTICE([ *   AppIndicator: no])
fi
