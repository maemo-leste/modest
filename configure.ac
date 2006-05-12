dnl configure.ac for modest 
dnl Time-stamp: <2006-05-01 19:11:26 (djcb)>
dnl written by Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>
AC_INIT([modest],[0.0.9],[http://maemo.org])
AC_CONFIG_SRCDIR(src/modest-main.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)

AC_CONFIG_HEADERS([config.h])

if test x$prefix=xNONE; then
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

dnl # GLib/Gobject => mandatory
PKG_CHECK_MODULES(MODEST_GSTUFF,glib-2.0 >= 2.6 gobject-2.0) 
AC_SUBST(MODEST_GSTUFF_CFLAGS)
AC_SUBST(MODEST_GSTUFF_LIBS)

dnl # tinymail => mandatory
PKG_CHECK_MODULES(MODEST_TINYMAIL, libtinymail-1.0 libtinymail-camel-1.0 libtinymailui-1.0 libtinymailui-gtk-1.0 libtinymail-gnome-desktop-1.0 camel-1.2)
AC_SUBST(MODEST_TINYMAIL_CFLAGS)
AC_SUBST(MODEST_TINYMAIL_LIBS)

dnl # firefox-gtkmozembed => dependency from tinymail
PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_GNOME_DESKTOP, firefox-nss firefox-xpcom firefox-gtkmozembed libnm_glib glib-2.0 >= 2.8 gobject-2.0 libgnomeui-2.0 gtk+-2.0)
AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS)
AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

dnl # GConf => mandatory
PKG_CHECK_MODULES(MODEST_GCONF,gconf-2.0)
AC_SUBST(MODEST_GCONF_CFLAGS)
AC_SUBST(MODEST_GCONF_LIBS)

dnl # Gtk+ => mandatory
PKG_CHECK_MODULES(MODEST_GTK,gtk+-2.0 >= 2.6)
AC_SUBST(MODEST_GTK_CFLAGS)
AC_SUBST(MODEST_GTK_LIBS)

dnl # Glade
PKG_CHECK_MODULES(MODEST_GLADE,libglade-2.0,glade_support=yes,echo "no";glade_support=no)
AC_SUBST(MODEST_GLADE_CFLAGS)
AC_SUBST(MODEST_GLADE_LIBS)

dnl dnl build the Hildon UI?
PKG_CHECK_MODULES(MODEST_HILDON,hildon libglade-2.0 libtinymailui-gtk-1.0,hildon_support=yes,echo "no";hildon_support=no)
AC_SUBST(MODEST_HILDON_CFLAGS)				       
AC_SUBST(MODEST_HILDON_LIBS)

dnl now, determine what to build
AC_MSG_NOTICE([Determining what UI to build])
modest_ui="none"
if test "x$hildon_support" = "xyes"; then
      AC_SUBST(MODEST_UI_DIR,"hildon")
      AC_DEFINE([MODEST_ENABLE_HILDON_UI],1,[Enable Hildon UI])
      modest_ui="hildon"
elif test "x$glade_support" = "xyes"; then
      AC_SUBST(MODEST_UI_DIR,"gtk-glade")		 
      AC_DEFINE([MODEST_ENABLE_GTK_GLADE_UI],1,[Enable Gtk-Glade UI])
      modest_ui="gtk-glade"						
else 
      AC_SUBST(MODEST_UI_DIR,"gtk")		 
      AC_DEFINE([MODEST_ENABLE_GTK_UI],1,[Enable Plain Gtk+ UI])
      modest_ui="gtk"						
fi

AC_OUTPUT([
Makefile
src/Makefile
src/gtk/Makefile
src/gtk-glade/Makefile
src/hildon/Makefile
docs/Makefile
docs/reference/Makefile
])

echo
echo "modest configure results"
echo "-----------------------------------"
echo "User-interface       : $modest_ui"
echo ""
