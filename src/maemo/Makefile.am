# Copyright (c) 2006,2007 Nokia Corporation
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# * Neither the name of the Nokia Corporation nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SUBDIRS=easysetup

INCLUDES=\
	$(LIBMODEST_DBUS_CLIENT_CFLAGS) \
	$(MODEST_GSTUFF_CFLAGS)             \
	$(MODEST_WPEDITOR_CFLAGS)           \
	$(MODEST_PLATFORM_CFLAGS)           \
	$(MODEST_TOOLKIT_CFLAGS)           \
	$(MODEST_PLUGIN_DEFINES)	    \
	$(MODEST_ABOOK_CFLAGS)              \
	$(MODEST_HILDON_HELP_CFLAGS)        \
	$(MODEST_LIBCONIC_CFLAGS)	    \
	$(MODEST_HILDON_MIME_CFLAGS)        \
	$(MODEST_HILDON_NOTIFY_CFLAGS)      \
	$(MODEST_LIBALARM_CFLAGS)           \
	-I ${top_srcdir}/src/widgets        \
	-I ${top_srcdir}/src                \
	-DPREFIX=\"@prefix@\" 	            \
	-DMAEMO_CHANGES                     \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

libmodest_ui_la_SOURCES=              \
	modest-account-view-window.c  \
	modest-default-account-settings-dialog.c \
	modest-icon-names.h           \
	modest-maemo-global-settings-dialog.c \
	modest-maemo-global-settings-dialog.h \
	modest-maemo-security-options-view.c \
	modest-main-window.c          \
	modest-main-window-ui.h       \
	modest-hildon-includes.h      \
	modest-hildon-sort-dialog.c   \
	modest-hildon-sort-dialog.h   \
	modest-platform.c	      \
	modest-signature-editor-dialog.c         \
	modest-signature-editor-dialog.h         \
	modest-msg-view-window.c      \
	modest-msg-edit-window.c      \
	modest-maemo-utils.c          \
	modest-connection-specific-smtp-window.h \
	modest-connection-specific-smtp-window.c \
	modest-connection-specific-smtp-edit-window.h \
	modest-connection-specific-smtp-edit-window.c \
	modest-osso-autosave-callbacks.c \
	modest-osso-autosave-callbacks.h \
	modest-osso-state-saving.c \
	modest-osso-state-saving.h   

if MODEST_USE_DUMMY_ADDRESS_BOOK
#	 nothing 
else
libmodest_ui_la_SOURCES += modest-address-book.c
endif 


libmodest_ui_la_LIBADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_ABOOK_LIBS) \
	$(MODEST_PLATFORM_LIBS) \
	$(MODEST_TOOLKIT_LIBS) \
	$(MODEST_WPEDITOR_LIBS) \
	$(MODEST_HILDON_MIME_LIBS) \
	$(MODEST_LIBCONIC_LIBS) \
	$(MODEST_HILDON_HELP_LIBS) \
	$(MODEST_LIBALARM_LIBS)   \
	$(MODEST_HILDON_NOTIFY_LIBS) \
	easysetup/libmodest-easysetup.la 

PIXMAP_FILES=\
	pixmaps/*.png 

pixmapdir = $(datadir)/pixmaps/modest
pixmap_DATA = $(PIXMAP_FILES)



UI_FILES=\
	ui/modest-main-window-ui.xml \
	ui/modest-msg-edit-window-ui.xml \
	ui/modest-msg-view-window-ui.xml

uidir = $(datadir)/modest/ui
ui_DATA = $(UI_FILES)

serviceentry_DATA   = com.nokia.modest.service
desktopentry_DATA   = modest.desktop
backupsettings_DATA = modest.conf
ossocudscripts_DATA = modest-clean.sh

icon_26x26_DATA     = pixmaps/modest-icon.png

EXTRA_DIST=\
	$(UI_FILES) \
	$(PIXMAP_FILES) \
	$(serviceentry_DATA) \
	$(desktopentry_DATA) \
	$(backupsettings_DATA) \
	$(ossocudscripts_DATA) \
	$(icon_26x26_DATA)	
