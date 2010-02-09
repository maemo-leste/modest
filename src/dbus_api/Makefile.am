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
	$(LIBMODEST_DBUS_CLIENT_CFLAGS) \
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_PLATFORM_CFLAGS) \
	$(MODEST_TOOLKIT_CFLAGS)\
	$(MODEST_PLUGIN_DEFINES)\
	$(MODEST_OGS_CFLAGS) \
	-I ${top_srcdir}/src/widgets  \
	-I ${top_srcdir}/src  \
	-I ${top_srcdir}/src/$(MODEST_TOOLKIT_DIR) \
	-DPREFIX=\"@prefix@\" \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-DPROVIDER_DATA_DIR=\""${datadir}/modest/provider-data/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-dbus-api.la

libmodest_dbus_api_la_SOURCES=              \
	modest-dbus-callbacks.h modest-dbus-callbacks.c \
	modest-dbus-api.h

if MODEST_BUILD_ASDBUS
libmodest_dbus_api_la_SOURCES+= \
	asdbus.h asdbus.c asdbus-bindings.h
endif

LDADD = \
	$(LIBMODEST_DBUS_CLIENT_LIBS)
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_PLATFORM_LIBS) \
	$(MODEST_TOOLKIT_LIBS) \
	$(MODEST_OGS_LIBS)

