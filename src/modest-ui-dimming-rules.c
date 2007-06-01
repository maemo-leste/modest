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
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <string.h>
#include "modest-ui-dimming-rules.h"
#include "modest-tny-folder.h"
#include <modest-runtime.h>


static gboolean _folder_is_any_of_type (TnyFolder *folder, TnyFolderType types[], guint ntypes);
static gboolean _invalid_msg_selected (ModestMainWindow *win, gboolean unique);
static gboolean _already_opened_msg (ModestWindow *win);
static gboolean _selected_msg_marked_as (ModestWindow *win, TnyHeaderFlags mask, gboolean opposite);
static gboolean _selected_folder_not_writeable (ModestMainWindow *win);
static gboolean _selected_folder_is_any_of_type (ModestMainWindow *win, TnyFolderType types[], guint ntypes);
static gboolean _selected_folder_is_root (ModestMainWindow *win);
static gboolean _msg_download_in_progress (ModestMsgViewWindow *win);


gboolean 
modest_ui_dimming_rules_on_new_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _msg_download_in_progress (MODEST_MSG_VIEW_WINDOW(win));

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_new_folder (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);


	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!parent_folder)
		return TRUE;
	
	/* If it's the local account do not dim */
	if (modest_tny_folder_store_is_virtual_local_folders (parent_folder)) {
		return FALSE;
	} else if (TNY_IS_ACCOUNT (parent_folder)) {
		/* If it's the MMC root folder then dim it */
		if (!strcmp (tny_account_get_id (TNY_ACCOUNT (parent_folder)), MODEST_MMC_ACCOUNT_ID)) {
			dimmed = TRUE;
		} else {
			const gchar *proto_str = tny_account_get_proto (TNY_ACCOUNT (parent_folder));
			/* If it's POP then dim */
			dimmed = (modest_protocol_info_get_transport_store_protocol (proto_str) == 
				  MODEST_PROTOCOL_STORE_POP) ? TRUE : FALSE;
		}
	} else {
		/* TODO: the specs say that only one level of subfolder is allowed, is this true ? */

		/* Apply folder rules */	
		dimmed = _selected_folder_not_writeable (MODEST_MAIN_WINDOW(win));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_rename_folder (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_not_writeable (MODEST_MAIN_WINDOW(win));
	if (!dimmed)
		dimmed = _selected_folder_is_root (MODEST_MAIN_WINDOW(win));

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_open_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_reply_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	TnyFolderType types[3];

	/* main window dimming rules */
	if (MODEST_IS_MAIN_WINDOW(win)) {
		
		types[0] = TNY_FOLDER_TYPE_DRAFTS; 
		types[1] = TNY_FOLDER_TYPE_OUTBOX;
		types[2] = TNY_FOLDER_TYPE_ROOT;
		
		/* Check dimmed rule */	
		if (!dimmed)
			dimmed = _selected_folder_is_any_of_type (MODEST_MAIN_WINDOW(win), types, 3);
		
		if (!dimmed)
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE);

	/* msg view window dimming rules */
	} else if (MODEST_IS_MSG_VIEW_WINDOW(win)) {
		
		/* Check dimmed rule */	
		if (!dimmed)
			dimmed = _msg_download_in_progress (MODEST_MSG_VIEW_WINDOW(win));
	}
	
	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_contents_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_always_dimmed (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = TRUE;

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_delete_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE);
	if (!dimmed)
		dimmed = _already_opened_msg (win);
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_details (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	
	/* main window dimming rules */
	if (MODEST_IS_MAIN_WINDOW(win)) {
		GtkWidget *header_view;

		/* Check dimmed rule */
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
								   MODEST_WIDGET_TYPE_HEADER_VIEW);

		/* If the header view does not have the focus then do
		   not apply msg dimming rules because the action will
		   show the folder details that have no dimming
		   rule */
		if (gtk_widget_is_focus (header_view)) {
			if (!dimmed)
				dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE);
		}

	/* msg view window dimming rules */
	} else {

		/* Check dimmed rule */	
		if (!dimmed)
			dimmed = _msg_download_in_progress (MODEST_MSG_VIEW_WINDOW(win));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_mark_as_read_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	TnyHeaderFlags flags;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	
	flags = TNY_HEADER_FLAG_SEEN; 

	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE);
	if (!dimmed) 
		dimmed = _selected_msg_marked_as (win, flags, FALSE);
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_mark_as_unread_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	TnyHeaderFlags flags;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	
	flags = TNY_HEADER_FLAG_SEEN; 

	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE);
	if (!dimmed) 
		dimmed = _selected_msg_marked_as (win, flags, TRUE);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_move_to (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	if (MODEST_IS_MAIN_WINDOW (win)) 
		dimmed = modest_ui_dimming_rules_on_main_window_move_to (win, user_data);
	else 
		dimmed = modest_ui_dimming_rules_on_view_window_move_to (win, user_data);

	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_main_window_move_to (ModestWindow *win, gpointer user_data)
{
	GtkWidget *folder_view = NULL;
	GtkWidget *header_view = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), TRUE);
	
	/* Get the folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);

	/* Get header view */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Check common diming rules */

	/* Check diming rules for folder transfer  */
	if (gtk_widget_is_focus (folder_view)) {
		if (!dimmed) 
			dimmed = _selected_folder_not_writeable(MODEST_MAIN_WINDOW(win));
	}
	/* Check diming rules for msg transfer  */
	else if (gtk_widget_is_focus (header_view)) {
		if (!dimmed)
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE);
		
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_view_window_move_to (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_paste_msgs (ModestWindow *win, gpointer user_data)
{
	TnyFolderType types[3];
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_is_any_of_type (MODEST_MAIN_WINDOW(win), types, 3);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_delete_msgs (ModestWindow *win, gpointer user_data)
{
	TnyFolderType types[5];
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	types[3] = TNY_FOLDER_TYPE_INBOX;
	types[4] = TNY_FOLDER_TYPE_ROOT;
	
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_is_any_of_type (MODEST_MAIN_WINDOW(win), types, 5);

	return dimmed;
}


/* *********************** static utility functions ******************** */

static gboolean
_selected_folder_not_writeable (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	ModestTnyFolderRules rules;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(parent_folder || TNY_IS_FOLDER(parent_folder)))
		return TRUE;
	
	/* Check dimmed rule */	
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent_folder));
	result = rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;

	/* free */
	g_object_unref (parent_folder);

	return result;
}

