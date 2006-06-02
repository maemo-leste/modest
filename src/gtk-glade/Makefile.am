#
# Makefile.am
# Time-stamp: <2006-06-02 14:23:35 (djcb)>
#
#
# use Automake 'trick' ==> convenience static libraries, which
# won't be installed, just for linking
#
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	-DPREFIX=\"@prefix@\"

noinst_LTLIBRARIES=\
	libmodest-ui.la

GLADE_FILES=\
	modest.glade\
	pixmaps/*.png 

PIXMAP_FILES=\
	pixmaps/*.xpm

libmodest_ui_la_SOURCES=\
	modest-ui.c

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

EXTRA_DIST=pixmaps/*.png pixmaps/*.xpm modest.glade modest.gladep

gladedir = $(prefix)/share/modest/glade
glade_DATA = $(GLADE_FILES)

pixmapdir = $(prefix)/share/modest/pixmaps
pixmap_DATA = $(PIXMAP_FILES)
