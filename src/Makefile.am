#
# Makefile.am
# Time-stamp: <2006-06-02 16:29:55 (djcb)>
SUBDIRS=$(MODEST_UI_DIR)

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_UI_DIR) -Wall\
	-DPREFIX=\"@prefix@\" \
	-Wall

BUILT_SOURCES = modest-marshal.c modest-marshal.h

modest-marshal.c:
	glib-genmarshal --body --prefix=modest_marshal < marshall.list > modest-marshal.c
	
modest-marshal.h:
	glib-genmarshal --header --prefix=modest_marshal < marshall.list > modest-marshal.h


bin_PROGRAMS=\
	modest

modest_SOURCES=\
	modest-main.c\
	modest-conf-keys.h\
	modest-conf.h \
	modest-conf-gconf.c\
	modest-identity-keys.h\
	modest-identity-mgr.h\
	modest-identity-mgr.c\
	modest-account-keys.h\
	modest-account-mgr.h\
	modest-account-mgr.c\
	modest-window-mgr.h\
	modest-window-mgr.c\
	modest-icon-factory.c\
	modest-icon-factory.h\
	modest-tny-account-store.h\
	modest-tny-account-store.c\
	modest-tny-folder-tree-view.h\
	modest-tny-folder-tree-view.c\
	modest-tny-header-tree-view.h\
	modest-tny-header-tree-view.c\
	modest-tny-msg-view.h\
	modest-tny-msg-view.c\
	modest-tny-msg-actions.h\
	modest-tny-msg-actions.c\
	modest-proto.h\
	modest-proto.c\
	modest-ui.h\
	modest-text-utils.h\
	modest-text-utils.c\
	modest-tny-stream-gtkhtml.h\
	modest-tny-stream-gtkhtml.c\
	modest-tny-transport-actions.c\
	modest-tny-transport-actions.h\
	modest-tny-store-actions.c\
	modest-tny-store-actions.h\
	modest-editor-window.c\
	modest-editor-window.h\
	modest-viewer-window.c\
	modest-viewer-window.h\
	modest-ui.h \
	modest-marshal.c \
	modest-marshal.h

modest_LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) \
	$(MODEST_UI_DIR)/libmodest-ui.la 
