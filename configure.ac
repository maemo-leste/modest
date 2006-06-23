dnl configure.ac for modest 
dnl Time-stamp: <2006-06-02 16:29:39 (djcb)>
dnl written by Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>
AC_INIT([modest],[0.0.9],[http://maemo.org])
AC_CONFIG_SRCDIR(src/modest-main.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)

AC_CONFIG_HEADERS([config.h])

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

dnl dnl build the Hildon UI?
PKG_CHECK_MODULES(MODEST_HILDON,hildon-libs,hildon_support=yes,echo "no";hildon_support=no)

dnl now, determine what to build
AC_MSG_NOTICE([Determining what UI to build])
modest_ui="none"
if test "x$hildon_support" = "xyes"; then
      AC_MSG_NOTICE([Looks like Hildon])

      AC_SUBST(MODEST_UI_DIR,"hildon-libs")
      AC_DEFINE([MODEST_ENABLE_HILDON_UI],1,[Enable Hildon UI])
      PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_MAEMO,libtinymail-maemo-1.0 hildon-libs hildon-lgpl libosso)
      AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_CFLAGS)
      AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_LIBS) 
      modest_ui="hildon"
else
      AC_MSG_NOTICE([Looks like Gnome/GTK+])

      AC_SUBST(MODEST_UI_DIR,"gtk-glade")		 
      AC_DEFINE([MODEST_ENABLE_GTK_GLADE_UI],1,[Enable Gtk-Glade UI])
      PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_GNOME_DESKTOP,libtinymail-gnome-desktop-1.0 libnm_glib glib-2.0 >= 2.8)
      AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS)
      AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)
      modest_ui="gtk-glade"						
fi

AM_CONDITIONAL(HILDON, test x$hildon_support = xyes)

dnl now, determine what to build
AC_MSG_NOTICE([Determining what UI to build])
modest_ui="none"
if test "x$hildon_support" = "xyes"; then
      AC_SUBST(MODEST_UI_DIR,"hildon")
      AC_DEFINE([MODEST_ENABLE_HILDON_UI],1,[Enable Hildon UI])
      modest_ui="hildon"
else 
      AC_SUBST(MODEST_UI_DIR,"gtk-glade")		 
      AC_DEFINE([MODEST_ENABLE_GTK_GLADE_UI],1,[Enable Gtk-Glade UI])
      modest_ui="gtk-glade"						
fi

AC_OUTPUT([
Makefile
src/Makefile
src/gtk/Makefile
src/gtk-glade/Makefile
src/hildon/Makefile
src/hildon/com.nokia.modest.service
src/hildon/modest.desktop.in
docs/Makefile
docs/reference/Makefile
])

echo
echo "modest configure results"
echo "-----------------------------------"
echo "User-interface       : $modest_ui"
echo ""
