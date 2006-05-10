#
# Makefile.am
# Time-stamp: <2006-04-24 00:08:56 (djcb)>
#
#
# use Automake 'trick' ==> convenience static libraries, which
# won't be installed, just for linking
#
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_GTK_CFLAGS) \
	$(MODEST_TINYMAIL_CFLAGS)

noinst_LTLIBRARIES=\
	libmodest-ui.la

libmodest_ui_la_SOURCES=\
	modest-ui.c\
	modest-main-window.h\
	modest-main-window.c\
	modest-edit-window.h\
	modest-edit-window.c

LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_GTK_LIBS) \
	$(MODEST_TINYMAIL_LIBS)
