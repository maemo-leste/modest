#
# Makefile.am
# Time-stamp: <2008-01-06 10:18:49 (djcb)>

SUBDIRS= $(MODEST_TOOLKIT_DIR) widgets $(dbus_api)
DIST_SUBDIRS = widgets gtk hildon2 dbus_api

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)					\
	$(MODEST_PLATFORM_CFLAGS)				\
	$(MODEST_TOOLKIT_CFLAGS)				\
	$(MODEST_PLUGIN_DEFINES)				\
	$(MODEST_OGS_CFLAGS)					\
	$(MODEST_HILDON_HELP_CFLAGS)				\
	$(MODEST_LIBCONIC_CFLAGS)				\
	$(MODEST_HILDON_NOTIFY_CFLAGS)				\
	$(MODEST_MOZEMBED_CFLAGS)				\
	$(MODEST_WEBKIT_CFLAGS)				\
	-fPIC 							\
	-I$(MODEST_TOOLKIT_DIR)					\
	-DPREFIX=\"@prefix@\"					\
	-DMODEST_LOCALEDIR=\"$(MODEST_LOCALEDIR)\"		\
	-DMODEST_PLATFORM=\"$(MODEST_PLATFORM)\"		\
	-DMODEST_TOOLKIT=\"$(MODEST_TOOLKIT)\"		\
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\"		\
	-DMODEST_UIDIR=\"$(datadir)/modest/ui/\"		\
	-DMODEST_MAILPLUGINDIR=\"$(libdir)/modest/mail-plugins\"

BUILT_SOURCES = modest-marshal.c modest-marshal.h

modest-marshal.c: modest-marshal.list
	glib-genmarshal --body --prefix=modest_marshal - < modest-marshal.list > modest-marshal.c

modest-marshal.h: modest-marshal.list
	glib-genmarshal --header --prefix=modest_marshal - < modest-marshal.list > modest-marshal.h

noinst_LTLIBRARIES = libmodest.la

bin_PROGRAMS=\
	modest

#
# NOTE: $(addressbook) will be modest-addressbook-dummy.c
# if no real addressbook has been defined (configure.ac)
#

modest_public_headers = \
	modest-account-mgr.h \
	modest-account-mgr-helpers.h \
	modest-account-protocol.h \
	modest-account-settings.h \
	modest-cache-mgr.h \
	modest-conf.h \
	modest-defs.h \
	modest-local-folder-info.h \
	modest-mail-operation.h \
	modest-mail-operation-queue.h \
	modest-pair.h \
	modest-platform.h \
	modest-plugin.h \
	modest-plugin-ui-actions.h \
	modest-protocol.h \
	modest-protocol-registry.h \
	modest-server-account-settings.h \
	modest-signal-mgr.h \
	modest-text-utils.h \
	modest-tny-account-store.h \
	modest-tny-folder.h \
	modest-tny-local-folders-account.h \
	modest-tny-send-queue.h \
	modest-utils.h

