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
GETTEXT_PACKAGE=modest
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", ["Description"])

ALL_LINGUAS="en_GB"

MODEST_LOCALE_DIR='$prefix/${DATADIRNAME}/locale'
AM_GLIB_DEFINE_LOCALEDIR([MODEST_LOCALE_DIR])
AM_GLIB_GNU_GETTEXT



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
PKG_CHECK_MODULES(MODEST_GSTUFF,glib-2.0 >= 2.6 gobject-2.0 gtk+-2.0 >= 2.6 gconf-2.0 gnome-vfs-2.0 libgtkhtml-3.8 libtinymail-1.0 libtinymail-camel-1.0 libtinymail-gnomevfs-1.0 libtinymailui-1.0 libtinymailui-gtk-1.0) 
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
   
   PKG_CHECK_MODULES(HILDON, hildon-1 >= 0.9.9, hildon1=yes, hildon1=no)	
   if test "$hildon1" == "yes"; then   # the 'new' hildon      			
      PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_MAEMO, conic libtinymail-maemo-1.0 hildon-1 libosso libhildonmime osso-addressbook-1.0 wpeditor libossohelp libebook-1.2 libalarm)     	
   
    else     # the old hildon
     PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_MAEMO, conic libtinymail-maemo-1.0 hildon-libs >= 0.12.0 libosso libossomime libossohelp osso-addressbook-1.0 wpeditor libebook-1.2 libalarm gnome-vfs-module-2.0)     	 
        AC_DEFINE_UNQUOTED(MODEST_HILDON_VERSION_0, 1, ["The Hildon version we support."])   
    fi 	
   
   AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_CFLAGS)
   AC_SUBST(MODEST_LIBTINYMAIL_MAEMO_LIBS) 
   
   AC_DEFINE_UNQUOTED(MODEST_PLATFORM_MAEMO, 1, ["Whether modest is being built for the Maemo platform."])
 
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

  dbus_api="dbus_api"
  AC_SUBST(dbus_api)

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

  # warnings are errors, but don't check format errors, because
  # it breaks with logical id's
  CFLAGS="$CFLAGS -Wno-format-extra-args -Werror"
else
   # gnome frontend 
   PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_GNOME_DESKTOP,libtinymail-gnome-desktop-1.0 libnm_glib glib-2.0 >= 2.8 gtk+-2.0 >= 2.10)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)  
   AC_DEFINE_UNQUOTED(MODEST_PLATFORM_GNOME, 1, ["Whether modest is being built for the GNOME platform."])

  # no dbus api for gnome
  dbus_api=""
  AC_SUBST(dbus_api)

fi

dnl do we have the easy setup stuff?
have_easysetup=false
if test -d src/${with_platform}/easysetup; then	
      	have_easysetup=true
	AC_DEFINE_UNQUOTED(MODEST_HAVE_EASYSETUP, 1, ["Whether the easysetup feature is available."])

	# Just check that the dependency exists:
    PKG_CHECK_MODULES(MODEST_PROVIDERS_DATA, modest-providers-data)	
    
	# Get the location of the ISO-Codes (currencies, languages) files:
	AC_DEFINE_UNQUOTED([MODEST_PROVIDERS_DATA_PATH],["`$PKG_CONFIG --variable=path modest-providers-data`"],[modest-providers-data file path])
fi
AM_CONDITIONAL(MODEST_HAVE_EASYSETUP,test x$have_easysetup = xtrue) 


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
src/dbus_api/Makefile
src/gnome/Makefile
src/maemo/Makefile
src/maemo/easysetup/Makefile
src/widgets/Makefile
docs/Makefile
docs/reference/Makefile
tests/Makefile
tests/dbus_api/Makefile
src/maemo/modest.desktop
src/maemo/modest.service
libmodest-dbus-client/Makefile
libmodest-dbus-client/libmodest-dbus-client-1.0.pc
])


echo
echo "modest configure results"
echo "-----------------------------------"
echo "Platform         : $with_platform"
echo "Build easy setup : $have_easysetup"
echo "Build tests      : $build_tests"
echo "Build docs       : $build_docs"
echo "Debug version    : $with_debug"
echo ""
echo "NOTE: you should do a 'make clean' after changing architecture or platform (--with-platform=)"
echo ""
