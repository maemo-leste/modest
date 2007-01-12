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

#ifndef __MODEST_MAIN_WINDOW_UI_PRIV_H__
#define __MODEST_MAIN_WINDOW_UI_PRIV_H__

#include <glib/gi18n.h>
#include "modest-icon-names.h"
#include "modest-ui-actions.h"

G_BEGIN_DECLS

static const GtkActionEntry modest_edit_msg_action_entries [] = {

	/* Toplevel menus */
	{ "View", NULL, N_("_View") },
	{ "Insert", NULL, N_("_Insert") },
	{ "Format", NULL, N_("For_mat") },

	/* ACTIONS */
	{ "ActionsSend", MODEST_STOCK_MAIL_SEND, N_("Send"),  NULL, N_("Send a message"),  G_CALLBACK (_modest_ui_actions_on_send) },
};

static const GtkToggleActionEntry modest_edit_msg_toggle_action_entries [] = {

	/* VIEW */
	{ "ViewToField",   NULL,    N_("To: field"),  NULL, N_("Shows the To: field"),  NULL, TRUE  },
	{ "ViewCcField",   NULL,    N_("Cc: field"),  NULL, N_("Shows the Cc: field"),  NULL, TRUE  },
	{ "ViewBccField",  NULL,    N_("Bcc: filed"), NULL, N_("Shows the Bcc: field"), NULL, FALSE },
};

G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */
