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

AC_INIT([modest],[3.0],[http://maemo.org])
AC_CONFIG_HEADERS([config.h])

m4_define([modest_api_major_version], [1])
m4_define([modest_api_minor_version], [99])
m4_define([modest_api_micro_version], [0])
m4_define([modest_api_version], [modest_api_major_version.modest_api_minor_version.modest_api_micro_version])


AC_CONFIG_SRCDIR(src/modest-main.c)
AM_INIT_AUTOMAKE([dist-bzip2])

MODEST_API_MAJOR_VERSION=modest_api_major_version
MODEST_API_MINOR_VERSION=modest_api_minor_version
MODEST_API_MICRO_VERSION=modest_api_micro_version
MODEST_API_VERSION=modest_api_version
AC_SUBST(MODEST_API_MAJOR_VERSION)
AC_SUBST(MODEST_API_MINOR_VERSION)
AC_SUBST(MODEST_API_MICRO_VERSION)
AC_SUBST(MODEST_API_VERSION)

ALL_LINGUAS="en_GB"

MODEST_LOCALE_DIR='$prefix/${DATADIRNAME}/locale'
AM_GLIB_DEFINE_LOCALEDIR([MODEST_LOCALE_DIR])
AM_GLIB_GNU_GETTEXT

if test x$prefix = xNONE; then
   prefix=/usr/local
fi
AC_SUBST(prefix)

AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL


# Option to enable debugging
AC_ARG_ENABLE(debug,
              [AC_HELP_STRING([--enable-debug],[Debugging (default=no)])],
              [with_debug=yes], [with_debug=no])

if test "x$with_debug" == "xyes" ; then
        CFLAGS="$CFLAGS -ggdb -O0 -DDEBUG -Wall"
else
        CFLAGS="$CFLAGS -O2 -Wall"
fi

# Option to enable debugging
AC_ARG_ENABLE(gcov,
              [AC_HELP_STRING([--enable-gcov],[Coverage reporting (default=no)])],
              [with_gcov=yes], [with_gcov=no])

if test "x$with_gcov" == "xyes" ; then
        CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	LDFLAGS="$LDFLAGS -lgcov"
fi



dnl -------
dnl GtkHTML
dnl -------

AC_MSG_CHECKING(for installed version of libgtkhtml3)
pkg-config --exists libgtkhtml-3.14
retval=$?
gtk_html_found=false
if test $retval -eq 0 ; then
    gtk_html_found=true
    gtk_html_version=3.14
else
    pkg-config --exists libgtkhtml-3.8
    retval=$?
    if test $retval -eq 0 ; then
      gtk_html_found=true
      gtk_html_version=3.8
    else
          if [ $gui = "true" ]; then
            AC_MSG_RESULT(libgtkhtml 3.4 or higher is not found!)
          fi
    fi
fi

if [ $gtk_html_found = "true" ]; then
   version=`pkg-config --modversion libgtkhtml-${gtk_html_version}`
   AC_MSG_RESULT(found version $version)
else
   AC_MSG_ERROR([install gtkhtml 3.8 or newer])
fi


dnl # GLib/Gobject/Gtk/Gconf => mandatory

PKG_CHECK_MODULES(MODEST_GSTUFF,glib-2.0 >= 2.6 gobject-2.0 gtk+-2.0 >= 2.6 gconf-2.0 gnome-vfs-2.0 libgtkhtml-${gtk_html_version} libtinymail-1.0 libtinymail-camel-1.0 libtinymail-gnomevfs-1.0 libtinymailui-1.0 libtinymailui-gtk-1.0) 
AC_SUBST(MODEST_GSTUFF_CFLAGS)
AC_SUBST(MODEST_GSTUFF_LIBS)

modest_save_cflags="$CFLAGS"
modest_save_libs="$LIBS"
PKG_CHECK_MODULES(MODEST_GTKHTML,libgtkhtml-${gtk_html_version})
AC_SUBST(MODEST_GTKHTML_CFLAGS)
AC_SUBST(MODEST_GTKHTML_LIBS)
CFLAGS="$MODEST_GTKHTML_CFLAGS"
LIBS="$MODEST_GTKHTML_LIBS"
AC_CHECK_DECLS([gtk_widget_tap_and_hold_setup],[],[], [#include <gtk/gtk.h>])
AC_CHECK_FUNCS(gtk_html_set_max_image_size)
AC_SUBST(HAVE_GTK_HTML_SET_MAX_IMAGE_SIZE)
AC_CHECK_FUNCS(gtk_html_get_selection_area)
AC_SUBST(HAVE_GTK_HTML_GET_SELECTION_AREA)
AC_CHECK_FUNCS(gtk_html_set_allow_dnd)
AC_SUBST(HAVE_GTK_HTML_SET_ALLOW_DND)
AC_CHECK_FUNCS(gtk_html_set_default_engine)
AC_SUBST(HAVE_GTK_HTML_SET_DEFAULT_ENGINE)
AC_CHECK_FUNCS(gtk_html_set_word_char_wrapping)
AC_SUBST(HAVE_GTK_HTML_SET_WORD_CHAR_WRAPPING)
AC_CHECK_FUNCS(gtk_tree_view_column_get_cell_data_hint)
AC_SUBST(HAVE_GTK_TREE_VIEW_COLUMN_GET_CELL_DATA_HINT)
CFLAGS="$modest_save_cflags"
LIBS="$modest_save_libs"


dnl dnl now, determine what to build
dnl AC_MSG_NOTICE([Determining what platform to build])
PKG_CHECK_MODULES(MODEST_MAEMO_LIBS,conic,[detected_platform=maemo],[detected_platform=gnome])

AC_ARG_WITH(platform, [
Target platform:
  --with-platform=detect  detect target platform (default)
  --with-platform=gnome   build for Gnome
  --with-platform=maemo   build for Maemo
],
[case "${with_platform}" in
       gnome|maemo|detect)    MODEST_PLATFORM=${with_platform};;
       *)        AC_MSG_ERROR([Unknown platform selected]);;
esac], [MODEST_PLATFORM=detect])

if test "$MODEST_PLATFORM" = "detect"; then
	MODEST_PLATFORM="$detected_platform"
	with_platform=$detected_platform
fi

if test "$with_platform" = "maemo"; then
   	MODEST_PLUGIN_DEFINES=" -DMODEST_PLATFORM_MAEMO -DMODEST_API_VERSION=${MODEST_API_VERSION}"
	PKG_CHECK_MODULES(LIBMODEST_DBUS_CLIENT,libmodest-dbus-client-1.0)
elif test "$with_platform" = "gnome"; then
	MODEST_PLUGIN_DEFINES=" -DMODEST_PLATFORM_GNOME -DMODEST_API_VERSION=${MODEST_API_VERSION}"
fi
AC_SUBST(MODEST_PLUGIN_DEFINES)

#
# check the widgets library
#
have_hildon2=false
have_hildon1=false
have_hildon0=false
have_gtk=false
PKG_CHECK_EXISTS(hildon-1, have_hildon1=true,have_hildon1=false)
if test "$have_hildon1" = "true"; then
   PKG_CHECK_EXISTS(hildon-1 >= 2.1.0 hildon-fm-2, have_hildon2=true, have_hildon2=false)
   PKG_CHECK_MODULES(MODEST_HILDON_WIDGETS,hildon-1 hildon-fm-2)
   hildonpkgs="hildon-1 hildon-fm-2"
   detected_toolkit=hildon1
   echo "DETECTED HILDON 1 $have_hildon2."
else
   hildonpkgs="hildon-libs hildon-fm"
   detected_toolkit=hildon1
   PKG_CHECK_EXISTS(hildon-libs >= 0.12.0, have_hildon1=true, have_hildon1=false)
   if test "$have_hildon1" = "false"; then
      PKG_CHECK_EXISTS(hildon-libs, have_hildon0=true, have_hildon0=false)
      if test "$have_hildon0" = "false"; then
      	 have_gtk=true
	 detected_toolkit=gtk
	 hildonpkgs=
      fi
   fi
   if test "$have_gtk" = "false"; then
      PKG_CHECK_MODULES(MODEST_HILDON_WIDGETS,hildon-libs hildon-fm)
   fi
fi

dnl --------------- HILDON & PLUGIN PKGS -----------
pluginpkgs=$hildonpkgs
echo "Plugin Packages $pluginpkgs"
AC_SUBST(pluginpkgs)

use_profile_data=false
# Plugin packages must not include libcanberra and libprofile
if test "$have_hildon2" = "true"; then
   echo "DETECTED HILDON 2 $have_hildon2."
   detected_toolkit=hildon2
   hildonpkgs="$hildonpkgs libcanberra"
   AC_CHECK_HEADER([profiled/keys_nokia.h], use_profile_data=true, use_profile_data=false)
   if test "$use_profile_data" = "true"; then
      hildonpkgs="$hildonpkgs profile"
      AC_DEFINE_UNQUOTED(MODEST_USE_PROFILE, 1, ["Use the Profile data include"])
   fi
   echo "HILDON 2 PACKAGES $hildonpkgs"
fi
AC_SUBST(hildonpkgs)

dnl --------------- HILDON API ------------
hildon_api=
if test "$have_hildon0" = "true"; then
   hildon_api=0
elif test "$have_hildon1" = "true"; then
   hildon_api=1
elif test "$have_gtk" = "false"; then
   hildon_api=`$PKG_CONFIG --modversion hildon-1 | cut -d. -f1`
fi

AC_DEFINE_UNQUOTED(MODEST_HILDON_API, $hildon_api, ["the API version of hildon"])
MODEST_PLUGIN_DEFINES="$MODEST_PLUGIN_DEFINES -DMODEST_HILDON_API=$hildon_api"

AC_ARG_WITH(toolkit, [
Target toolkit:
  --with-toolkit=detect  detect target toolkit (default)
  --with-toolkit=gtk   build for Gnome
  --with-toolkit=hildon1   build for Hildon 1.x
  --with-toolkit=hildon2   build for Hildon 2.x
],
[case "${with_toolkit}" in
       gtk|hildon1|hildon2|detect)    MODEST_TOOLKIT=${with_toolkit};;
       *)        AC_MSG_ERROR([Unknown platform selected]);;
esac], [MODEST_TOOLKIT=detect])

