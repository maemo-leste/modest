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

AC_INIT([modest],[1.0],[http://maemo.org])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_SRCDIR(src/modest-main.c)
AM_INIT_AUTOMAKE([dist-bzip2])


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
PKG_CHECK_MODULES(MODEST_MAEMO_LIBS,libosso,[with_platform=maemo],true)

AC_ARG_WITH(platform,
[--with-platform=gnome		platform to build for [gnome]],
[case "${with_platform}" in
       gnome)    MODEST_PLATFORM=gnome;;
       maemo) MODEST_PLATFORM=maemo;;
       *)      MODEST_PLATFORM=${with_platform}
esac], [MODEST_PLATFORM=gnome
               with_platform=gnome])

have_easysetup=false
if test "x$with_platform" == "xgnome"; then
      PKG_CHECK_MODULES(MODEST_PLATFORM,libtinymail-gnome-desktop-1.0 libnm_glib glib-2.0 >= 2.8 gtk+-2.0 >= 2.10 libnotify)
    AC_DEFINE_UNQUOTED(MODEST_PLATFORM_GNOME, 1, ["Whether modest is being built for the GNOME platform."])
    AC_MSG_ERROR([The GNOME Build is currently broken; please try the Maemo build instead])
else
    have_easysetup=true
    PKG_CHECK_MODULES(MODEST_PLATFORM,libosso conic libtinymail-maemo-1.0 osso-addressbook-1.0 libebook-1.2 libalarm)
    AC_DEFINE_UNQUOTED(MODEST_PLATFORM_MAEMO, 1, ["Whether modest is being built for the MAEMO platform."])
    AC_DEFINE_UNQUOTED(MODEST_HAVE_EASYSETUP, 1, ["Whether we have the easysetup feature"])
fi		    

AM_CONDITIONAL(MODEST_HAVE_EASYSETUP,test "x$have_easysetup" == "xtrue")
AC_SUBST(MODEST_PLATFORM)




#
# check for wpeditor or libwpeditor-plus (on maemo)
#
if test "x$with_platform" == "xmaemo"; then  
PKG_CHECK_MODULES(MODEST_WPEDITOR,wpeditor,have_wpeditor=true,have_wpeditor=false)
if test "x$have_wpeditor" == "xfalse"; then
   PKG_CHECK_MODULES(MODEST_WPEDITOR,libwpeditor-plus,have_libwpeditorplus=true,have_libwpeditorplus=false)	
   if test "x$have_libwpeditorplus" == "xtrue"; then
      	wpeditor="libwpeditor-plus"
	AC_DEFINE_UNQUOTED(MODEST_HAVE_LIBWPEDITOR_PLUS, 1, ["Whether libwpeditor-plus is used."])		
   else
	AC_MSG_ERROR([you need to have either wpeditor or libwpeditor-plus installed])
   fi
else
   wpeditor="wpeditor"	
   AC_DEFINE_UNQUOTED(MODEST_HAVE_WPEDITOR, 1, ["Whether wpeditor is used."])		
fi
fi


#
# check the providers-data (on maemo)
#
if test "x$with_platform" == "xmaemo"; then  
PKG_CHECK_MODULES(MODEST_PROVIDERS_DATA,modest-providers-data,have_modest_providers_data=true,have_modest_providers_data=false)
if test "x$have_modest_providers_data" == "xfalse"; then
   PKG_CHECK_MODULES(MODEST_PROVIDERS_DATA,maemo-providers-data,have_maemo_providers_data=true,have_maemo_providers_data=false)
   if test "x$have_maemo_providers_data" == "xfalse"; then
      	echo "x$have_maemo_providers_data"   
       	AC_MSG_ERROR([you need to have either modest-providers-data or maemo-providers-data installed])
   else
	AC_DEFINE_UNQUOTED(MODEST_HAVE_MAEMO_PROVIDERS_DATA, 1, ["Whether maemo-providers-data is used."])
	AC_DEFINE_UNQUOTED([MODEST_PROVIDERS_DATA_PATH],["`$PKG_CONFIG --variable=path maemo-providers-data`"],[maemo-providers-data file path])
	providers_data="maemo-providers-data"		
   fi
