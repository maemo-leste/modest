/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* modest-ui-glade.h */

#ifndef __MODEST_UI_GLADE_H__
#define __MODEST_UI_GLADE_H__

#include "../modest-ui.h"
#include "../modest-account-mgr.h"
#include "../modest-identity-mgr.h"
#include "../modest-window-mgr.h"
#include "../modest-tny-account-store.h"

#define MODEST_GLADE          PREFIX "/share/modest/glade/modest.glade"
#define MODEST_GLADE_MAIN_WIN "main"
#define MODEST_GLADE_EDIT_WIN "new_mail"

typedef struct _ModestUIPrivate ModestUIPrivate;
struct _ModestUIPrivate {

	ModestConf           *modest_conf;
	ModestAccountMgr     *modest_acc_mgr;
	ModestIdentityMgr    *modest_id_mgr;
	ModestWindowMgr      *modest_window_mgr;
	TnyAccountStoreIface *account_store;
	GtkWidget            *folder_view;
	GtkWidget            *header_view;
	GtkWidget            *message_view;

	GtkWindow            *main_window;
	GladeXML             *glade_xml;

	TnyMsgFolderIface    *current_folder;
};

#define MODEST_UI_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MODEST_TYPE_UI, \
                                       ModestUIPrivate))

#endif /* __MODEST_UI_GLADE_H__ */