if test "$MODEST_TOOLKIT" = "detect"; then
	MODEST_TOOLKIT="$detected_toolkit"
	with_toolkit=$detected_toolkit
fi

if test "$with_toolkit" = "hildon1"; then
   	MODEST_PLUGIN_DEFINES="$MODEST_PLUGIN_DEFINES -DMODEST_TOOLKIT_HILDON1"
elif test "$with_toolkit" = "hildon2"; then
	MODEST_PLUGIN_DEFINES="$MODEST_PLUGIN_DEFINES -DMODEST_TOOLKIT_HILDON2"
elif test "$with_toolkit" = "gtk"; then
	MODEST_PLUGIN_DEFINES="$MODEST_PLUGIN_DEFINES -DMODEST_TOOLKIT_GNOME"
fi

dnl Whether to use maemo-launcher or not
AC_ARG_ENABLE([maemo-launcher],
	      [AS_HELP_STRING([--enable-maemo-launcher],
			      [build with maemo-launcher support])],
			      [case "${enableval}" in
			       yes) maemo_launcher=true ;;
			       no)  maemo_launcher=false ;;
			       *) AC_MSG_ERROR([bad value ${enableval} for --enable-maemo-launcher]) ;;
			       esac], [maemo_launcher=false])
if test x$maemo_launcher = xtrue
then
	PKG_CHECK_MODULES(MAEMO_LAUNCHER, [maemo-launcher-app])
	AC_SUBST(MAEMO_LAUNCHER_CFLAGS)
	AC_SUBST(MAEMO_LAUNCHER_LIBS)
	MAEMO_LAUNCHER_MODEST_LDFLAGS="-shared "
