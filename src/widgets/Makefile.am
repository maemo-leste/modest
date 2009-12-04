#
# Makefile.am
# Time-stamp: <2007-11-24 13:34:06 (djcb)>

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_PLATFORM_CFLAGS) \
	$(MODEST_TOOLKIT_CFLAGS) \
	$(MODEST_PLUGIN_DEFINES) \
	$(MODEST_HILDON_HELP_CFLAGS)    \
	$(MODEST_WEBKIT_CFLAGS) \
	$(MODEST_MOZEMBED_CFLAGS) \
	$(MODEST_WPEDITOR_CFLAGS)           \
	$(MODEST_LIBCONIC_CFLAGS)	    \
	$(MODEST_HILDON_NOTIFY_CFLAGS) \
	$(MODEST_CALENDAR_WIDGETS_CFLAGS)   \
	-I ${top_srcdir}/src/$(MODEST_TOOLKIT_DIR) \
	-I ${top_srcdir}/src \
	-DPREFIX=\"@prefix@\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-widgets.la


libmodest_widgets_public_headers = \
	modest-account-settings-dialog.h \
	modest-easysetup-wizard-page.h \
	modest-folder-view.h           \
	modest-sort-criterium-view.h  \
	modest-ui-constants.h \
	modest-validating-entry.h \
	modest-window.h	\
	modest-window-mgr.h	\
	modest-wizard-dialog.h


