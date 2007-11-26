#
# Makefile.am
# Time-stamp: <2007-11-14 22:42:34 (djcb)>

SUBDIRS=$(MODEST_PLATFORM) widgets $(dbus_api)

DIST_SUBDIRS = widgets gnome maemo dbus_api

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_PLATFORM_CFLAGS) \
	$(MODEST_HILDON_WIDGETS_CFLAGS) \
	$(MODEST_OGS_CFLAGS) \
	$(MODEST_HILDON_HELP_CFLAGS) \
	$(MODEST_LIBCONIC_CFLAGS) \
	$(MODEST_HILDON_NOTIFY_CFLAGS) \
	$(MODEST_MOZEMBED_CFLAGS) \
	-I$(MODEST_PLATFORM) \
	-DPREFIX=\"@prefix@\" \
	-DMAEMO_CHANGES \
	-DMODEST_LOCALEDIR=\"$(MODEST_LOCALEDIR)\" \
	-DMODEST_PLATFORM=\"$(MODEST_PLATFORM)\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\"$(datadir)/modest/ui/\"

BUILT_SOURCES = modest-marshal.c modest-marshal.h

modest-marshal.c: modest-marshal.list
	glib-genmarshal --body --prefix=modest_marshal < modest-marshal.list > modest-marshal.c

modest-marshal.h: modest-marshal.list
	glib-genmarshal --header --prefix=modest_marshal < modest-marshal.list > modest-marshal.h

bin_PROGRAMS=\
	modest

#
# NOTE: $(addressbook) will be modest-addressbook-dummy.c
# if no real addressbook has been defined (configure.ac)
#
modest_SOURCES=\
	modest-account-mgr-helpers.c \
	modest-account-mgr-helpers.h \
	modest-account-mgr-priv.h \
	modest-account-mgr.c \
	modest-account-mgr.h \
	modest-address-book.h \
	modest-cache-mgr.c \
	modest-cache-mgr.h \
	modest-conf.c \
	modest-conf.h \
	modest-defs.h \
	modest-debug.h \
	modest-dimming-rule.c \
	modest-dimming-rule.h \
	modest-dimming-rules-group.c \
	modest-dimming-rules-group.h \
	modest-email-clipboard.c \
	modest-email-clipboard.h \
	modest-error.h \
	modest-formatter.c \
	modest-formatter.h \
	modest-init.c \
	modest-init.h \
	modest-local-folder-info.c \
	modest-local-folder-info.h \
	modest-mail-operation-queue.c \
	modest-mail-operation-queue.h \
	modest-mail-operation.c \
	modest-mail-operation.h \
	modest-main.c \
	modest-marshal.c \
	modest-marshal.h \
	modest-pair.c \
	modest-pair.h \
	modest-platform.h \
	modest-platform.h \
	modest-progress-object.c \
	modest-progress-object.h \
	modest-protocol-info.c \
	modest-protocol-info.h \
	modest-runtime-priv.h \
	modest-runtime.c \
	modest-runtime.h \
	modest-search.c \
	modest-search.h \
	modest-signal-mgr.c \
	modest-signal-mgr.h \
	modest-singletons.c \
	modest-singletons.h \
	modest-text-utils.c \
	modest-text-utils.h \
	modest-tny-account-store.c \
	modest-tny-account-store.h \
	modest-tny-account.c \
	modest-tny-account.h \
	modest-tny-folder.c \
	modest-tny-folder.h \
	modest-tny-local-folders-account.c \
	modest-tny-local-folders-account.h \
	modest-tny-msg.c \
	modest-tny-msg.h \
	modest-tny-outbox-account.c \
	modest-tny-outbox-account.h \
	modest-tny-platform-factory.c \
	modest-tny-platform-factory.h \
	modest-tny-send-queue.c \
	modest-tny-send-queue.h \
	modest-transport-account-decorator.c \
	modest-transport-account-decorator.h \
	modest-stream-text-to-html.c \
	modest-stream-text-to-html.h \
	modest-ui-actions.c \
	modest-ui-actions.h \
	modest-ui-dimming-manager.c \
	modest-ui-dimming-manager.h \
	modest-ui-dimming-rules.c \
	modest-ui-dimming-rules.h \
	modest-widget-memory-priv.h \
	modest-widget-memory.c \
	modest-widget-memory.h 

if MODEST_USE_DUMMY_ADDRESS_BOOK
modest_SOURCES += modest-address-book-dummy.c
endif 


modest_CFLAGS  = $(MAEMO_LAUNCHER_CFLAGS)
modest_LDFLAGS = $(MAEMO_LAUNCHER_LDFLAGS)

modest_LDADD = 						\
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_MOZEMBED_LIBS) 				\
	$(MODEST_PLATFORM_LIBS)	                        \
	$(MODEST_HILDON_WIDGETS_LIBS)			\
	$(MODEST_HILDON_HELP_LIBS)			\
	$(MODEST_HILDON_NOTIFY_LIBS)			\
	$(MODEST_OGS_LIBS)				\
	${easysetupmaybe}				\
	$(MODEST_PLATFORM)/libmodest-ui.la		\
	widgets/libmodest-widgets.la


if MODEST_HAVE_DBUS_API
modest_LDADD +=	dbus_api/libmodest-dbus-api.la
endif

EXTRA_DIST=modest-marshal.list
