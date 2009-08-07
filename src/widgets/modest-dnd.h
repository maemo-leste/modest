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

#ifndef __MODEST_DND_H__
#define __MODEST_DND_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

extern GdkAtom tree_path_as_string_list_atom;

#define GTK_TREE_PATH_AS_STRING_LIST "text/tree-path-as-string-list"

enum {
       MODEST_FOLDER_ROW,
       MODEST_HEADER_ROW
};

/**
 * modest_dnd_selection_data_set_paths:
 * @selection_data: 
 * @selected_rows: 
 * 
 * This function sets a list of gtk_tree_path's represented as strings
 * as the data of a #GtkSelectionData object that will be used during
 * drag and drop
 **/
void     modest_dnd_selection_data_set_paths (GtkSelectionData *selection_data,
					      GList            *selected_rows);


/**
 * modest_dnd_selection_data_get_paths:
 * @selection_data: 
 * 
 * This function gets a list of gtk_tree_path's represented as strings
 * from a #GtkSelectionData object used during drag and drop
 * 
 * Returns: the list of gtk_tree_paths as strings or NULL
 **/
gchar**  modest_dnd_selection_data_get_paths (GtkSelectionData *selection_data);

#endif /* __MODEST_DND_H__ */
