#
# Makefile.am
# Time-stamp: <2007-02-10 14:48:38 (djcb)>

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

libmodest_widgets_la_SOURCES=          \
	modest-account-view-window.h   \
	modest-account-view.c          \
	modest-account-view.h          \
	modest-attachments-view.c      \
	modest-attachments-view.h      \
	modest-combo-box.c	       \
	modest-combo-box.h             \
	modest-folder-view.c           \
	modest-folder-view.h           \
	modest-header-view-priv.h      \
	modest-header-view-render.c    \
	modest-header-view.c           \
	modest-header-view.h           \
	modest-mail-header-view.c      \
	modest-mail-header-view.h      \
	modest-main-window.h	       \
	modest-msg-edit-window-ui.h    \
	modest-msg-edit-window.h       \
	modest-msg-view-window.h       \
	modest-msg-view.c              \
	modest-msg-view.h	       \
	modest-recpt-view.c            \
	modest-recpt-view.h            \
	modest-tny-stream-gtkhtml.c    \
	modest-tny-stream-gtkhtml.h    \
	modest-window.c                \
	modest-window.h             

LDADD = \
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)        \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) 		