else
	MAEMO_LAUNCHER_MODEST_LDFLAGS="-export-dynamic "
fi
AC_SUBST(MAEMO_LAUNCHER_MODEST_LDFLAGS)

have_easysetup=false
if test "x$with_toolkit" == "xgtk"; then
   PKG_CHECK_MODULES(MODEST_TOOLKIT,glib-2.0 >= 2.8 gtk+-2.0 >= 2.10 libnotify gtksourceview-2.0)
   AC_DEFINE_UNQUOTED(MODEST_TOOLKIT_GTK, 1, ["Whether modest is being build for the gtk toolkit."])
   MODEST_TOOLKIT_DIR=gnome
else
   have_easysetup=true
   PKG_CHECK_MODULES(MODEST_TOOLKIT,$hildonpkgs)
   if test "$with_toolkit" = "hildon2"; then
      AC_DEFINE_UNQUOTED(MODEST_TOOLKIT_HILDON2, 1, ["Whether modest is being build for the hildon2 toolkit."])
      MODEST_TOOLKIT_DIR=hildon2
   else
      AC_DEFINE_UNQUOTED(MODEST_TOOLKIT_HILDON1, 1, ["Whether modest is being build for the hildon2 toolkit."])
      MODEST_TOOLKIT_DIR=maemo
   fi
    AC_DEFINE_UNQUOTED(MODEST_HAVE_EASYSETUP, 1, ["Whether we have the easysetup feature"])
