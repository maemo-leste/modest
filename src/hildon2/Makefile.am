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
	$(LIBMODEST_DBUS_CLIENT_CFLAGS)     \
	$(MODEST_GSTUFF_CFLAGS)             \
	$(MODEST_WPEDITOR_CFLAGS)           \
	$(MODEST_PLATFORM_CFLAGS)           \
	$(MODEST_TOOLKIT_CFLAGS)            \
	$(MODEST_PLUGIN_DEFINES)            \
	$(MODEST_ABOOK_CFLAGS)              \
	$(MODEST_HILDON_HELP_CFLAGS)        \
	$(MODEST_LIBCONIC_CFLAGS)	    \
	$(MODEST_HILDON_MIME_CFLAGS)        \
	$(MODEST_HILDON_NOTIFY_CFLAGS)      \
	$(MODEST_LIBALARM_CFLAGS)           \
	$(MODEST_CALENDAR_WIDGETS_CFLAGS)   \
	-I ${top_srcdir}/src/widgets        \
	-I ${top_srcdir}/src                \
	-DPREFIX=\"@prefix@\" 	            \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-ui.la

libmodest_ui_la_SOURCES=              \
	modest-country-picker.h modest-country-picker.c \
	modest-color-button.h modest-color-button.c \
	modest-provider-picker.h modest-provider-picker.c \
	modest-serversecurity-picker.h modest-serversecurity-picker.c \
	modest-servertype-picker.h modest-servertype-picker.c \
	modest-icon-names.h           \
	modest-hildon2-global-settings-dialog.c \
	modest-hildon2-global-settings-dialog.h \
	modest-maemo-security-options-view.c \
	modest-hildon-includes.h      \
	modest-hildon-find-toolbar.h  \
	modest-hildon-find-toolbar.c  \
	modest-hildon-pannable-area-scrollable.h	\
	modest-hildon-pannable-area-scrollable.c	\
	modest-hildon2-details-dialog.h \
	modest-hildon2-details-dialog.c \
	modest-hildon2-sort-dialog.c   \
	modest-hildon2-sort-dialog.h   \
	modest-hildon2-window.h \
	modest-hildon2-window.c \
	modest-platform.c	      \
	modest-hildon2-msg-view-window.h      \
	modest-hildon2-msg-view-window.c      \
	modest-maemo-utils.c          \
	modest-number-editor.c        \
	modest-number-editor.h        \
	modest-secureauth-picker.h modest-secureauth-picker.c \
	modest-selector-picker.h modest-selector-picker.c \
	modest-hildon2-window-mgr.h modest-hildon2-window-mgr.c


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
	$(MODEST_LIBIPHB_LIBS)   \
	$(MODEST_HILDON_NOTIFY_LIBS) \
	$(MODEST_CALENDAR_WIDGETS_LIBS)

UI_FILES=\
	ui/modest-main-window-ui.xml \
	ui/modest-msg-edit-window-ui.xml \
	ui/modest-msg-view-window-ui.xml

uidir = $(datadir)/modest/ui
ui_DATA = $(UI_FILES)

serviceentry_DATA   = com.nokia.modest.service
desktopentry_DATA   = modest.desktop
backupsettings_DATA = modest.conf
prebackup_SCRIPTS = modest-prebackup.sh
restore_SCRIPTS = modest-restore.sh
ossocudscripts_SCRIPTS = modest-clean.sh

EXTRA_DIST=\
	$(UI_FILES) \
	$(PIXMAP_FILES) \
	$(serviceentry_DATA) \
	$(desktopentry_DATA) \
	$(backupsettings_DATA) \
	$(prebackup_SCRIPTS) \
	$(restore_SCRIPTS) \
	$(ossocudscripts_SCRIPTS)
