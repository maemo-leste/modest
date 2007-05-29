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

#include "modest-ui-dimming-rules.h"
#include "modest-tny-folder.h"

static gboolean _folder_is_any_of_type (TnyFolder *folder, TnyFolderType types[], guint ntypes);
static gboolean _invalid_msg_selected (ModestMainWindow *win, gboolean unique);

gboolean 
modest_ui_dimming_rules_on_new_folder (ModestWindow *win, gpointer user_data)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	ModestTnyFolderRules rules;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!parent_folder)
		return TRUE;
	
	/* Check dimmed rule */	
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent_folder));
	dimmed = rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;

	/* free */
	g_object_unref (parent_folder);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_open_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_reply_msg (ModestWindow *win, gpointer user_data)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *current_folder = NULL;
	gboolean local_folder = FALSE;
	gboolean dimmed = FALSE;
	TnyFolderType types[2];

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;

	/* Get selected folder as parent of new folder to create */
	current_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!current_folder)
		return TRUE;
	
	if (TNY_IS_FOLDER(current_folder))
		local_folder = modest_tny_folder_is_local_folder (TNY_FOLDER(current_folder));
	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;

	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = ((local_folder) && 
			  (_folder_is_any_of_type (TNY_FOLDER(current_folder), types, 2)));
	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE);

	/* free */
	g_object_unref (current_folder);

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

/* *********************** static utility functions ******************** */

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
