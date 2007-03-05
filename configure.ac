# Copyright (c) 2006,2007 Nokia Corporation
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# * Neither the name of the Nokia Corporation nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

AC_INIT([modest],[0.0.1],[http://maemo.org])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_SRCDIR(src/modest-main.c)
AM_INIT_AUTOMAKE([dist-bzip2])

# Check for gettext support
QGETTEXT_PACKAGE=modest
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", ["Description"])

ALL_LINGUAS="en_GB gl nl"
#AM_GLIB_DEFINE_LOCALEDIR([MODEST_LOCALE_DIR])
AM_GLIB_GNU_GETTEXT

modestlocaledir='$prefix/${DATADIRNAME}/locale'
AC_SUBST(modestlocaledir)

if test x$prefix = xNONE; then
   prefix=/usr/local
fi
AC_SUBST(prefix)

AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL


# Option to enable debugging
AC_ARG_ENABLE(debug,
              [AC_HELP_STRING([  --enable-debug],[Debugging (default=no)])],
              [with_debug=yes], [with_debug=no])

if test "x$with_debug" == "xyes" ; then
        CFLAGS="$CFLAGS -ggdb -O0 -DDEBUG -Wall"
else
        CFLAGS="$CFLAGS -O2 -Wall"
fi


dnl # GLib/Gobject/Gtk/Gconf => mandatory
PKG_CHECK_MODULES(MODEST_GSTUFF,glib-2.0 >= 2.6 gobject-2.0 gtk+-2.0 >= 2.6 gconf-2.0 gnome-vfs-2.0 libgtkhtml-3.8 libtinymail-1.0 libtinymail-camel-1.0 libtinymailui-1.0 libtinymailui-gtk-1.0) 
AC_SUBST(MODEST_GSTUFF_CFLAGS)
AC_SUBST(MODEST_GSTUFF_LIBS)

dnl dnl now, determine what to build
AC_MSG_NOTICE([Determining what platform to build])
PKG_CHECK_MODULES(LIBOSSO,libosso,[with_platform=maemo],true)

AC_ARG_WITH(platform,
[--with-platform=gnome		platform to build for [gnome]],
[case "${with_platform}" in
       gnome)    MODEST_PLATFORM=gnome;;
       maemo) MODEST_PLATFORM=maemo;;
       *)      MODEST_PLATFORM=${with_platform}
esac], [MODEST_PLATFORM=gnome
               with_platform=gnome])
AC_SUBST(MODEST_PLATFORM)

if test "x$with_platform" = "xmaemo"; then
   PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_MAEMO,libtinymail-maemo-1.0 hildon-libs libosso libossomime osso-addressbook-1.0)
   AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_CFLAGS)
   AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_LIBS) 
 
  desktopentrydir=`$PKG_CONFIG osso-af-settings --variable=desktopentrydir`
  serviceentrydir=`$PKG_CONFIG dbus-glib-1 --variable=prefix`/share/dbus-1/services
  localedir=`$PKG_CONFIG osso-af-settings --variable=localedir`
  pixmapdir=`$PKG_CONFIG osso-af-settings --variable=hildonpixmapdir`
  icon_26x26dir=$datadir/icons/hicolor/26x26/hildon
  icon_34x34dir=$datadir/icons/hicolor/34x34/hildon
  icon_40x40dir=$datadir/icons/hicolor/40x40/hildon
  icon_50x50dir=$datadir/icons/hicolor/50x50/hildon
  icon_scalabledir=$datadir/icons/hicolor/scalable/hildon
  pluginlibdir=`$PKG_CONFIG hildon-control-panel --variable=plugindir`
  plugindesktopentrydir=`$PKG_CONFIG hildon-control-panel --variable=plugindesktopentrydir`

  # Define as variables in Makefiles
  AC_SUBST(desktopentrydir)
  AC_SUBST(serviceentrydir)
  AC_SUBST(localedir)
  AC_SUBST(pixmapdir)
  AC_SUBST(icon_26x26dir)
  AC_SUBST(icon_34x34dir)
  AC_SUBST(icon_40x40dir)
  AC_SUBST(icon_50x50dir)
  AC_SUBST(icon_scalabledir)
  AC_SUBST(pluginlibdir)
  AC_SUBST(plugindesktopentrydir)

  MODEST_PLATFORM_ID=2

else
   PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_GNOME_DESKTOP,libtinymail-gnome-desktop-1.0 libnm_glib glib-2.0 >= 2.8)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)
   MODEST_PLATFORM_ID=1
fi

dnl 1==>gnome, 2==>maemo
AC_SUBST(MODEST_PLATFORM_ID)





dnl ## setup the testing framework (ie., make check)
build_tests=false
AC_ARG_ENABLE(tests,
AC_HELP_STRING([--enable-tests],
	[Build tests (no, yes)]),
[case "${enableval}" in
      yes)
	build_tests=true ;;
      *) build_tests=false ;;
esac],[build_tests=false])
AM_CONDITIONAL(BUILD_TESTS, test x$build_tests = xtrue)

dnl ## is the check package installed?
if test x$build_tests = xtrue; then
   PKG_CHECK_MODULES(CHECK, check >= 0.9.4 glib-2.0)
fi



dnl ## do we want documentation (gtk-doc)
build_docs=false
AC_ARG_ENABLE(docs,
AC_HELP_STRING([--enable-docs],
	[Build documentation (yes, no)]),
[case "${enableval}" in
      yes)
	build_docs=true ;;
      *) build_docs=false ;;
esac],[build_docs=false])
AM_CONDITIONAL(BUILD_DOCS, test x$build_docs = xtrue)

dnl ## gtk-doc
GTK_DOC_CHECK([1.0])

dnl Intltool utility programs
#AC_PROG_INTLTOOL([0.33.0],[no-xml])


AC_OUTPUT([
po/Makefile.in
Makefile
man/Makefile
src/Makefile
src/gnome/Makefile
src/maemo/Makefile
src/widgets/Makefile
docs/Makefile
docs/reference/Makefile
tests/Makefile
src/maemo/modest.desktop
src/maemo/modest.service
])



echo
echo "modest configure results"
echo "-----------------------------------"
echo "Platform       : $with_platform"
echo "Build tests    : $build_tests"
echo "Build docs     : $build_docs"
echo "Debug version  : $with_debug"
echo ""
echo "NOTE: you should do a 'make clean' after changing architecture or platform (--with-platform=)"
echo ""
