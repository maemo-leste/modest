#
# Makefile.am
# Time-stamp: <2006-07-21 09:29:41 (djcb)>
SUBDIRS=$(MODEST_PLATFORM_DIR)
DIST_SUBDIRS = gtk gtk2 hildon

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_PLATFORM_DIR) \
	-DPREFIX=\"@prefix@\" 

if OLD_UI_STUFF
INCLUDES +=\
	-DOLD_UI_STUFF
endif

BUILT_SOURCES = modest-marshal.c modest-marshal.h

modest-marshal.c: modest-marshal.list
	glib-genmarshal --body --prefix=modest_marshal < modest-marshal.list > modest-marshal.c

modest-marshal.h: modest-marshal.list
	glib-genmarshal --header --prefix=modest_marshal < modest-marshal.list > modest-marshal.h


bin_PROGRAMS=\
	modest

modest_SOURCES=\
	modest-main.c\
	modest-conf-keys.h\
	modest-conf.h \
	modest-conf.c\
	modest-widget-factory.h\
	modest-widget-factory.c\
	modest-account-keys.h\
	modest-account-mgr.h\
	modest-account-mgr.c\
	modest-window-mgr.h\
	modest-window-mgr.c\
	modest-icon-factory.c\
	modest-icon-factory.h\
	modest-tny-account-store.h\
	modest-tny-account-store.c\
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
	modest-tny-attachment.h\
	modest-tny-attachment.c\
	modest-tny-store-actions.c\
	modest-tny-store-actions.h\
	modest-ui.h \
	modest-marshal.c \
	modest-marshal.h 

if OLD_UI_STUFF
modest_SOURCES += \
	modest-identity-keys.h\
	modest-identity-mgr.h\
	modest-identity-mgr.c\
	modest-editor-window.c\
	modest-editor-window.h\
	modest-viewer-window.c\
	modest-viewer-window.h
endif

modest_LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) \
	$(MODEST_PLATFORM_DIR)/libmodest-ui.la \
	widgets/libmodest-widgets.la
		

EXTRA_DIST=modest-marshal.list
