#
# Makefile.am
# Time-stamp: <2007-01-17 16:00:38 (djcb)>
#
#
# use Automake 'trick' ==> convenience static libraries, which
# won't be installed, just for linking
#
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	-DMODEST_PLATFORM_ID=$(MODEST_PLATFORM_ID) \
	-I ${top_srcdir}/src/widgets  \
	-I ${top_srcdir}/src  \
	-DPREFIX=\"@prefix@\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

libmodest_ui_la_SOURCES=              \
	modest-account-view-window.c  \
	modest-account-view-window.h  \
	modest-edit-msg-window.c      \
	modest-icon-names.h           \
	modest-main-window.c          \
	modest-store-widget.c         \
	modest-store-widget.h         \
	modest-transport-widget.c     \
	modest-transport-widget.h     

#	modest-account-assistant.c    
#	modest-account-assistant.h    


LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

PIXMAP_FILES=\
	pixmaps/*.png 

pixmapdir = $(datadir)/pixmaps/modest
pixmap_DATA = $(PIXMAP_FILES)

UI_FILES=\
	ui/modest-ui.xml \
	ui/modest-edit-msg-window-ui.xml

uidir = $(datadir)/modest/ui
ui_DATA = $(UI_FILES)

EXTRA_DIST=\
	$(UI_FILES) \
	$(PIXMAP_FILES)
