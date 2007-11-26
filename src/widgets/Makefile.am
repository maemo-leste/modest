#
# Makefile.am
# Time-stamp: <2007-11-24 13:34:06 (djcb)>

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_PLATFORM_CFLAGS) \
	$(MODEST_HILDON_WIDGETS_CFLAGS) \
	$(MODEST_HILDON_HELP_CFLAGS)    \
	$(MODEST_MOZEMBED_CFLAGS) \
	$(MODEST_HILDON_NOTIFY_CFLAGS) \
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
	modest-dnd.c                   \
	modest-dnd.h                   \
	modest-folder-view.c           \
	modest-folder-view.h           \
	modest-global-settings-dialog.c \
	modest-global-settings-dialog.h \
	modest-gtkhtml-mime-part-view.c \
	modest-gtkhtml-mime-part-view.h \
	modest-hbox-cell-renderer.c    \
	modest-hbox-cell-renderer.h    \
	modest-vbox-cell-renderer.c    \
	modest-vbox-cell-renderer.h    \
	modest-header-view-observer.c  \
	modest-header-view-observer.h  \
	modest-header-view-render.c    \
	modest-header-view.c           \
	modest-header-view.h           \
	modest-isearch-view.c          \
	modest-isearch-view.h          \
	modest-limit-retrieve-combo-box.h \
	modest-limit-retrieve-combo-box.c \
	modest-mail-header-view.c      \
	modest-mail-header-view.h      \
	modest-main-window.h	       \
	modest-mime-part-view.c        \
	modest-mime-part-view.h        \
	modest-msg-edit-window-ui.h    \
	modest-msg-edit-window.h       \
	modest-msg-view-window.h       \
	modest-msg-view.c              \
	modest-msg-view.h	       \
	modest-gtkhtml-msg-view.c              \
	modest-gtkhtml-msg-view.h	       \
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
	modest-window-mgr.c            \
	modest-zoomable.h              \
	modest-zoomable.c

if BUILD_MOZEMBED
libmodest_widgets_la_SOURCES+=          \
	modest-mozembed-msg-view.c              \
	modest-mozembed-msg-view.h	       \
	modest-mozembed-mime-part-view.c \
	modest-mozembed-mime-part-view.h 	
endif

LDADD = \
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_MOZEMBED_LIBS)                         \
	$(MODEST_PLATFORM_LIBS)				\
	$(MODEST_HILDON_WIDGETS_LIBS) 