else
  AC_DEFINE_UNQUOTED(MODEST_HAVE_MODEST_PROVIDERS_DATA, 1, ["Whether modest-providers-data is used."])
  AC_DEFINE_UNQUOTED([MODEST_PROVIDERS_DATA_PATH],["`$PKG_CONFIG --variable=path modest-providers-data`"],[modest-providers-data file path])
  providers_data="modest-providers-data"								
fi
fi      
   


#
# check hildon-help (only for maemo)
#
if test "x$with_platform" == "xmaemo"; then  
   PKG_CHECK_MODULES(MODEST_HILDON_HELP,hildon-help,have_hildon_help=true,have_hildon_help=false)
   if test "x$have_hildon_help" == "xfalse"; then
      PKG_CHECK_MODULES(MODEST_HILDON_HELP,libossohelp,have_osso_help=true,have_osso_help=false)	
      if test "x$have_osso_help" == "xtrue"; then
      	helplib="osso-help"
	AC_DEFINE_UNQUOTED(MODEST_HAVE_OSSO_HELP, 1, ["Whether osso-help is used."])		
      else
	AC_MSG_ERROR([you need to have either osso-help or hildon-help installed])
      fi
   else
     helplib="hildon-help"	
     AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON_HELP, 1, ["Whether hildon-help is used."])		
   fi
fi



#
# check for the notification stuff (only maemo)
#
if test "x$with_platform" == "xmaemo"; then  
   PKG_CHECK_MODULES(MODEST_HILDON_NOTIFY,hildon-notify libnotify,have_hildon_notify=true,have_hildon_notify=false)
   if test "x$have_hildon_notify" == "xtrue"; then
      AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON_NOTIFY, 1, ["Whether hildon-notify is available"])      
   fi
fi


#
# check for the uri/mime stuff
#
if test "x$with_platform" == "xmaemo"; then  
   PKG_CHECK_MODULES(MODEST_HILDON_MIME,libhildonmime,have_hildonmime=true,have_hildonmime=false)
   if test "x$have_hildonmime" == "xfalse"; then
      PKG_CHECK_MODULES(MODEST_HILDON_MIME,libossomime,have_ossomime=true,have_ossomime=false)	
      if test "x$have_ossomime" == "xtrue"; then
      	mimelib="osso-mime"
	AC_DEFINE_UNQUOTED(MODEST_HAVE_OSSO_MIME, 1, ["Whether osso-mime is used."])		
      else
	AC_MSG_ERROR([you need to have either osso-mime or hildon-mime installed])
      fi
   else
     helplib="hildon-mime"	
     AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON_MIME, 1, ["Whether hildon-mime is used."])		
   fi
fi



#dnl wether we can and should build the 
#dnl search plugin (on maemo)
if test "x$with_platform" == "xmaemo"; then  
PKG_CHECK_MODULES(MODEST_OGS, libosso >= 0.8.5 libogs-1.2, have_ogs=true, have_ogs=false)
have_ogs_support=false
AC_ARG_ENABLE(ogs_support,
AC_HELP_STRING([--enable-ogs-support],
	[Build with osso global search support (yes, no)]),
[case "${enableval}" in
      yes)
	if test x$have_ogs = xfalse; then
	   AC_MSG_ERROR([building of ogs support requested and libogs-1.2 not found])
	fi
	;;
      *) 
	have_ogs_support=false
	;;
esac],[have_ogs_support=$have_ogs])

if test x$have_ogs_support = xtrue; then
	AC_DEFINE_UNQUOTED(MODEST_HAVE_OGS, 1, ["Whether ogs is available."])
fi
fi