fi
AM_CONDITIONAL(MODEST_HAVE_EASYSETUP,test "x$have_easysetup" == "xtrue")
AC_SUBST(MODEST_TOOLKIT)
AC_SUBST(MODEST_TOOLKIT_DIR)

if test "x$with_platform" == "xgnome"; then
      PKG_CHECK_MODULES(MODEST_PLATFORM,libtinymail-gnome-desktop-1.0 glib-2.0 >= 2.8 libnotify)
    AC_DEFINE_UNQUOTED(MODEST_PLATFORM_GNOME, 1, ["Whether modest is being built for the GNOME platform."])
dnl     AC_MSG_ERROR([The GNOME Build is currently broken; please try the Maemo build instead])
else
    have_dbusapi=true
    PKG_CHECK_MODULES(MODEST_PLATFORM,libosso dbus-1 libtinymail-maemo-1.0 libebook-1.2 )
    AC_DEFINE_UNQUOTED(MODEST_PLATFORM_MAEMO, 1, ["Whether modest is being built for the MAEMO platform."])
fi
AM_CONDITIONAL(MODEST_HAVE_DBUS_API,test "x$have_dbusapi" == "xtrue")
AC_SUBST(MODEST_PLATFORM)



#
# check for wpeditor or libwpeditor-plus (on maemo)
#
if test "x$with_toolkit" != "xgtk"; then  
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
# check libalarm
#
if test "x$with_platform" == "xmaemo"; then  
   PKG_CHECK_MODULES(MODEST_LIBALARM,libalarm,have_libalarm=true,have_libalarm=false)
   if test "x$have_libalarm" == "xfalse"; then
      PKG_CHECK_MODULES(MODEST_LIBALARM, alarm, have_libalarm=true,have_libalarm=false)
   fi
   if test "x$have_libalarm" == "xtrue"; then
      AC_DEFINE_UNQUOTED(MODEST_HAVE_LIBALARM, 1, ["Whether libalarm is used."])		
      AC_SUBST(MODEST_LIBALARM_CFLAGS)
      AC_SUBST(MODEST_LIBALARM_LIBS)
   fi
fi

#
# check for MCE
#
if test "x$with_platform" == "xmaemo"; then  
PKG_CHECK_MODULES(MODEST_MCE,mce,have_mce=true,have_mce=false)
if test "x$have_mce" == "xtrue"; then
   mce="mce"	
   AC_DEFINE_UNQUOTED(MODEST_HAVE_MCE, 1, ["Whether mce is used."])
   AC_SUBST(MODEST_MCE_CFLAGS)
   AC_SUBST(MODEST_MCE_LIBS)
else
   mce=""
