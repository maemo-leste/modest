#
# Makefile.am
# Time-stamp: <2007-01-15 10:49:27 (djcb)>
SUBDIRS=$(MODEST_PLATFORM) widgets
DIST_SUBDIRS = widgets gtk maemo

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I ${top_srcdir}/src/widgets \
	-I$(MODEST_PLATFORM) \
	-DPREFIX=\"@prefix@\" \
	-DMODEST_LOCALEDIR=\"$(MODEST_LOCALEDIR)\" \
	-DMODEST_PLATFORM=\"$(MODEST_PLATFORM)\" \
	-DMODEST_PLATFORM_ID=$(MODEST_PLATFORM_ID) \
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
	modest-account-keys.h\
	modest-account-mgr.c\
	modest-account-mgr.h\
	modest-conf-keys.h\
	modest-conf.c\
	modest-conf.h \
	modest-debug.c\
	modest-debug.h\
	modest-error.h \
	modest-formatter.c \
	modest-formatter.h \
	modest-icon-factory.c\
	modest-icon-factory.h\
	modest-mail-operation-queue.c \
	modest-mail-operation-queue.h \
	modest-mail-operation.c \
	modest-mail-operation.h \
	modest-main.c\
	modest-marshal.c \
	modest-marshal.h \
	modest-pair.c\
	modest-pair.h\
	modest-presets.c\
	modest-presets.h\
	modest-protocol-info.c\
	modest-protocol-info.h\
	modest-text-utils.c\
	modest-text-utils.h\
	modest-tny-account-store.c\
	modest-tny-account-store.h\
	modest-tny-folder.c \
	modest-tny-folder.h \
	modest-tny-msg-actions.c\
	modest-tny-msg-actions.h\
	modest-tny-platform-factory.c \
	modest-tny-platform-factory.h \
	modest-ui.c\
	modest-ui.h\
	modest-widget-factory.c	\
	modest-widget-factory.h	\
	modest-widget-memory.c\
	modest-widget-memory.h

modest_LDADD = 						\
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)	\
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) 		\
	$(MODEST_PLATFORM)/libmodest-ui.la		\
	widgets/libmodest-widgets.la			

EXTRA_DIST=modest-marshal.list