static gboolean
_selected_folder_is_root (ModestMainWindow *win)
{
	TnyFolderType types[2];
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_ROOT; 
	types[1] = TNY_FOLDER_TYPE_INBOX; 

	/* Check folder type */
	result = _selected_folder_is_any_of_type (win, types, 2);

	return result;
}

static gboolean
_selected_folder_is_any_of_type (ModestMainWindow *win,
				 TnyFolderType types[], 
				 guint ntypes)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *folder = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(folder || TNY_IS_FOLDER(folder)))
		return TRUE;
	
	/* Check folder type */
	result = _folder_is_any_of_type (TNY_FOLDER(folder), types, ntypes);

	/* free */
	g_object_unref (folder);

	return result;	
}

static gboolean
_folder_is_any_of_type (TnyFolder *folder,
			TnyFolderType types[], 
			guint ntypes)
{
	TnyFolderType folder_type;
	gboolean result = FALSE;
	guint i;

	g_return_val_if_fail (TNY_IS_FOLDER(folder), FALSE);

	/* Get folder type */
	if (modest_tny_folder_is_local_folder (folder))
		folder_type = modest_tny_folder_get_local_folder_type (folder);		
	else 
		folder_type = modest_tny_folder_guess_folder_type (folder);		
	
	/* Check foler type */
	for (i=0; i < ntypes; i++) {
		result = result || folder_type == types[i];
	}

	return result;
}



static gboolean
_invalid_msg_selected (ModestMainWindow *win,
		       gboolean unique) 
{
	GtkWidget *header_view = NULL;		
	GtkWidget *folder_view = NULL;
	TnyList *selected_headers = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Get header view to check selected messages */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);
	
	/* Get folder view to check focus */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);

	/* Get selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));

	/* Check dimmed rule (TODO: check focus on widgets */	
	result = ((selected_headers == NULL) || 
		  (GTK_WIDGET_HAS_FOCUS (folder_view)));
	if (!result)
		result = tny_list_get_length (selected_headers) > 1;
	
	/* free */
	if (selected_headers != NULL) 
		g_object_unref (selected_headers);

	return result;
}

static gboolean
_already_opened_msg (ModestWindow *win)
{
	ModestWindow *window = NULL;
	ModestWindowMgr *mgr = NULL;
	GtkWidget *header_view = NULL;		
	TnyList *selected_headers = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	gboolean result = TRUE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Get header view to check selected messages */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Get selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
	if (selected_headers == NULL) 
		return FALSE;

	/* Check dimmed rule (TODO: check focus on widgets */	
	mgr = modest_runtime_get_window_mgr ();
	iter = tny_list_create_iterator (selected_headers);
	while (!tny_iterator_is_done (iter) && result) {
		header = TNY_HEADER (tny_iterator_get_current (iter));
		window = modest_window_mgr_find_window_by_header (mgr, header);
		result = result && (window != NULL);
			
		g_object_unref (header);
		tny_iterator_next (iter);
	}
	
	/* free */
	if (selected_headers != NULL) 
		g_object_unref (selected_headers);
	if (iter != NULL)
		g_object_unref (iter);
		
	return result;
}

static gboolean
_selected_msg_marked_as (ModestWindow *win, 
			 TnyHeaderFlags mask, 
			 gboolean opposite)
{
	GtkWidget *header_view = NULL;
	TnyList *selected_headers = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	TnyHeaderFlags flags;
	gboolean result = FALSE;

	/* Get header view to check selected messages */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Get selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
	if (selected_headers == NULL) 
		return TRUE;
	
	/* Call the function for each header */
	iter = tny_list_create_iterator (selected_headers);
	while (!tny_iterator_is_done (iter) && !result) {
		header = TNY_HEADER (tny_iterator_get_current (iter));

		flags = tny_header_get_flags (header);
		if (opposite)
			result = (flags & mask) == 0; 
		else
			result = (flags & mask) != 0; 

		g_object_unref (header);
		tny_iterator_next (iter);
	}

	/* free */
	if (selected_headers != NULL) 
		g_object_unref (selected_headers);
	if (iter != NULL)
		g_object_unref (iter);

	return result;
}

static gboolean
_msg_download_in_progress (ModestMsgViewWindow *win)
{
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win), FALSE);

	return result;
}
