#
# Makefile.am
# Time-stamp: <2006-07-21 11:12:44 (djcb)>
#
#
# use Automake 'trick' ==> convenience static libraries, which
# won't be installed, just for linking
#
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	 -I ${top_srcdir}/src  \
	-DPREFIX=\"@prefix@\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

GLADE_FILES=\
	modest.glade\
	glade/*.png

PIXMAP_FILES=\
	pixmaps/*.png

libmodest_ui_la_SOURCES=\
	modest-ui.c\
	modest-ui-glade.h\
	modest-icon-names.h\
	modest-ui-main-view.c\
	modest-ui-main-view.h\
	modest-ui-message-editor.c\
	modest-ui-message-editor.h\
	modest-ui-message-viewer.c\
	modest-ui-message-viewer.h\
	modest-ui-wizard.h\
	modest-ui-wizard.c\
	modest-ui-account-setup.h\
	modest-ui-account-setup.c

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS)

modestnet_SCRIPTS = modest.sh
modestnetdir = $(prefix)/bin

gladedir = $(prefix)/share/modest/glade
glade_DATA = $(GLADE_FILES)

pixmapdir = $(prefix)/share/modest/pixmaps
pixmap_DATA = $(PIXMAP_FILES)


desktopdir = $(datadir)/applications/hildon
desktop_in_files = modest.desktop.in

servicedir = $(datadir)/dbus-1/services
service_in_files = com.nokia.modest.service.in

pixmaps_extradir = $(datadir)/pixmaps
pixmaps_extra_DATA = modest.png

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
service_DATA = $(service_in_files:.service.in=.service)

# will be replaced by:
# @INTLTOOL_DESKTOP_RULE@
modest.desktop:
	cp modest.desktop.in modest.desktop

clean-local:
	rm -f modest.desktop 

EXTRA_DIST=pixmaps/* glade/* modest.glade modest.png com.nokia.modest.service.in modest.desktop.in
