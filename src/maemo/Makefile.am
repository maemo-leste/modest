#
# Makefile.am
# Time-stamp: <2007-01-09 17:08:25 (djcb)>
#
#
# use Automake 'trick' ==> convenience static libraries, which
# won't be installed, just for linking
#
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	-DMODEST_PLATFORM_ID=$(MODEST_PLATFORM_ID) \
	-I ${top_srcdir}/src  \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DPREFIX=\"@prefix@\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

PIXMAP_FILES=\
	pixmaps/*.png 

libmodest_ui_la_SOURCES=              \
	modest-icon-names.h           \
	modest-main-window.h          \
	modest-main-window.c          \
	modest-account-view-window.c  \
	modest-account-view-window.h  \
	modest-edit-msg-window.c      \
	modest-edit-msg-window.h      \
	modest-transport-widget.c     \
	modest-transport-widget.h     \
	modest-store-widget.c         \
	modest-store-widget.h 

#	modest-account-assistant.c    
#	modest-account-assistant.h    

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

EXTRA_DIST=pixmaps/*

pixmapdir = $(prefix)/share/pixmaps/modest/
pixmap_DATA = $(PIXMAP_FILES)
