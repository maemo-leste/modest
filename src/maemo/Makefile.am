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
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_MAEMO_CFLAGS) \
	-DMODEST_PLATFORM_ID=$(MODEST_PLATFORM_ID) \
	-I ${top_srcdir}/src/widgets  \
	-I ${top_srcdir}/src  \
	-DPREFIX=\"@prefix@\" \
	-DMAEMO_CHANGES \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

libmodest_ui_la_SOURCES=              \
	modest-account-view-window.c  \
	modest-address-book.c	      \
	modest-icon-names.h           \
	modest-maemo-ui-constants.h \
	modest-main-window.c          \
	modest-main-window-ui.h       \
	modest-hildon-includes.h      \
	modest-platform.c	      \
	modest-signature-editor-dialog.c         \
	modest-signature-editor-dialog.h         \
	modest-scroll-area.h          \
	modest-scroll-area.c          \
	modest-store-widget.c         \
	modest-store-widget.h         \
	modest-transport-widget.c     \
	modest-transport-widget.h     \
	modest-msg-view-window.c      \
	modest-msg-edit-window.c      \
	modest-maemo-utils.c          \
	modest-account-settings-dialog.h \
	modest-account-settings-dialog.c \
	modest-connection-specific-smtp-window.h \
	modest-connection-specific-smtp-window.c \
	modest-connection-specific-smtp-edit-window.h \
	modest-connection-specific-smtp-edit-window.c \
	modest-progress-bar-widget.c      \
	modest-progress-bar-widget.h \
	modest-osso-autosave-callbacks.c \
	modest-osso-autosave-callbacks.h \
	modest-osso-state-saving.c \
	modest-osso-state-saving.h   


libmodest_ui_la_LIBADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_MAEMO_LIBS) \
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

serviceentry_DATA = modest.service
desktopentry_DATA = modest.desktop

icon_26x26_DATA    = pixmaps/modest-icon.png

EXTRA_DIST=\
	$(UI_FILES) \
	$(PIXMAP_FILES) \
	$(serviceentry_DATA) \
	$(desktopentry_DATA) \
	$(icon_26x26_DATA)	
