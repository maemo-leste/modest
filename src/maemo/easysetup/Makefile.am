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

# HACK_TOP_SRCDIR is temporary and should be remove.
INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_PLATFORM_CFLAGS) \
	$(MODEST_HILDON_WIDGETS_CFLAGS) \
	$(MODEST_HILDON_HELP_CFLAGS) \
	$(MODEST_HILDON_NOTIFY_CFLAGS)\
	-I ${top_srcdir}/src/widgets  \
	-I ${top_srcdir}/src  \
	-DPREFIX=\"@prefix@\" \
	-DMAEMO_CHANGES \
	-DPIXMAP_PREFIX=\"${datadir}/pixmaps/modest/\" \
	-DMODEST_UIDIR=\""$(datadir)/modest/ui/"\" \
	-DHACK_TOP_SRCDIR=\"${top_srcdir}/\"  \
	-DPROVIDER_DATA_DIR=\""${datadir}/modest/provider-data/"\" \
	-Wall

noinst_LTLIBRARIES=\
	libmodest-easysetup.la

libmodest_easysetup_la_SOURCES=              \
	modest-wizard-dialog.h modest-wizard-dialog.c \
	modest-presets.h modest-presets.c \
	modest-easysetup-wizard.h modest-easysetup-wizard.c \
	modest-easysetup-country-combo-box.h modest-easysetup-country-combo-box.c \
	modest-easysetup-provider-combo-box.h modest-easysetup-provider-combo-box.c \
	modest-easysetup-servertype-combo-box.h modest-easysetup-servertype-combo-box.c

LDADD = \
	$(MODEST_HILDON_HELP_LIBS) \
	$(MODEST_HILDON_NOTIFY_LIBS)\
	$(MODEST_HILDON_WIDGETS_LIBS) \
	$(MODEST_GSTUFF_LIBS) \
	$(MODEST_PLATFORM_LIBS)

# mcc_mapping may be provided by the operator-wizard-settings package:
# the one here is a copy, as that package is not available in chinook
MMC_MAPPING_FILE=mcc_mapping
mmccmappingdir=$(datadir)/modest/provider-data
mmccmapping_DATA = $(MMC_MAPPING_FILE)

PROVIDER_DATA_FILE=maemo-provider-data.keyfile
providerdatadir=$(datadir)/modest/provider-data
providerdata_DATA = $(PROVIDER_DATA_FILE)

EXTRA_DIST=\
	$(MMC_MAPPING_FILE)    \
	$(PROVIDER_DATA_FILE)
