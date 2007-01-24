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
#include <modest-defs.h>
#include <widgets/modest-header-view.h>
#include <widgets/modest-folder-view.h>
#include <modest-tny-platform-factory.h>
#include <modest-widget-memory.h>
#include <modest-widget-memory-priv.h>
#include <modest-local-folder-info.h>
#include <modest-init.h>
#include <glib/gstdio.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>

typedef struct {
	ModestHeaderViewColumn col;
	guint                  width;
} FolderCols;

static const FolderCols INBOX_COLUMNS_DETAILS[] = {
	{MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 40},
	{MODEST_HEADER_VIEW_COLUMN_ATTACH,  40},
	{MODEST_HEADER_VIEW_COLUMN_FROM,    80},
	{MODEST_HEADER_VIEW_COLUMN_SUBJECT, 80},
	{MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE, 60},
	{MODEST_HEADER_VIEW_COLUMN_SIZE, 50}
};
static const FolderCols INBOX_COLUMNS_TWOLINES[] = {
	{MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 40},
	{MODEST_HEADER_VIEW_COLUMN_ATTACH,  40},
	{MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN, 200}
};

static const FolderCols OUTBOX_COLUMNS_DETAILS[] = {
	 {MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 40},
	 {MODEST_HEADER_VIEW_COLUMN_ATTACH,  40},
	 {MODEST_HEADER_VIEW_COLUMN_TO,    80},
	 {MODEST_HEADER_VIEW_COLUMN_SUBJECT, 80},
	 {MODEST_HEADER_VIEW_COLUMN_SENT_DATE, 80},
	 {MODEST_HEADER_VIEW_COLUMN_SIZE, 50}
};
static const FolderCols OUTBOX_COLUMNS_TWOLINES[] = {
	 {MODEST_HEADER_VIEW_COLUMN_MSGTYPE, 40},
	 {MODEST_HEADER_VIEW_COLUMN_ATTACH,  40},
	 {MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT,200},
};
	 
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


static ModestAccountMgr*
get_account_mgr (void)
{
	ModestTnyPlatformFactory *fact =
		get_platform_factory ();
	ModestAccountMgr *acc_mgr =
		modest_tny_platform_factory_get_account_mgr_instance (fact);
	if (!acc_mgr) {
		g_printerr ("modest: cannot get modest account mgr instance\n");
		return NULL;
	}
	return acc_mgr;
}


/* NOTE: the exact details of this format are important, as they
 * are also used in modest-widget-memory. FIXME: make a shared function
 * for this with widget-memory
 */
static gboolean
save_header_settings (ModestConf *conf, TnyFolderType type,
		      ModestHeaderViewStyle style,  const FolderCols* cols,
		      guint col_num, gboolean overwrite)
{
	int i;
	gchar *key;
	GString *str;

	g_return_val_if_fail (cols, FALSE);

	key = _modest_widget_memory_get_keyname_with_double_type ("header-view",
								  type, style,
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
		save_header_settings (conf, folder_type,
				      MODEST_HEADER_VIEW_STYLE_DETAILS,
				      OUTBOX_COLUMNS_DETAILS,
				      G_N_ELEMENTS(OUTBOX_COLUMNS_DETAILS),
				      overwrite);
		save_header_settings (conf, folder_type,
				      MODEST_HEADER_VIEW_STYLE_TWOLINES,
				      OUTBOX_COLUMNS_TWOLINES,
				      G_N_ELEMENTS(OUTBOX_COLUMNS_TWOLINES),
				      overwrite);
		break;

		default:
		save_header_settings (conf, folder_type,
				      MODEST_HEADER_VIEW_STYLE_DETAILS,
				      INBOX_COLUMNS_DETAILS,
				      G_N_ELEMENTS(INBOX_COLUMNS_DETAILS),
				      overwrite);
		save_header_settings (conf, folder_type,
				      MODEST_HEADER_VIEW_STYLE_TWOLINES,
				      INBOX_COLUMNS_TWOLINES,
				      G_N_ELEMENTS(INBOX_COLUMNS_TWOLINES),
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



static void
free_element (gpointer data, gpointer user_data)
{
	g_free (data);
}


gboolean
modest_init_default_account_maybe  (void)
{
	ModestAccountMgr *acc_mgr;

	GSList *all_accounts = NULL;
	gchar *default_account;
	gboolean retval = TRUE;
	
	acc_mgr = get_account_mgr ();
	if (!acc_mgr) {
		g_printerr ("modest: cannot get modest account mgr\n");
		return FALSE;
	}

	all_accounts = modest_account_mgr_account_names (acc_mgr, NULL);
	if (all_accounts) { /* if there are any accounts, there should be a default one */
		default_account = 
			modest_account_mgr_get_default_account (acc_mgr);
		if (!default_account) {
			gchar *first_account;
			g_printerr ("modest: no default account defined\n");
			first_account = (gchar*)all_accounts->data;
			if ((retval = modest_account_mgr_set_default_account (acc_mgr, first_account)))
				g_printerr ("modest: set '%s' as the default account\n",
					    first_account);
			else
				g_printerr ("modest: failed to set '%s' as the default account\n",
					    first_account);
			g_free (default_account);
		}
		g_slist_foreach (all_accounts, free_element, NULL);
		g_slist_free    (all_accounts);
	}
	return retval;
}
