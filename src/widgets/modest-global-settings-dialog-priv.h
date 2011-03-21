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

#ifndef __MODEST_GLOBAL_SETTINGS_DIALOG_PRIV_H__
#define __MODEST_GLOBAL_SETTINGS_DIALOG_PRIV_H__

#include <gtk/gtk.h>
#include "modest-pair.h"

G_BEGIN_DECLS

typedef struct _ModestGlobalSettingsState {
	gboolean auto_update;
	gint     connect_via;
	gint     update_interval;
	gchar   *default_account;
	gboolean play_sound;
	gboolean prefer_formatted_text;
	gboolean notifications;
	gboolean add_to_contacts;
	gboolean tree_view;
} ModestGlobalSettingsState;

typedef struct _ModestGlobalSettingsDialogPrivate ModestGlobalSettingsDialogPrivate;
struct _ModestGlobalSettingsDialogPrivate {
	GtkWidget *notebook;
	GtkWidget *updating_page;
	GtkWidget *composing_page;

	GtkWidget *auto_update;
	
	ModestPairList *connect_via_list;
	GtkWidget *connect_via;
	
	ModestPairList *accounts_list;
	GtkWidget *default_account_selector;

	ModestPairList *update_interval_list;
	GtkWidget *update_interval;
	
	GtkWidget *play_sound;
	
	ModestPairList *msg_format_list;
	GtkWidget *msg_format;

	GtkWidget *notifications;
	GtkWidget *add_to_contacts;

	GtkWidget *tree_view;

	ModestGlobalSettingsState initial_state;
};

#define MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                           MODEST_TYPE_GLOBAL_SETTINGS_DIALOG, \
                                                           ModestGlobalSettingsDialogPrivate))

ModestPairList*   _modest_global_settings_dialog_get_connected_via   (void);
ModestPairList*   _modest_global_settings_dialog_get_update_interval (void);
ModestPairList*   _modest_global_settings_dialog_get_msg_formats     (void);

G_END_DECLS

#endif /* __MODEST_GLOBAL_SETTINGS_DIALOG_PRIV_H__ */

