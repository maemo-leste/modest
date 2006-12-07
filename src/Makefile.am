#
# Makefile.am
# Time-stamp: <2006-11-28 17:42:40 (djcb)>
SUBDIRS=$(MODEST_PLATFORM) widgets
DIST_SUBDIRS = widgets gtk

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-I$(MODEST_PLATFORM) \
	-DPREFIX=\"@prefix@\" \
	-DMODESTLOCALEDIR=\""$(modestlocaledir)"\" \
	-DMODEST_PLATFORM=\"$(MODEST_PLATFORM)\" \
	-DMODEST_PLATFORM_ID=$(MODEST_PLATFORM_ID)

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
	modest-pair.h\
	modest-pair.c\
	modest-widget-factory.h	\
	modest-widget-factory.c	\
	modest-widget-memory.h\
	modest-widget-memory.c\
	modest-account-keys.h\
	modest-account-mgr.h\
	modest-account-mgr.c\
	modest-icon-factory.c\
	modest-icon-factory.h\
	modest-tny-account-store.h\
	modest-tny-account-store.c\
	modest-tny-msg-actions.h\
	modest-tny-msg-actions.c\
	modest-protocol-mgr.h\
	modest-protocol-mgr.c\
	modest-ui.h\
	modest-text-utils.h\
	modest-text-utils.c\
	modest-tny-stream-gtkhtml.h\
	modest-tny-stream-gtkhtml.c\
	modest-tny-attachment.h\
	modest-tny-attachment.c\
	modest-marshal.c \
	modest-marshal.h \
	modest-mail-operation.c \
	modest-mail-operation.h \
	modest-mail-operation-queue.c \
	modest-mail-operation-queue.h \
	modest-tny-platform-factory.c \
	modest-tny-platform-factory.h \
	modest-formatter.c \
	modest-formatter.h

modest_LDADD = 						\
	$(MODEST_GSTUFF_LIBS) 				\
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)	\
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) 		\
	$(MODEST_PLATFORM)/libmodest-ui.la		\
	widgets/libmodest-widgets.la			

EXTRA_DIST=modest-marshal.list
