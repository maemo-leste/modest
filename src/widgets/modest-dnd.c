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

#include "modest-dnd.h"
#include <gtk/gtk.h>
#include <string.h>

GdkAtom tree_path_as_string_list_atom;

static void
init_atom (void)
{
	if (!tree_path_as_string_list_atom)
		tree_path_as_string_list_atom = 
			gdk_atom_intern_static_string (GTK_TREE_PATH_AS_STRING_LIST);
}


void 
modest_dnd_selection_data_set_paths (GtkSelectionData *selection_data,
				     GList            *selected_rows)
{
	init_atom ();

	if (selection_data->target == tree_path_as_string_list_atom) {
		GString *list;
		gchar *result;
		GList *row;
      
		row = selected_rows;
		list = g_string_new (NULL);
		
		while (row != NULL) {
			g_string_append (list, gtk_tree_path_to_string (row->data));
			row = g_list_next (row);
			if (row != NULL)
				g_string_append (list, "\n");
		}

		result = g_strdup (list->str);
		g_string_free (list, TRUE);
		
		if (result) {
			gtk_selection_data_set (selection_data,
						tree_path_as_string_list_atom,
						8, (guchar *)result, 
						strlen (result));
			
			g_free (result);
		}
	}
}

gchar**  
modest_dnd_selection_data_get_paths (GtkSelectionData *selection_data)
{
	gchar **result = NULL;
	
	init_atom ();
  
	if (selection_data->length >= 0 &&
	    selection_data->type == tree_path_as_string_list_atom) {
		
		result = g_strsplit ((const gchar*)selection_data->data, "\n", 0);		
	}	
	return result;
}
