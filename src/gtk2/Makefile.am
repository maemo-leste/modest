#
# Makefile.am
# Time-stamp: <2006-07-14 12:43:36 (djcb)>
#
#
# use Automake 'trick' ==> convenience static libraries, which
# won't be installed, just for linking
#
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	-DPREFIX=\"@prefix@\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

PIXMAP_FILES=\
	pixmaps/*.png

libmodest_ui_la_SOURCES=\
	modest-ui.c\
	modest-icon-names.h\
	modest-main-window.h\
	modest-main-window.c

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

EXTRA_DIST=pixmaps/*

pixmapdir = $(prefix)/share/modest/pixmaps
pixmap_DATA = $(PIXMAP_FILES)
