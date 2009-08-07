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

#ifndef __MODEST_HEADER_VIEW_PRIV_H__
#define __MODEST_HEADER_VIEW_PRIV_H__

#include <gtk/gtk.h>
#include "modest-header-view.h"

G_BEGIN_DECLS

#define ACTIVE_COLOR "active-color"
#define BOLD_IS_ACTIVE_COLOR "bold-is-active-color"

/* PROTECTED method. It's useful when we want to force a given
   selection to reload a msg. For example if we have selected a header
   in offline mode, when Modest become online, we want to reload the
   message automatically without an user click over the header */
void  _modest_header_view_change_selection (GtkTreeSelection *selection, gpointer user_data);

/* private: renderers */
void _modest_header_view_msgtype_cell_data (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
					    GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data);
void _modest_header_view_attach_cell_data (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
					   GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data);
void _modest_header_view_header_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
					    GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data);
void _modest_header_view_date_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
					  GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data);
void _modest_header_view_size_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
					  GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data);
void _modest_header_view_status_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
					    GtkTreeModel *tree_model,  GtkTreeIter *iter,
					    gpointer user_data);
void _modest_header_view_sender_receiver_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
						     GtkTreeModel *tree_model,  GtkTreeIter *iter,  gboolean is_sender);
void _modest_header_view_compact_header_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
						    GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data);

const gchar *_modest_header_view_get_display_date (ModestHeaderView *self, time_t date);

typedef enum _ModestHeaderViewCompactHeaderMode {
	MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_IN = 0,
	MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_OUT = 1,
	MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_OUTBOX = 2
} ModestHeaderViewCompactHeaderMode;

G_END_DECLS

#endif /* __MODEST_HEADER_VIEW_PRIV_H__ */
