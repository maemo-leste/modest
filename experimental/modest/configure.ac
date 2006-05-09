dnl configure.ac for modest 
dnl Time-stamp: <2006-04-23 20:45:53 (djcb)>
dnl written by Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>
AC_INIT([modest],[0.0.5],[http://maemo.org])
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

GTK_DOC_CHECK([1.0])

# GLib/Gobject => mandatory
PKG_CHECK_MODULES(MODEST_GSTUFF,glib-2.0 >= 2.6 gobject-2.0) 
AC_SUBST(MODEST_GSTUFF_CFLAGS)
AC_SUBST(MODEST_GSTUFF_LIBS)

# tinymail => mandatory
PKG_CHECK_MODULES(MODEST_TINYMAIL, libtinymail-1.0 libtinymail-camel-1.0 libtinymailui-1.0 libtinymailui-gtk-1.0)
AC_SUBST(MODEST_TINYMAIL_CFLAGS)
AC_SUBST(MODEST_TINYMAIL_LIBS)

# Gtk+
PKG_CHECK_MODULES(MODEST_GTK,gtk+-2.0 >= 2.6,gtk_support=yes,echo "no";gtk_support=no)
AC_SUBST(MODEST_GTK_CFLAGS)
AC_SUBST(MODEST_GTK_LIBS)

# GConf
PKG_CHECK_MODULES(MODEST_GCONF,gconf-2.0,gconf_support=yes,echo "no";gconf_support=no)
AC_SUBST(MODEST_GCONF_CFLAGS)
AC_SUBST(MODEST_GCONF_LIBS)

# Glade
PKG_CHECK_MODULES(MODEST_GLADE,libglade-2.0,glade_support=yes,echo "no";glade_support=no)
AC_SUBST(MODEST_GLADE_CFLAGS)
AC_SUBST(MODEST_GLADE_LIBS)

dnl build the Hildon UI?
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
dnl elif test "x$glade_support" = "xyes"; then
dnl      AC_SUBST(MODEST_UI_DIR,"gtk-glade")		 
dnl      AC_DEFINE([MODEST_ENABLE_GTK_GLADE_UI],1,[Enable Gtk-Glade UI])
dnl      modest_ui="gtk-glade"						
elif  test "x$gtk_support" = "xyes"; then
      AC_SUBST(MODEST_UI_DIR,"gtk")		 
      AC_DEFINE([MODEST_ENABLE_GTK_UI],1,[Enable Plain Gtk+ UI])
      modest_ui="gtk-plain"						
else
      AC_MSG_ERROR([No supported UI found. Please check your installation])
fi


AC_MSG_NOTICE([Determining what configuration system to support])
modest_conf="none"
if test "x$gconf_support"="xyes"; then
      AC_DEFINE([MODEST_ENABLE_GCONF],1,[Enable GConf])
      modest_conf="gconf"
else
      AC_MSG_ERROR([No supported configuration system found. Please check your installation])
fi
AM_CONDITIONAL(MODEST_ENABLE_GCONF,test "x$modest_conf" = "xgconf")

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
echo "Configuration-system : $modest_conf"
echo ""
