#
# Makefile.am
# Time-stamp: <2007-06-23 15:40:04 (djcb)>

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_PLATFORM_CFLAGS) \
	-I ${top_srcdir}/src/$(MODEST_PLATFORM) \
	-I ${top_srcdir}/src \
	-DPREFIX=\"@prefix@\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-widgets.la

libmodest_widgets_la_SOURCES=          \
	modest-account-view-window.h   \
	modest-account-view.c          \
	modest-account-view.h          \
	modest-attachment-view.c       \
	modest-attachment-view.h       \
	modest-attachments-view.c      \
	modest-attachments-view.h      \
	modest-combo-box.c	       \
	modest-combo-box.h             \
	modest-folder-view.c           \
	modest-folder-view.h           \
	modest-global-settings-dialog.c \
	modest-global-settings-dialog.h \
	modest-hbox-cell-renderer.c    \
	modest-hbox-cell-renderer.h    \
	modest-vbox-cell-renderer.c    \
	modest-vbox-cell-renderer.h    \
	modest-header-view-render.c    \
	modest-header-view.c           \
	modest-header-view.h           \
	modest-limit-retrieve-combo-box.h \
	modest-limit-retrieve-combo-box.c \
	modest-mail-header-view.c      \
	modest-mail-header-view.h      \
	modest-main-window.h	       \
	modest-msg-edit-window-ui.h    \
	modest-msg-edit-window.h       \
	modest-msg-view-window.h       \
	modest-msg-view.c              \
	modest-msg-view.h	       \
	modest-details-dialog.c        \
	modest-details-dialog.h        \
	modest-recpt-editor.c          \
	modest-recpt-editor.h          \
	modest-recpt-view.c            \
	modest-recpt-view.h            \
	modest-retrieve-combo-box.h    \
	modest-retrieve-combo-box.c    \
	modest-scroll-text.c           \
	modest-scroll-text.h           \
	modest-serversecurity-combo-box.h modest-serversecurity-combo-box.c \
	modest-secureauth-combo-box.h modest-secureauth-combo-box.c \
	modest-tny-stream-gtkhtml.c    \
	modest-tny-stream-gtkhtml.h    \
	modest-ui-constants.h \
	modest-validating-entry.h modest-validating-entry.c \
	modest-window.c                \
	modest-window.h	               \
	modest-window-mgr.h            \
	modest-window-mgr.c

LDADD = \
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_PLATFORM_LIBS)
