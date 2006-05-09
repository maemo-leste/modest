#
# Makefile.am
# Time-stamp: <2006-04-26 16:39:54 (djcb)>
SUBDIRS=gtk gtk-glade hildon

#FIXME: make the gconf stuff conditional

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS)\
	$(MODEST_GCONF_CFLAGS)\
	$(MODEST_GTK_CFLAGS)\
	$(MODEST_TINYMAIL_CFLAGS)

bin_PROGRAMS=\
	modest

if MODEST_ENABLE_GCONF
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
	modest-ui.h
else
modest_SOURCES=\
	modest-main.c\
	modest-conf.h \
	modest-conf-keys.h\
	modest-account-keys.h\
	modest-account-mgr.h\
	modest-account-mgr.c\
	modest-ui.h
endif

modest_LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_GCONF_LIBS) \
	$(MODEST_GTK_LIBS)\
	$(MODEST_TINYMAIL_LIBS)\
	$(MODEST_UI_DIR)/libmodest-ui.la 
