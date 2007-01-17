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

#include <config.h>
#include <widgets/modest-header-view.h>
#include <widgets/modest-folder-view.h>
#include <modest-tny-platform-factory.h>
#include <modest-widget-memory.h>
#include <modest-widget-memory-priv.h>
#include <modest-local-folder-info.h>
#include <modest-init.h>
#include <glib/gstdio.h>

typedef struct {
	ModestHeaderViewColumn col;
	guint                  width;
} FolderCols;


#if MODEST_PLATFORM_ID==1   /*gtk*/
static const FolderCols INBOX_COLUMNS[] = {
	{MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 20},
	{MODEST_HEADER_VIEW_COLUMN_ATTACH,  20},
	{MODEST_HEADER_VIEW_COLUMN_FROM,    50},
	{MODEST_HEADER_VIEW_COLUMN_SUBJECT, 50},
	{MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE, 50},
	{MODEST_HEADER_VIEW_COLUMN_SIZE, 30},
};

static const FolderCols OUTBOX_COLUMNS[] = {
	 {MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 20},
	 {MODEST_HEADER_VIEW_COLUMN_ATTACH,  20},
	 {MODEST_HEADER_VIEW_COLUMN_TO,    50},
	 {MODEST_HEADER_VIEW_COLUMN_SUBJECT, 50},
	 {MODEST_HEADER_VIEW_COLUMN_SENT_DATE, 50},
	 {MODEST_HEADER_VIEW_COLUMN_SIZE, 30},
};
#elif MODEST_PLATFORM==2  /*maemo*/
static const FolderCols INBOX_COLUMNS = {
	{MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 20},
	{MODEST_HEADER_VIEW_COLUMN_ATTACH,  20},
	{MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN,150},
};
static const FolderCols OUTBOX_COLUMNS = {
	 {MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 20},
	 {MODEST_HEADER_VIEW_COLUMN_ATTACH,  20},
	 {MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT,150},
};
#endif /*MODEST_PLATFORM*/

static const ModestLocalFolderType LOCAL_FOLDERS[] = {
	MODEST_LOCAL_FOLDER_TYPE_OUTBOX,
	MODEST_LOCAL_FOLDER_TYPE_DRAFTS,
	MODEST_LOCAL_FOLDER_TYPE_SENT,
	MODEST_LOCAL_FOLDER_TYPE_ARCHIVE	
};


static ModestTnyPlatformFactory*
get_platform_factory (void)
{
	TnyPlatformFactory *fact =
		modest_tny_platform_factory_get_instance ();

	if (!fact) {
		g_printerr ("modest: cannot get platform factory instance\n");
		return NULL;
	}

	return MODEST_TNY_PLATFORM_FACTORY(fact);
}


static ModestConf*
get_modest_conf (void)
{
	ModestTnyPlatformFactory *fact =
		get_platform_factory ();
	ModestConf *conf =
		modest_tny_platform_factory_get_conf_instance (fact);
	if (!conf) {
		g_printerr ("modest: cannot get modest conf instance\n");
		return NULL;
	}
	return conf;
}


/* NOTE: the exact details of this format are important, as they
 * are also used in modest-widget-memory. FIXME: make a shared function
 * for this with widget-memory
 */
static gboolean
save_header_settings (ModestConf *conf, TnyFolderType type, const FolderCols* cols,
		     guint col_num, gboolean overwrite)
{
	int i;
	gchar *key;
	GString *str;

	g_return_val_if_fail (cols, FALSE);

	key = _modest_widget_memory_get_keyname_with_type ("header-view",
							   type,
							   MODEST_WIDGET_MEMORY_PARAM_COLUMN_WIDTH);
	/* if we're not in overwrite mode, only write stuff it
	 * there was nothing before */
	if (!overwrite &&  modest_conf_key_exists(conf, key, NULL)) {
		g_free (key);
		return TRUE;
	}

	/* the format is necessarily the same as the one in modest-widget-memory */
	str = g_string_new (NULL);
	for (i = 0; i != col_num; ++i) 
		g_string_append_printf (str, "%d:%d ",
					cols[i].col, cols[i].width); 

	modest_conf_set_string (conf, key, str->str, NULL);
	g_free (key);
	g_string_free (str, TRUE);
	
	return TRUE;
}

/* we set the the defaults here for all folder types */
gboolean
modest_init_header_columns (gboolean overwrite)
{
	ModestConf *conf;
	int folder_type;

	conf = get_modest_conf ();
	if (!conf) {
		g_printerr ("modest: cannot get modest conf\n");
		return FALSE;
	}
	
	for (folder_type = TNY_FOLDER_TYPE_UNKNOWN;
	     folder_type <= TNY_FOLDER_TYPE_CALENDAR; ++folder_type) {		
		
		switch (folder_type) {
		case TNY_FOLDER_TYPE_OUTBOX:
		case TNY_FOLDER_TYPE_SENT:
		case TNY_FOLDER_TYPE_DRAFTS:
			save_header_settings (conf, folder_type, OUTBOX_COLUMNS,
					      G_N_ELEMENTS(OUTBOX_COLUMNS),
					      overwrite);
			break;
		default:
			save_header_settings (conf, folder_type, INBOX_COLUMNS,
					      G_N_ELEMENTS(INBOX_COLUMNS),
					      overwrite);
		};
	}
	return TRUE;
}

gboolean
modest_init_local_folders  (void)
{
	int i;
	gchar *maildir_path;
	static const gchar* maildirs[] = {
		"cur", "new", "tmp"
	};
	
	maildir_path = modest_local_folder_info_get_maildir_path ();

	for (i = 0; i != G_N_ELEMENTS(LOCAL_FOLDERS); ++i) {
		int j;
		for (j = 0; j != G_N_ELEMENTS(maildirs); ++j) {
			gchar *dir;
			dir = g_build_filename (maildir_path,
						modest_local_folder_info_get_type_name(LOCAL_FOLDERS[i]),
						maildirs[j],
						NULL);
			if (g_mkdir_with_parents (dir, 0755) < 0) {
				g_printerr ("modest: failed to create %s\n", dir);
				g_free (dir);
				g_free (maildir_path);
				return FALSE;
			}
			g_free(dir);
		}
	}
	
	g_free (maildir_path);
	return TRUE;
}
