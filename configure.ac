#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([OL_MAJOR_VER], [0])
m4_define([OL_MINOR_VER], [4])
m4_define([OL_MICRO_VER], [3])
m4_define([OL_BUILD], [])
m4_define(OL_BUILD_VER,
          m4_esyscmd([if test x]OL_BUILD[ == x-git; then git log -1 --format=format:%ad --date=short | sed 's/-//g'; fi]))

m4_define([OL_VERSION], [OL_MAJOR_VER.OL_MINOR_VER.OL_MICRO_VER[]OL_BUILD[]OL_BUILD_VER])
AC_INIT([osdlyrics], [OL_VERSION], [http://code.google.com/p/osd-lyrics/issues/list])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/ol_main.c])
AC_CONFIG_HEADERS([config.h])
AC_DEFINE([PROGRAM_NAME], ["OSD Lyrics"], [The full name of the program to display])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h libintl.h limits.h locale.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# automake 1.12 seems to require this, but automake 1.11 doesn't recognize it
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AM_PROG_CC_C_O
AM_PROG_AR
AC_PROG_RANLIB
AC_CONFIG_FILES([ po/Makefile.in
	Makefile
	lib/Makefile
        lib/chardetect/Makefile
	dailydeb.sh
	src/Makefile
	src/tests/Makefile
	data/Makefile
])

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

dnl MPD support
AC_ARG_ENABLE([mpd],
  [AS_HELP_STRING([--disable-mpd], [Disables MPD support. You won't need libmpd if you use this option])],
  [],
  [enable_mpd=yes])

AC_ARG_ENABLE([xmms2],
  [AS_HELP_STRING([--disable-xmms2], [Disables XMMS2 support. You won't need libxmmsclient if you use this option])],
  [],
  [enable_xmms2=yes])

AC_ARG_ENABLE([amarok1],
  [AS_HELP_STRING([--enable-amarok1], [Enables Amarok 1.4 support])],
  [],
  [enable_amarok1=no])

AS_IF([test "x$enable_amarok1" != xno],
  [AC_DEFINE([ENABLE_AMAROK1], [1], [Define if compile to support Amarok 1.4])])

# Checks for libraries.
AC_CHECK_LIB([dl], [exp])
PKG_CHECK_MODULES([GTK2], [gtk+-2.0 >= 2.20.0,
                          gthread-2.0,
                          gmodule-export-2.0,
                          gio-2.0,
                          gio-unix-2.0])
PKG_CHECK_MODULES([X11], [x11])   dnl For XGrabKey
PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1])
PKG_CHECK_MODULES([CURL], [libcurl])
PKG_CHECK_MODULES([NOTIFY], [libnotify])
PKG_CHECK_EXISTS([libnotify >= 0.7],
  AC_DEFINE(HAVE_LIBNOTIFY_0_7, 1, [Have libnotify >= 0.7]))

PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.3.0])
MPD_CFLAGS=
MPD_LIBS=
AS_IF([test "x$enable_mpd" != "xno"],
  [PKG_CHECK_MODULES([MPD], [libmpd],
    [AC_DEFINE([ENABLE_MPD], [1], [Define if compile to support MPD])
     #BUILD_MPD_SUPPORT=1
    ],
    [enable_mpd=no
     #BUILD_MPD_SUPPORT=0
     AC_MSG_WARN(
       [libmpd is not available, use --disable-mpd to disable MPD support])
    ])
  ],
  [BUILD_MPD_SUPPORT=0
   AC_SUBST(MPD_CFLAGS)
   AC_SUBST(MPD_LIBS)
  ]
)
XMMS2_CFLAGS=
XMMS2_LIBS=
AS_IF([test "x$enable_xmms2" != "xno"],
  [PKG_CHECK_MODULES([XMMS2], [xmms2-client],
    [AC_DEFINE([ENABLE_XMMS2], [1], [Define if compile to support XMMS2])
    ],
    [enable_xmms2=no
     AC_MSG_WARN(
       [xmms2-client is not available, use --disable-xmms2 to disable XMMS2 support])
    ])
  ],
  [BUILD_XMMS2_SUPPORT=0
   AC_SUBST(XMMS2_CFLAGS)
   AC_SUBST(XMMS2_LIBS)
  ]
)
#AM_CONDITIONAL(BUILD_MPD_SUPPORT, [test "x$enable_mpd" != "xno"])
#AC_SUBST(BUILD_MPD_SUPPORT)
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
AM_GLIB_GNU_GETTEXT

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
