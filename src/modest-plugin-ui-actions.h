/* Copyright (c) 2008, Nokia Corporation
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

#ifndef __MODEST_PLUGIN_UI_ACTIONS_H__
#define __MODEST_PLUGIN_UI_ACTIONS_H__

/* This should contain simple facades for internal ui actions in modest
 * that should be available in plugins.
 */

#include <gtk/gtkwindow.h>
#include <glib.h>

G_BEGIN_DECLS

/**
 * modest_ui_actions_on_delete_account:
 * @parent_window: the parent #GtkWindow of the dialog that will be shown
 * @account_name: the modest name for the account to be deleted
 * @account_display_name: the display name that will be used in the dialog
 *
 * This function will perform the actions required to delete an
 * account. This function assumes that the account to be deleted is
 * already disconnected, so the caller must verify that this is the
 * case. Otherwise weird behaviours could happen
 *
 * Returns: TRUE if the account was successfully deleted, FALSE otherwise
 **/
gboolean modest_ui_actions_on_delete_account (GtkWindow *parent_window,
					      const gchar *account_name,
					      const gchar *account_display_name);
G_END_DECLS
#endif /* __MODEST_PLUGIN_UI_ACTIONS_H__ */