#
# check the version of the widgets on maemo
#
if test "x$with_platform" == "xmaemo"; then  
PKG_CHECK_MODULES(MODEST_HILDON_WIDGETS, hildon-1 >= 0.9.9 hildon-fm-2, have_hildon1=true,have_hildon1=false)	
if test "x$have_hildon1" == "xfalse"; then 
   PKG_CHECK_MODULES(MODEST_HILDON_WIDGETS, hildon-libs >= 0.12.0 hildon-fm, have_hildon0=true,have_hildon0=false)	
   if test "x$have_hildon0" == "xtrue"; then
      AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON0_WIDGETS, 1, ["Whether hildon0 widgets are used."])		      
   else
      AC_MSG_ERROR([hildon widgets and/or hildon-fm not found])
   fi
else
      AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON1_WIDGETS, 1, ["Whether hildon1 widgets are used."])		    
fi
fi



#
# check the version of the addressbook on maemo
#
if test "x$with_platform" == "xmaemo"; then  
   PKG_CHECK_MODULES(MODEST_ABOOK, osso-addressbook-1.0 < 2.0, have_old_abook=true,have_old_abook=false)	
   if test "x$have_old_abook" == "xtrue"; then
      AC_DEFINE_UNQUOTED(MODEST_HAVE_OLD_ABOOK, 1, ["whether we have an old osso-addressbook"])
   fi
fi



#FIXME: hack to set the right gettext package..
if test "x$have_hildon1" == "xtrue"; then
     GETTEXT_PACKAGE=osso-email
else
     GETTEXT_PACKAGE=modest	
fi
# Check for gettext support
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", ["gettext package"])


#
# settings for maemo platform
#
if test "x$with_platform" == "xmaemo"; then  

  # warnings are errors, but don't check format errors, because
  # it breaks with logical ids
  CFLAGS="$CFLAGS -Wno-format-extra-args -Werror"

   desktopentrydir=`$PKG_CONFIG osso-af-settings --variable=desktopentrydir`
   serviceentrydir=`$PKG_CONFIG dbus-glib-1 --variable=prefix`/share/dbus-1/services
   localedir=`$PKG_CONFIG osso-af-settings --variable=localedir`
   backupsettingsdir=/etc/osso-backup/applications
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
   AC_SUBST(backupsettingsdir)
   AC_SUBST(localedir)
   AC_SUBST(pixmapdir)
   AC_SUBST(icon_26x26dir)
   AC_SUBST(icon_34x34dir)
   AC_SUBST(icon_40x40dir)
   AC_SUBST(icon_50x50dir)
   AC_SUBST(icon_scalabledir)
   AC_SUBST(pluginlibdir)
   AC_SUBST(plugindesktopentrydir)

else
   # gnome frontend 
   PKG_CHECK_MODULES(MODEST_LIBTINYMAIL_GNOME_DESKTOP,libtinymail-gnome-desktop-1.0 libnm_glib glib-2.0 >= 2.8 gtk+-2.0 >= 2.10 libnotify)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS)
   AC_SUBST(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)  
   AC_DEFINE_UNQUOTED(MODEST_PLATFORM_GNOME, 1, ["Whether modest is being built for the GNOME platform."])

  # no dbus api for gnome
  dbus_api=""
  AC_SUBST(dbus_api)

fi


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
src/maemo/com.nokia.modest.service
libmodest-dbus-client/Makefile
libmodest-dbus-client/libmodest-dbus-client-1.0.pc
])


echo
echo "modest configure results"
echo "-----------------------------------"
echo "Platform         : $with_platform"
echo "Prefix           : ${prefix}"
echo "Build tests      : $build_tests"
echo "Build docs       : $build_docs"
echo "Gettext Packages : $GETTEXT_PACKAGE"
echo "Easysetup data   : $providers_data"
echo "Editor lib       : $wpeditor"
echo "Help lib         : $helplib"
echo "Hildon-Notify    : $have_hildon_notify"    
echo "LibOGS support   : $have_ogs_support"
echo "CFLAGS           : $CFLAGS"
echo "Gettext Packages : $GETTEXT_PACKAGE"
echo "Debug version    : $with_debug"
echo ""
echo "NOTE: you should do a 'make clean' after changing architecture or platform (--with-platform=)"
echo ""