fi
fi

#
# check for libconic; this is a trick to check whether we run
# on a 'real' maemo platform, or on, say, ubuntu embedded
# this determines whether we build with the non-hildon GTK+
#
if test "x$with_platform" == "xmaemo"; then  
PKG_CHECK_MODULES(MODEST_LIBCONIC,conic,have_libconic=true,have_libconic=false)
if test "x$have_libconic" == "xtrue"; then
   AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON_GTK,1,["Whether modest is being built with Hildon/Gtk"])
   AC_DEFINE_UNQUOTED(MODEST_HAVE_CONIC,1,["Whether modest has conic available"])
   AC_SUBST(MODEST_LIBCONIC_CLAGS)
   AC_SUBST(MODEST_LIBCONIC_LIBS)
else
# if we define this, tinymail will not try to #include conic headers
   CFLAGS="$CFLAGS -DLIBTINYMAIL_MAEMO_WITHOUT_CONIC"
fi
fi

#
# check hildon-help (only for maemo AND NOT fremantle)
#
if test "x$with_toolkit" == "xhildon1"; then
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
if test "x$with_toolkit" != "xgtk"; then  
   PKG_CHECK_MODULES(MODEST_HILDON_NOTIFY,hildon-notify libnotify,have_hildon_notify=true,have_hildon_notify=false)
   if test "x$have_hildon_notify" == "xtrue"; then
      AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON_NOTIFY, 1, ["Whether hildon-notify is available"])      
   fi
fi


#
# check for the uri/mime stuff
#
if test "x$with_toolkit" != "xgtk"; then  
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
     mimelib="hildon-mime"	
     AC_DEFINE_UNQUOTED(MODEST_HAVE_HILDON_MIME, 1, ["Whether hildon-mime is used."])		
   fi
fi

PKG_CHECK_MODULES(MODEST_MOZEMBED, libtinymailui-mozembed-1.0, have_mozembed=true, have_mozembed=false)
AC_ARG_ENABLE(mozembed,
AC_HELP_STRING([--enable-mozembed],
	[Build with GtkMozEmbed (yes, no)]),
[case "${enableval}" in
      yes)
	if test x$have_mozembed = xfalse; then
	   AC_MSG_ERROR([building of GtkMozEmbed support requested and gtkmozembed not found])
	fi
	use_mozembed=true
	;;
      *)
        use_mozembed=false
	;;
esac],[use_mozembed=false])

AM_CONDITIONAL(BUILD_MOZEMBED, test x$use_mozembed = xtrue)

if test x$use_mozembed = xtrue; then
   	AC_DEFINE_UNQUOTED(MODEST_USE_MOZEMBED, 1, ["Whether we use GtkMozEmbed."])
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
# check the version of the addressbook on maemo
#
if test "x$with_platform" == "xmaemo"; then  
   PKG_CHECK_MODULES(MODEST_ABOOK,osso-addressbook-1.0,have_abook=true,have_abook=false)
   abook_api=`$PKG_CONFIG --modversion osso-addressbook-1.0 | cut -d. -f1`
   AC_SUBST(MODEST_ABOOK_CFLAGS)
   AC_SUBST(MODEST_ABOOK_LIBS)

   if test "x$have_abook" == "xfalse"; then
     PKG_CHECK_MODULES(MODEST_ABOOK, libosso-abook-1.0,have_abook=true,have_abook=false)
     abook_api=`$PKG_CONFIG --modversion libosso-abook-1.0 | cut -d. -f1`
     AC_SUBST(MODEST_ABOOK_CFLAGS)
     AC_SUBST(MODEST_ABOOK_LIBE)

     modest_save_cflags="$CFLAGS"
     modest_save_libs="$LIBS"
     CFLAGS="$MODEST_ABOOK_CFLAGS"
     LIBS="$MODEST_ABOOK_LIBS"
     AC_CHECK_FUNCS(osso_abook_contact_chooser_set_visible_func)
     AC_SUBST(HAVE_OSSO_ABOOK_CONTACT_CHOOSER_SET_VISIBLE_FUNC)
     CFLAGS="$modest_save_cflags"
     LIBS="$modest_save_libs"
   fi

   if test "x$have_abook" == "xtrue"; then
      AC_DEFINE_UNQUOTED(OSSO_ABOOK_ENABLE_DEPRECATED, 1, ["use deprecated addressbook API"])
      AC_DEFINE_UNQUOTED(MODEST_ABOOK_API, $abook_api, ["the API version of osso-addressbook"])
      AC_DEFINE_UNQUOTED(MODEST_HAVE_ABOOK, 1, ["whether we have osso-addressbook"])
      addressbook="osso-addressbook"
   fi
