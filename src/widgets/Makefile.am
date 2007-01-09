#
# Makefile.am
# Time-stamp: <2007-01-09 17:54:07 (djcb)>

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I ${top_srcdir}/src/$(MODEST_PLATFORM) \
	-I ${top_srcdir}/src \
	-DPREFIX=\"@prefix@\" \
	-DMODEST_PLATFORM_ID=$(MODEST_PLATFORM_ID) \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-widgets.la

libmodest_widgets_la_SOURCES=   \
	modest-account-view.c   \
	modest-account-view.h   \
	modest-folder-view.c    \
	modest-folder-view.h    \
	modest-header-view.c    \
	modest-header-view.h    \
	modest-header-view-priv.h \
	modest-msg-view.c       \
	modest-msg-view.h	\
	modest-toolbar.c        \
	modest-toolbar.h        \
	modest-combo-box.c	\
	modest-combo-box.h      \
	modest-window.h         \
	modest-main-window.h	\
	modest-edit-msg-window.h
 
LDADD = \
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)        \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) 		
