dnl configure.ac for modest 
dnl Time-stamp: <2006-07-20 17:01:42 (djcb)>
dnl written by Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>

AC_INIT([modest],[0.0.1],[http://maemo.org])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_SRCDIR(src/modest-main.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)

if test x$prefix = xNONE; then
   prefix=/usr/local
fi
AC_SUBST(prefix)

AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL

# check for perl
AC_CHECK_PROG([perl])
GTK_DOC_CHECK([1.0])

dnl # GLib/Gobject/Gtk/Gconf => mandatory
PKG_CHECK_MODULES(MODEST_GSTUFF,glib-2.0 >= 2.6 gobject-2.0 gtk+-2.0 >= 2.6 gconf-2.0 libgtkhtml-3.8 libglade-2.0 libtinymail-1.0 libtinymail-camel-1.0 libtinymailui-1.0 libtinymailui-gtk-1.0 camel-1.2) 
AC_SUBST(MODEST_GSTUFF_CFLAGS)
AC_SUBST(MODEST_GSTUFF_LIBS)

dnl now, determine what to build
AC_MSG_NOTICE([Determining what platform to build])
PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_MAEMO,libosso,[with_platform=hildon])

AC_ARG_WITH(platform,
[--with-platform=gtk		platform to build for [gtk]],
[case "${with_platform}" in
      gtk)    MODEST_PLATFORM_DIR=gtk;;
      gtk2)   MODEST_PLATFORM_DIR=gtk2;;
      hildon) MODEST_PLATFORM_DIR=hildon;;
      *)      MODEST_PLATFORM_DIR=${with_platform}
esac], [MODEST_PLATFORM_DIR=gtk
       with_platform=gtk])
AC_SUBST(MODEST_PLATFORM_DIR)
	
if test "x$with_platform" = "xhildon"; then
   PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_MAEMO,libtinymail-maemo-1.0 hildon-libs hildon-lgpl libosso)
   AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_CFLAGS)
   AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_LIBS) 
else
   PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_GNOME_DESKTOP,libtinymail-gnome-desktop-1.0 libnm_glib glib-2.0 >= 2.8)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)
fi

AM_CONDITIONAL(OLD_UI_STUFF, test x$with_platform != xgtk2)

AC_OUTPUT([
Makefile
src/Makefile
src/gtk/Makefile
src/gtk2/Makefile
src/hildon/Makefile
src/hildon/com.nokia.modest.service
src/hildon/modest.desktop.in
docs/Makefile
docs/reference/Makefile
])

echo
echo "modest configure results"
echo "-----------------------------------"
echo "Platform       : $with_platform"
echo ""
echo "NOTE: you should do a 'make clean' after changing platform (--with-platform=)"
echo ""
