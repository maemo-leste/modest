#
# Makefile.am
# Time-stamp: <2007-06-23 15:41:18 (djcb)>
#
#
# use Automake 'trick' ==> convenience static libraries, which
# won't be installed, just for linking
#
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_PLATFORM_CFLAGS) \
	$(MODEST_TOOLKIT_CFLAGS) \
	$(MODEST_PLUGIN_DEFINES) \
	-I ${top_srcdir}/src  \
	-DPREFIX=\"@prefix@\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

PIXMAP_FILES=\
	pixmaps/*.png

libmodest_ui_la_SOURCES=              	\
	modest-account-assistant.c    	\
	modest-account-assistant.h    	\
	modest-default-account-settings-dialog.c \
	modest-account-view-window.c  	\
	modest-msg-edit-window.c      	\
	modest-icon-names.h           	\
	modest-gnome-global-settings-dialog.c \
	modest-gnome-global-settings-dialog.h \
	modest-gnome-sort-dialog.c 	\
	modest-gnome-sort-dialog.h 	\
	modest-gnome-utils.h		\
	modest-gnome-utils.c		\
	modest-main-window.c          	\
	modest-main-window-ui.h	      	\
	modest-msg-view-window.c      	\
	modest-platform.c	      	\
	modest-platform.c	      	\
	modest-store-widget.c         	\
	modest-store-widget.h         	\
	modest-transport-widget.c     	\
	modest-transport-widget.h     

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_PLATFORM_LIBS) \
	$(MODEST_TOOKIT_LIBS)

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

