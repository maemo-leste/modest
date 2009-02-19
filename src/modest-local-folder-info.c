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

#include <glib/gi18n.h>
#include <string.h> /* strcmp */
#include <modest-local-folder-info.h>
#include <modest-defs.h>
#include <stdio.h>

typedef struct {
	TnyFolderType   type;
	const gchar     *name;
	const gchar     *display_name;
} ModestLocalFolder;

const ModestLocalFolder ModestLocalFolderMap[] = {
	{ TNY_FOLDER_TYPE_UNKNOWN,  "<unknown>",  N_("<Unknown>")},
	{ TNY_FOLDER_TYPE_NORMAL,   "<normal>",   N_("<Normal>")},
	{ TNY_FOLDER_TYPE_OUTBOX,   "outbox",     N_("mcen_me_folder_outbox")},
	{ TNY_FOLDER_TYPE_TRASH,    "trash",      N_("Trash")},
	{ TNY_FOLDER_TYPE_JUNK,     "junk",       N_("Junk")},
	{ TNY_FOLDER_TYPE_SENT,     "sent",       N_("mcen_me_folder_sent")},
	{ TNY_FOLDER_TYPE_ROOT,     "<root>",     N_("<root>")},
	{ TNY_FOLDER_TYPE_NOTES,    "notes",      N_("Notes")},
	{ TNY_FOLDER_TYPE_DRAFTS,   "drafts",     N_("mcen_me_folder_drafts")},
	{ TNY_FOLDER_TYPE_ARCHIVE,  "archive",    N_("mcen_me_folder_archive")}
};


TnyFolderType
modest_local_folder_info_get_type (const gchar *name)
{
	int i;
	g_return_val_if_fail (name, TNY_FOLDER_TYPE_UNKNOWN);

	for (i = 0; i != G_N_ELEMENTS(ModestLocalFolderMap); ++i) {
		if (strcmp (ModestLocalFolderMap[i].name, name) == 0)
			return ModestLocalFolderMap[i].type;
	}
	return TNY_FOLDER_TYPE_UNKNOWN;
}


const gchar*
modest_local_folder_info_get_type_name (TnyFolderType type)
{
	int i = 0;
	g_return_val_if_fail (type >= TNY_FOLDER_TYPE_UNKNOWN &&
			      type <  TNY_FOLDER_TYPE_NUM, NULL);
	
	for (i = 0; i != G_N_ELEMENTS(ModestLocalFolderMap); ++i) {
		if (ModestLocalFolderMap[i].type == type)
			return ModestLocalFolderMap[i].name;
	}
	return NULL;	
}

const gchar*
modest_local_folder_info_get_type_display_name (TnyFolderType type)
{
	int i = 0;
	g_return_val_if_fail (type >= TNY_FOLDER_TYPE_UNKNOWN &&
			      type <  TNY_FOLDER_TYPE_NUM, NULL);
	
	/* Note that we call _() to get the localized name.
	 * This works because we used N_() on the static string when we initialized the array,
	 * to mark the string for translation.
	 */
	for (i = 0; i != G_N_ELEMENTS(ModestLocalFolderMap); ++i) {
		if (ModestLocalFolderMap[i].type == type)
			return _(ModestLocalFolderMap[i].display_name);
	}
	return NULL;	
}


gchar *
modest_local_folder_info_get_maildir_path (const gchar* location_filepath)
{
	return g_build_filename (location_filepath ? location_filepath : g_get_home_dir(),
				 MODEST_DIR,
				 MODEST_LOCAL_FOLDERS_MAILDIR, 
				 NULL);
}

gchar*
modest_per_account_local_outbox_folder_info_get_maildir_path (const gchar* account_name)
{
	/* This directory should contain an "outbox" child directory: */
	return g_build_filename (g_get_home_dir(),
				 MODEST_DIR,
				 MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDERS_MAILDIR, 
				 account_name,
				 NULL);
}

gchar*
modest_per_account_local_outbox_folder_info_get_maildir_path_to_outbox_folder (const gchar* account_name)
{
	gchar *path_to_account_folder = 
		modest_per_account_local_outbox_folder_info_get_maildir_path(account_name);
	if (!path_to_account_folder)
		return NULL;

	gchar *path = g_build_filename (path_to_account_folder, "outbox", NULL);

	g_free (path_to_account_folder);

	return path;
}



