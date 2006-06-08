#
# Makefile.am
# Time-stamp: <2006-06-06 14:27:48 (djcb)>
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

GLADE_FILES=\
	modest.glade\
	glade/*.png 

PIXMAP_FILES=\
	pixmaps/*.png

libmodest_ui_la_SOURCES=\
	modest-ui.c\
	modest-icon-names.h\
	modest-ui-wizard.c

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

EXTRA_DIST=pixmaps/* glade/* modest.glade modest.gladep

gladedir = $(prefix)/share/modest/glade
glade_DATA = $(GLADE_FILES)

pixmapdir = $(prefix)/share/modest/pixmaps
pixmap_DATA = $(PIXMAP_FILES)
