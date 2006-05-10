#
# Makefile.am
# Time-stamp: <2006-05-08 21:11:31 (djcb)>
SUBDIRS=gtk gtk-glade hildon

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_GCONF_CFLAGS)\
	$(MODEST_GTK_CFLAGS)\
	$(MODEST_TINYMAIL_CFLAGS)\
	-DPREFIX=\"@prefix@\"

bin_PROGRAMS=\
	modest

modest_SOURCES=\
	modest-main.c\
	modest-conf-keys.h\
	modest-conf.h \
	modest-conf-gconf.c\
	modest-account-keys.h\
	modest-account-mgr.h\
	modest-account-mgr.c\
	modest-window-mgr.h\
	modest-window-mgr.c\
	modest-tny-account-store.h\
	modest-tny-account-store.c\
	modest-tny-folder-tree-view.h\
	modest-tny-folder-tree-view.c\
	modest-tny-header-tree-view.h\
	modest-tny-header-tree-view.c\
	modest-tny-msg-view.h\
	modest-tny-msg-view.c\
	modest-proto.h\
	modest-proto.c\
	modest-ui.h

modest_LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_GCONF_LIBS) \
	$(MODEST_GTK_LIBS)\
	$(MODEST_GLADE_LIBS) \
	$(MODEST_TINYMAIL_LIBS)\
	$(MODEST_UI_DIR)/libmodest-ui.la 