libmodest_la_SOURCES=\
	$(modest_public_headers) \
	modest-account-mgr-helpers.c \
	modest-account-mgr-priv.h \
	modest-account-mgr.c \
	modest-account-protocol.c \
	modest-account-settings.c \
	modest-address-book.h \
	modest-cache-mgr.c \
	modest-conf.c \
	modest-count-stream.c \
	modest-count-stream.h \
	modest-datetime-formatter.c \
	modest-datetime-formatter.h \
	modest-debug.h \
	modest-defs.c \
	modest-dimming-rule.c \
	modest-dimming-rule.h \
	modest-dimming-rules-group.c \
	modest-dimming-rules-group.h \
	modest-email-clipboard.h \
	modest-email-clipboard.c \
	modest-error.h \
	modest-formatter.c \
	modest-formatter.h \
	modest-init.c \
	modest-init.h \
	modest-local-folder-info.c \
	modest-mail-operation-queue.c \
	modest-mail-operation-queue.h \
	modest-mail-operation.c \
	modest-mail-operation.h \
	modest-main.c \
	modest-marshal.c \
	modest-marshal.h \
	modest-module.c \
	modest-module.h \
	modest-pair.c \
	modest-plugin.c \
	modest-plugin-factory.c \
	modest-plugin-factory.h \
	modest-presets.c \
	modest-presets.h \
	modest-progress-object.c \
	modest-progress-object.h \
	modest-protocol.c \
	modest-protocol-registry.c \
	modest-runtime-priv.h \
	modest-runtime.c \
	modest-runtime.h \
	modest-search.c \
	modest-search.h \
	modest-signal-mgr.c \
	modest-signal-mgr.h \
	modest-singletons.c \
	modest-singletons.h \
	modest-server-account-settings.c \
	modest-text-utils.c \
	modest-tny-account-store.c \
	modest-tny-account.c \
	modest-tny-account.h \
	modest-tny-folder.c \
	modest-tny-local-folders-account.c \
	modest-tny-mime-part.c \
	modest-tny-mime-part.h \
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
	modest-stream-html-to-text.c \
	modest-stream-html-to-text.h \
	modest-ui-actions.c \
	modest-ui-actions.h \
	modest-ui-dimming-manager.c \
	modest-ui-dimming-manager.h \
	modest-ui-dimming-rules.c \
	modest-ui-dimming-rules.h \
	modest-utils.c \
	modest-widget-memory-priv.h \
	modest-widget-memory.c \
	modest-widget-memory.h \
	modest-default-connection-policy.c \
	modest-default-connection-policy.h 

if MODEST_USE_DUMMY_ADDRESS_BOOK
libmodest_la_SOURCES += modest-address-book-dummy.c
endif


libmodest_la_CFLAGS  = $(MAEMO_LAUNCHER_CFLAGS)
libmodest_la_LIBADD = 						\
	$(MAEMO_LAUNCHER_LIBS) 				\
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_MOZEMBED_LIBS) 			\
	$(MODEST_WEBKIT_LIBS) 			\
	$(MODEST_PLATFORM_LIBS)	                        \
	$(MODEST_TOOLKIT_LIBS)			\
	$(MODEST_HILDON_HELP_LIBS)			\
	$(MODEST_HILDON_NOTIFY_LIBS)			\
	$(MODEST_OGS_LIBS)				\
	$(MODEST_LIBTIME_LIBS)                          \
	${easysetupmaybe} \
	widgets/libmodest-widgets.la                    \
	$(MODEST_TOOLKIT_DIR)/libmodest-ui.la
libmodest_la_LDFLAGS = -rdynamic -no-undefined

if MODEST_HAVE_DBUS_API
libmodest_la_LIBADD +=	dbus_api/libmodest-dbus-api.la
endif


modest_includedir = $(includedir)/modest-plugin-1.0/
modest_include_HEADERS = $(modest_public_headers)

modest_SOURCES = modest-main.c

modest_DEPENDENCIES = \
	libmodest.la

modest_CFLAGS = $(MAEMO_LAUNCHER_CFLAGS)
modest_LDADD = \
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_MOZEMBED_LIBS) 			\
	$(MODEST_PLATFORM_LIBS)	                        \
	$(MODEST_TOOLKIT_LIBS)			\
	$(MODEST_HILDON_HELP_LIBS)			\
	$(MODEST_HILDON_NOTIFY_LIBS)			\
	$(MODEST_OGS_LIBS)				\
	$(MODEST_LIBTIME_LIBS)                          \
	${easysetupmaybe}				\
	libmodest.la

modest_LDFLAGS = -Wl,--version-script=export.map $(MAEMO_LAUNCHER_LIBS)

# modest_UNWANTED = -export-dynamic -Wl,--export-dynamic


# mcc_mapping may be provided by the operator-wizard-settings package:
# the one here is a copy, as that package is not available in chinook
MMC_MAPPING_FILE=mcc_mapping
mmccmappingdir=$(datadir)/modest/provider-data
mmccmapping_DATA = $(MMC_MAPPING_FILE)

PROVIDER_DATA_FILE=fallback-provider-data.keyfile
providerdatadir=$(datadir)/modest/provider-data
providerdata_DATA = $(PROVIDER_DATA_FILE)

EXTRA_DIST=modest-marshal.list \
	$(MMC_MAPPING_FILE) \
	$(PROVIDER_DATA_FILE)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = modest-plugin-1.0.pc

DISTCLEANFILES = $(pkgconfig_DATA)
