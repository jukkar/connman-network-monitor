AC_PREREQ(2.60)
AC_INIT(libconnman-network-monitor, 0.1)
AM_INIT_AUTOMAKE([subdir-objects])

AM_CONFIG_HEADER(config.h)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PREFIX_DEFAULT(/usr)

AM_MAINTAINER_MODE
AC_SUBST(abs_top_srcdir)
AC_SUBST(abs_top_builddir)

PKG_PROG_PKG_CONFIG

COMPILER_FLAGS

AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_ISC_POSIX
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_LIBTOOL
AC_DISABLE_STATIC

PKG_CHECK_MODULES(GLIB, glib-2.0, [], [AC_MSG_ERROR([You need to install Glib])])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GOBJECT, gobject-2.0, [], [AC_MSG_ERROR([You need to install Gobject])])
AC_SUBST(GOBJECT_CFLAGS)
AC_SUBST(GOBJECT_LIBS)

PKG_CHECK_MODULES(DBUS, dbus-1, [], [AC_MSG_ERROR([You need to install DBus])])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(GIO, gio-2.0, [], [AC_MSG_ERROR([You need to install Gio])])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

AC_ARG_WITH(giomoduledir, AC_HELP_STRING([--with-giomoduledir=PATH],
	[path to GIO module directory]), [path_giomodule=${withval}],
		[path_giomodule="`$PKG_CONFIG --variable=giomoduledir gio-2.0`"])
if (test -z "${path_giomodule}"); then
	AC_MSG_ERROR([GIO module directory not set])
else
	GIO_MODULE_DIR="${path_giomodule}"
fi
AC_SUBST(GIO_MODULE_DIR)

AC_ARG_ENABLE([test],
		[AS_HELP_STRING([--enable-test], [Enable test programs])],
		[],[enable_test=no])
AM_CONDITIONAL([TEST], [test "x$enable_test" = "xyes"])

CONCFLAGS="-Wall -Werror -Wmissing-prototypes"
AC_SUBST(CONCFLAGS)

AC_OUTPUT(Makefile)