libmodest_widgets_la_SOURCES=          \
	$(libmodest_widgets_public_headers) \
	modest-account-view-window.h   \
	modest-account-view-window.c   \
	modest-account-settings-dialog.c \
	modest-account-view.c          \
	modest-account-view.h          \
	modest-accounts-window.c \
	modest-accounts-window.h \
	modest-attachment-view.c       \
	modest-attachment-view.h       \
	modest-attachments-view.c      \
	modest-attachments-view.h      \
	modest-combo-box.c	       \
	modest-combo-box.h             \
	modest-compact-mail-header-view.c \
	modest-compact-mail-header-view.h \
	modest-connection-specific-smtp-window.h \
	modest-connection-specific-smtp-window.c \
	modest-connection-specific-smtp-edit-window.h \
	modest-connection-specific-smtp-edit-window.c \
	modest-country-combo-box.c \
	modest-country-combo-box.h \
	modest-default-account-settings-dialog.h \
	modest-default-account-settings-dialog.c \
	modest-default-global-settings-dialog.h \
	modest-default-global-settings-dialog.c \
	modest-dnd.c                   \
	modest-dnd.h                   \
	modest-easysetup-wizard-dialog.h modest-easysetup-wizard-dialog.c \
	modest-easysetup-wizard-page.c \
	modest-find-toolbar.c          \
	modest-find-toolbar.h          \
	modest-folder-view.c           \
	modest-folder-window.c         \
	modest-folder-window.h         \
	modest-global-settings-dialog.c \
	modest-global-settings-dialog.h \
	modest-gtkhtml-mime-part-view.c \
	modest-gtkhtml-mime-part-view.h \
	modest-gtk-details-dialog.c \
	modest-gtk-details-dialog.h \
	modest-gtk-security-options-view.c \
	modest-gtk-security-options-view.h \
	modest-hbox-cell-renderer.c    \
	modest-hbox-cell-renderer.h    \
	modest-vbox-cell-renderer.c    \
	modest-vbox-cell-renderer.h    \
	modest-header-view-observer.c  \
	modest-header-view-observer.h  \
	modest-header-view-render.c    \
	modest-header-view.c           \
	modest-header-view.h           \
	modest-header-window.c         \
	modest-header-window.h         \
	modest-isearch-toolbar.c       \
	modest-isearch-toolbar.h       \
	modest-isearch-view.c          \
	modest-isearch-view.h          \
	modest-limit-retrieve-combo-box.h \
	modest-limit-retrieve-combo-box.c \
	modest-mail-header-view.h \
	modest-mail-header-view.c \
	modest-mailboxes-window.c \
	modest-mailboxes-window.h \
	modest-main-window.h	       \
	modest-mime-part-view.c        \
	modest-mime-part-view.h        \
	modest-msg-edit-window-ui.h    \
	modest-msg-edit-window.h       \
	modest-msg-edit-window.c       \
	modest-msg-view-window.h       \
	modest-msg-view-window.c       \
	modest-msg-view.c              \
	modest-msg-view.h	       \
	modest-gtkhtml-msg-view.c              \
	modest-gtkhtml-msg-view.h	       \
	modest-details-dialog.c        \
	modest-details-dialog.h        \
	modest-progress-bar.c	\
	modest-progress-bar.h	\
	modest-provider-combo-box.c    \
	modest-provider-combo-box.h    \
	modest-recpt-editor.c          \
	modest-recpt-editor.h          \
	modest-recpt-view.c            \
	modest-recpt-view.h            \
	modest-retrieve-combo-box.h    \
	modest-retrieve-combo-box.c    \
	modest-scrollable.h           \
	modest-scrollable.c           \
	modest-scroll-area.h          \
	modest-scroll-area.c          \
	modest-scroll-text.c           \
	modest-scroll-text.h           \
	modest-scrolled-window-scrollable.c \
	modest-scrolled-window-scrollable.h \
	modest-security-options-view.h	\
	modest-security-options-view.c	\
	modest-serversecurity-combo-box.h \
	modest-serversecurity-combo-box.c \
	modest-servertype-combo-box.h \
	modest-servertype-combo-box.c \
	modest-secureauth-combo-box.h \
	modest-secureauth-combo-box.c \
	modest-signature-editor-dialog.c         \
	modest-signature-editor-dialog.h         \
	modest-sort-criterium-view.c   \
	modest-sort-criterium-view.c   \
	modest-sort-criterium-view.h   \
	modest-tny-stream-gtkhtml.c    \
	modest-tny-stream-gtkhtml.h    \
	modest-toolkit-factory.c \
	modest-toolkit-factory.h \
	modest-toolkit-utils.c \
	modest-toolkit-utils.h \
	modest-validating-entry.c \
	modest-window.c                \
	modest-window-mgr-priv.h	\
	modest-window-mgr.h            \
	modest-window-mgr.c            \
	modest-wizard-dialog.h         \
	modest-wizard-dialog.c         \
	modest-wp-text-view.c \
	modest-wp-text-view.h \
	modest-zoomable.h              \
	modest-zoomable.c


libmodest_widgets_la_includedir = $(includedir)/modest-plugin-1.0/widgets/
libmodest_widgets_la_include_HEADERS = $(libmodest_widgets_public_headers)

LDADD = \
	$(MODEST_GSTUFF_LIBS)		\
	$(MODEST_MOZEMBED_LIBS)         \
	$(MODEST_WEBKIT_LIBS)           \
	$(MODEST_WPEDITOR_LIBS) 	\
	$(MODEST_PLATFORM_LIBS)		\
	$(MODEST_LIBCONIC_LIBS) 	\
	$(MODEST_TOOLKIT_LIBS) 		\
	$(MODEST_CALENDAR_WIDGETS_LIBS)

if MODEST_USE_WEBKIT
libmodest_widgets_la_SOURCES +=          \
	modest-webkit-mime-part-view.c   \
	modest-webkit-mime-part-view.h   \
	modest-webkit-msg-view.c         \
	modest-webkit-msg-view.h         \
	modest-tny-stream-webkit.c       \
	modest-tny-stream-webkit.h

endif

UI_FILES=\
	ui/modest-main-window-ui.xml \
	ui/modest-msg-edit-window-ui.xml \
	ui/modest-msg-view-window-ui.xml

uidir = $(datadir)/modest/ui
ui_DATA = $(UI_FILES)

EXTRA_DIST=\
	$(UI_FILES)
