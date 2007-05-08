#
# Makefile.am
# Time-stamp: <2007-04-21 11:15:36 (djcb)>
SUBDIRS=$(MODEST_PLATFORM) widgets

if BUILD_DBUSAPI
SUBDIRS += dbus_api
endif

DIST_SUBDIRS = widgets gnome maemo dbus_api

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_PLATFORM) \
	-DPREFIX=\"@prefix@\" \
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

modest_SOURCES=\
	modest-account-mgr-helpers.c\
	modest-account-mgr-helpers.h\
	modest-account-mgr-priv.c\
	modest-account-mgr-priv.h\
	modest-account-mgr.c\
	modest-account-mgr.h\
	modest-address-book.h\
	modest-cache-mgr.c\
	modest-cache-mgr.h\
	modest-conf.c\
	modest-conf.h \
	modest-defs.h\
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
	modest-main.c\
	modest-marshal.c \
	modest-marshal.h \
	modest-pair.c\
	modest-pair.h\
	modest-platform.h\
	modest-progress-object.c\
	modest-progress-object.h\
	modest-protocol-info.c\
	modest-protocol-info.h\
	modest-runtime.c\
	modest-runtime.h\
	modest-runtime-priv.h\
	modest-singletons.c\
	modest-singletons.h\
	modest-text-utils.c\
	modest-text-utils.h\
	modest-tny-account-store.c\
	modest-tny-account-store.h\
	modest-tny-account.c\
	modest-tny-account.h\
	modest-tny-folder.c \
	modest-tny-folder.h \
	modest-tny-msg.c\
	modest-tny-msg.h\
	modest-platform.h\
	modest-tny-platform-factory.c \
	modest-tny-platform-factory.h \
	modest-tny-send-queue.c\
	modest-tny-send-queue.h\
	modest-ui-actions.c\
	modest-ui-actions.h\
	modest-widget-memory-priv.h \
	modest-widget-memory.c\
	modest-widget-memory.h 

modest_LDADD = 						\
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)	\
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) 		\
	${easysetupmaybe}				\
	$(MODEST_PLATFORM)/libmodest-ui.la		\
	widgets/libmodest-widgets.la

if BUILD_DBUSAPI
	modest_LDADD += dbus_api/libmodest-dbus-api.la		
endif

EXTRA_DIST=modest-marshal.list
