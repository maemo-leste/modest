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


INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	-DMODEST_PLATFORM_ID=$(MODEST_PLATFORM_ID) \
	-I ${top_srcdir}/src/widgets  \
	-I ${top_srcdir}/src  \
	-DPREFIX=\"@prefix@\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

libmodest_ui_la_SOURCES=              \
	modest-account-view-window.c  \
	modest-account-view-window.h  \
	modest-edit-msg-window.c      \
	modest-icon-names.h           \
	modest-main-window.c          \
	modest-store-widget.c         \
	modest-store-widget.h         \
	modest-transport-widget.c     \
	modest-transport-widget.h     


LDADD = \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_LIBTINYMAIL_GNOME_DESKTOP_LIBS)

PIXMAP_FILES=\
	pixmaps/*.png 

pixmapdir = $(datadir)/pixmaps/modest
pixmap_DATA = $(PIXMAP_FILES)

UI_FILES=\
	ui/modest-ui.xml \
	ui/modest-edit-msg-window-ui.xml

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