fi

AC_CHECK_HEADERS([clockd/libtime.h], have_libtime=true, have_libtime=false)

if test "x$have_libtime" == "xtrue"; then
   AC_DEFINE_UNQUOTED(MODEST_USE_LIBTIME, 1, ["use libtime API"])
   MODEST_LIBTIME_LIBS=-ltime
else
   MODEST_LIBTIME_LIBS=
fi
AC_SUBST(MODEST_LIBTIME_LIBS)

#
# if we don't have an addressbook, use the dummy one
#
AM_CONDITIONAL(MODEST_USE_DUMMY_ADDRESS_BOOK,test "x$have_abook" != "xtrue")


AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

# gettext support
GETTEXT_PACKAGE=modest	
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
   prebackupdir=/etc/osso-backup/pre-backup.d/
   restoredir=/etc/osso-backup/restore.d/always/
   ossocudscriptsdir=/etc/osso-cud-scripts
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
   AC_SUBST(prebackupdir)
   AC_SUBST(restoredir)
   AC_SUBST(ossocudscriptsdir)
   AC_SUBST(localedir)
   AC_SUBST(pixmapdir)
   AC_SUBST(icon_26x26dir)
   AC_SUBST(icon_34x34dir)
   AC_SUBST(icon_40x40dir)
   AC_SUBST(icon_50x50dir)
   AC_SUBST(icon_scalabledir)
   AC_SUBST(pluginlibdir)
   AC_SUBST(plugindesktopentrydir)

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

AC_OUTPUT([
po/Makefile.in
Makefile
man/Makefile
src/Makefile
src/modest-plugin-1.0.pc
src/dbus_api/Makefile
src/gnome/Makefile
src/maemo/Makefile
src/maemo/easysetup/Makefile
src/hildon2/Makefile
src/widgets/Makefile
docs/Makefile
docs/reference/Makefile
tests/Makefile
tests/dbus_api/Makefile
src/maemo/modest.desktop
src/maemo/com.nokia.modest.service
src/hildon2/modest.desktop
src/hildon2/com.nokia.modest.service
])


echo
echo "modest configure results"
echo "-----------------------------------"
echo "Platform         : $with_platform"
echo "Toolkit          : $with_toolkit"
echo "Prefix           : ${prefix}"
echo "Build tests      : $build_tests"
echo "Build docs       : $build_docs"
echo "Gettext Package  : $GETTEXT_PACKAGE"

if test "x$with_platform" == "xmaemo"; then
   echo "Maemo Launcher   : $maemo_launcher"
   echo "Editor lib       : $wpeditor"
   echo "Addressbook      : $have_abook"
   if test "x$with_toolkit" == "xhildon1"; then
      echo "Help lib         : $helplib"
   fi
   echo "libconic support : $have_libconic"
   echo "Alarm            : $have_libalarm"
   echo "MCE              : $have_mce"
   echo "Hildon-Notify    : $have_hildon_notify"
   echo "LibOGS support   : $have_ogs_support"
fi

echo "MozEmbed support : $use_mozembed"
if [ $gtk_html_found = "true" ]; then
   echo "GtkHTML version  : $gtk_html_version"
fi
echo "Libtime          : $have_libtime"

echo "CFLAGS           : $CFLAGS"
echo "Debug version    : $with_debug"
echo ""
echo "NOTE: you should do a 'make clean' after changing architecture or platform (--with-platform=)"
echo ""
