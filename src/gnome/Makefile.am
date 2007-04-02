#
# Makefile.am
# Time-stamp: <2007-02-10 14:47:15 (djcb)>
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
	-DPREFIX=\"@prefix@\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

PIXMAP_FILES=\
	pixmaps/*.png \
	pixmaps/*.svg

libmodest_ui_la_SOURCES=              \
	modest-account-assistant.c    \
	modest-account-assistant.h    \
	modest-account-view-window.c  \
	modest-address-book.c         \
	modest-msg-edit-window.c      \
	modest-icon-names.h           \
	modest-gnome-info-bar.c       \
	modest-gnome-info-bar.h       \
	modest-main-window.c          \
	modest-main-window-ui.h	      \
	modest-msg-view-window.c      \
	modest-platform.c	      \
	modest-platform.c	      \
	modest-store-widget.c         \
	modest-store-widget.h         \
	modest-transport-widget.c     \
	modest-transport-widget.h     

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

pixmapdir = $(datadir)/pixmaps/modest
pixmap_DATA = $(PIXMAP_FILES)

UI_FILES=\
	ui/modest-main-window-ui.xml \
	ui/modest-msg-view-window-ui.xml \
	ui/modest-msg-edit-window-ui.xml 

uidir = $(datadir)/modest/ui
ui_DATA = $(UI_FILES)

EXTRA_DIST=\
	$(UI_FILES) \
	$(PIXMAP_FILES)

