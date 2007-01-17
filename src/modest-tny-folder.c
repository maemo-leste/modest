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

#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>
#include <modest-tny-folder.h>


typedef struct {
	ModestLocalFolderType   type;
	const gchar             *name;
	const gchar             *display_name;
} ModestLocalFolder;

const ModestLocalFolder ModestLocalFolderMap[] = {
	{ MODEST_LOCAL_FOLDER_TYPE_JUNK,    "junk",    N_("junk")},
	{ MODEST_LOCAL_FOLDER_TYPE_TRASH,   "trash",   N_("trash")},
	{ MODEST_LOCAL_FOLDER_TYPE_DRAFTS,  "drafts",  N_("drafts")},
	{ MODEST_LOCAL_FOLDER_TYPE_SENT,    "sent",    N_("sent")},
	{ MODEST_LOCAL_FOLDER_TYPE_OUTBOX,  "outbox",  N_("outbox")},
	{ MODEST_LOCAL_FOLDER_TYPE_ARCHIVE, "archive", N_("archive")}
};


ModestLocalFolderType
modest_tny_folder_get_local_folder_type (const gchar *name)
{
	int i;

	g_return_val_if_fail (name, MODEST_LOCAL_FOLDER_TYPE_UNKNOWN);

	for (i = 0; i != G_N_ELEMENTS(ModestLocalFolderMap); ++i) {
		if (strcmp (ModestLocalFolderMap[i].name, name) == 0)
			return ModestLocalFolderMap[i].type;
	}
	return MODEST_LOCAL_FOLDER_TYPE_UNKNOWN;
}

const gchar*
modest_tny_folder_get_local_folder_type_name (ModestLocalFolderType type)
{
	int i = 0;
	g_return_val_if_fail (type > MODEST_LOCAL_FOLDER_TYPE_UNKNOWN &&
			      type < MODEST_LOCAL_FOLDER_TYPE_NUM, NULL);
	
	for (i = 0; i != G_N_ELEMENTS(ModestLocalFolderMap); ++i) {
		if (ModestLocalFolderMap[i].type == type)
			return ModestLocalFolderMap[i].name;
	}
	return NULL;	
}


const gchar*
modest_tny_folder_get_local_folder_type_display_name (ModestLocalFolderType type)
{
	int i = 0;
	g_return_val_if_fail (type > MODEST_LOCAL_FOLDER_TYPE_UNKNOWN &&
			      type < MODEST_LOCAL_FOLDER_TYPE_NUM, NULL);
	
	for (i = 0; i != G_N_ELEMENTS(ModestLocalFolderMap); ++i) {
		if (ModestLocalFolderMap[i].type == type)
			return ModestLocalFolderMap[i].display_name;
	}
	return NULL;	
}




TnyFolderType
modest_tny_folder_guess_folder_type_from_name (const gchar* name)
{
	gint  type;
	gchar *folder;

	g_return_val_if_fail (name, TNY_FOLDER_TYPE_UNKNOWN);
	
	type = TNY_FOLDER_TYPE_UNKNOWN;
	folder = g_utf8_strdown (name, strlen(name));

	if (strcmp (folder, "inbox") == 0 ||
	    strcmp (folder, _("inbox")) == 0)
		type = TNY_FOLDER_TYPE_INBOX;
	else if (strcmp (folder, "outbox") == 0 ||
		 strcmp (folder, _("outbox")) == 0)
		type = TNY_FOLDER_TYPE_OUTBOX;
	else if (g_str_has_prefix(folder, "junk") ||
		 g_str_has_prefix(folder, _("junk")))
		type = TNY_FOLDER_TYPE_JUNK;
	else if (g_str_has_prefix(folder, "trash") ||
		 g_str_has_prefix(folder, _("trash")))
		type = TNY_FOLDER_TYPE_TRASH;
	else if (g_str_has_prefix(folder, "sent") ||
		 g_str_has_prefix(folder, _("sent")))
		type = TNY_FOLDER_TYPE_SENT;
	else if (g_str_has_prefix(folder, "draft") ||
		 g_str_has_prefix(folder, _("draft")))
		type = TNY_FOLDER_TYPE_DRAFTS;
	else if (g_str_has_prefix(folder, "notes") ||
		 g_str_has_prefix(folder, _("notes")))
		type = TNY_FOLDER_TYPE_NOTES;
	else if (g_str_has_prefix(folder, "contacts") ||
		 g_str_has_prefix(folder, _("contacts")))
		type = TNY_FOLDER_TYPE_CONTACTS;
	else if (g_str_has_prefix(folder, "calendar") ||
		 g_str_has_prefix(folder, _("calendar")))
		type = TNY_FOLDER_TYPE_CALENDAR;
	
	g_free (folder);
	return type;
}



TnyFolderType
modest_tny_folder_guess_folder_type (const TnyFolder *folder)
{
	TnyFolderType type;

	g_return_val_if_fail (folder, TNY_FOLDER_TYPE_UNKNOWN);

	type = tny_folder_get_folder_type (TNY_FOLDER (folder));
	
	if (type == TNY_FOLDER_TYPE_UNKNOWN) {
		const gchar *folder_name;

		folder_name = tny_folder_get_name (TNY_FOLDER (folder));
		type =	modest_tny_folder_guess_folder_type_from_name (folder_name);
	}

	return type;
}


ModestTnyFolderRules
modest_tny_folder_get_folder_rules   (const TnyFolder *folder)
{
	g_return_val_if_fail (folder, 0);
	
	/* FIXME -- implement this */
	return 0;
}
