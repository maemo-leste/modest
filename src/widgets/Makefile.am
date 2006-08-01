#
# Makefile.am
# Time-stamp: <2006-07-30 20:16:54 (djcb)>

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	-I${top_srcdir}/src/$(MODEST_PLATFORM_DIR) \
	-DPREFIX=\"@prefix@\" \
	 -I ${top_srcdir}/src \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-widgets.la

libmodest_widgets_la_SOURCES=    \
	modest-account-view.c    \
	modest-account-view.h    \
	modest-folder-view.c     \
	modest-folder-view.h     \
	modest-header-view.c     \
	modest-header-view.h     \
	modest-msg-view.c        \
	modest-msg-view.h        



#	modest-toolbar.h	 
#	modest-toolbar.c	
 
LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)
