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

#include <sys/utsname.h>
#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <modest-runtime.h>
#include <modest-runtime-priv.h>
#include <modest-init.h>
#include <modest-defs.h>
#include "modest-address-book.h"
#include <modest-singletons.h>
#include <widgets/modest-header-view.h>
#include <widgets/modest-folder-view.h>
#include <modest-tny-platform-factory.h>
#include <modest-platform.h>
#include <modest-widget-memory.h>
#include <modest-widget-memory-priv.h>
#include <modest-local-folder-info.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-icon-names.h>
#include "widgets/modest-global-settings-dialog.h"
#include "modest-tny-msg.h"
#include <libgnomevfs/gnome-vfs.h>
#include <string.h>
#include "modest-text-utils.h"

#ifndef MODEST_TOOLKIT_GTK
#include "modest-hildon-includes.h"
#endif
#include <locale.h>

static gboolean init_header_columns (ModestConf *conf, gboolean overwrite);
static gboolean init_default_account_maybe  (ModestAccountMgr *acc_mgr);
static void     init_i18n (void);
static void     init_stock_icons (void);
static void     init_debug_g_type (void);
static void     init_debug_logging (void);
static void     init_default_settings (ModestConf *conf);
static void     init_device_name (ModestConf *conf);
static gboolean init_ui (gint argc, gchar** argv);


static gboolean _is_initialized = FALSE;

/*
 * defaults for the column headers
 */
typedef struct {
	ModestHeaderViewColumn col;
	guint                  width;
	gint                  sort;
} FolderCols;


static const guint MODEST_MAIN_PANED_POS_PERCENTAGE = 30;
static const guint MODEST_MSG_PANED_POS_PERCENTAGE = 50;

static const FolderCols INBOX_COLUMNS_DETAILS[] = {
	{MODEST_HEADER_VIEW_COLUMN_ATTACH,  40, 0},
	{MODEST_HEADER_VIEW_COLUMN_FROM,    80, 0},
	{MODEST_HEADER_VIEW_COLUMN_SUBJECT, 80, 0},
	{MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE, 60, 0},
	{MODEST_HEADER_VIEW_COLUMN_SIZE, 50, 0}
};

static const FolderCols INBOX_COLUMNS_TWOLINES[] = {
	{MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN, 180, 0},
};

static const FolderCols OUTBOX_COLUMNS_DETAILS[] = {
	{MODEST_HEADER_VIEW_COLUMN_ATTACH,  40, 0},
	{MODEST_HEADER_VIEW_COLUMN_TO,    80, 0},
	{MODEST_HEADER_VIEW_COLUMN_SUBJECT, 80, 0},
	{MODEST_HEADER_VIEW_COLUMN_SENT_DATE, 80, 0},
	{MODEST_HEADER_VIEW_COLUMN_SIZE, 50, 0}
};

static const FolderCols OUTBOX_COLUMNS_TWOLINES[] = {
	{MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT,180, 0}
//	{MODEST_HEADER_VIEW_COLUMN_STATUS, 240, 0}
};

static const FolderCols SENT_COLUMNS_TWOLINES[] = {
	{MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT,180, 0},
};

#ifndef MODEST_TOOLKIT_GTK
static const TnyFolderType LOCAL_FOLDERS[] = {
/*	TNY_FOLDER_TYPE_OUTBOX, */
	TNY_FOLDER_TYPE_DRAFTS,
	TNY_FOLDER_TYPE_SENT
};
#else
static const TnyFolderType LOCAL_FOLDERS[] = {
/*	TNY_FOLDER_TYPE_OUTBOX, */
	TNY_FOLDER_TYPE_DRAFTS,
	TNY_FOLDER_TYPE_SENT,
	TNY_FOLDER_TYPE_TRASH,
	TNY_FOLDER_TYPE_ARCHIVE	
};
#endif /* MODEST_TOOLKIT_GTK */

static GList*
new_cold_ids_gslist_from_array( const FolderCols* cols, guint col_num)
{
	GList *result = NULL;
	
	guint i = 0;
	for (i = 0; i < col_num; ++i) {
		result = g_list_append (result, GINT_TO_POINTER (cols[i].col));
	}
	
	return result;
}

GList* 
modest_init_get_default_header_view_column_ids (TnyFolderType folder_type, ModestHeaderViewStyle style)
{
		GList *result = NULL;
		
		switch (folder_type) {
		case TNY_FOLDER_TYPE_SENT:
		case TNY_FOLDER_TYPE_DRAFTS:
			if (style == MODEST_HEADER_VIEW_STYLE_DETAILS)
				result = new_cold_ids_gslist_from_array (OUTBOX_COLUMNS_DETAILS,
				      G_N_ELEMENTS(OUTBOX_COLUMNS_DETAILS));
			else if (style == MODEST_HEADER_VIEW_STYLE_TWOLINES)
				result = new_cold_ids_gslist_from_array (SENT_COLUMNS_TWOLINES,
				      G_N_ELEMENTS(SENT_COLUMNS_TWOLINES));
		break;
		case TNY_FOLDER_TYPE_OUTBOX:
			if (style == MODEST_HEADER_VIEW_STYLE_TWOLINES)
				result = new_cold_ids_gslist_from_array (OUTBOX_COLUMNS_TWOLINES,
				      G_N_ELEMENTS(OUTBOX_COLUMNS_TWOLINES));
		break;

		default:
			if (style == MODEST_HEADER_VIEW_STYLE_DETAILS)
				result =  new_cold_ids_gslist_from_array (INBOX_COLUMNS_DETAILS,
				      G_N_ELEMENTS(INBOX_COLUMNS_DETAILS));
			else if (style == MODEST_HEADER_VIEW_STYLE_TWOLINES)
				result = new_cold_ids_gslist_from_array (INBOX_COLUMNS_TWOLINES,
				      G_N_ELEMENTS(INBOX_COLUMNS_TWOLINES));
		};
		
		if (!result) {
			g_warning("DEBUG: %s: No default columns IDs found for "
				"folder_type=%d, style=%d\n", __FUNCTION__, folder_type, style);	
		}
		
		return result;
}


static gboolean
force_ke_recv_load (void)
{
	if (strcmp ("cerm_device_memory_full",
		    _KR("cerm_device_memory_full")) == 0) {
		g_debug ("%s: cannot get translation for cerm_device_memory_full",
			   __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

gboolean
modest_init (int argc, char *argv[])
{
	gboolean reset;

	if (_is_initialized) {
		g_printerr ("modest: %s may only be invoked once\n", __FUNCTION__);
		return FALSE;
	}

	init_i18n();

	if (!force_ke_recv_load()) {
		g_printerr ("modest: %s: ke-recv is missing "
			    "or memory is very low\n", __FUNCTION__);
		/* don't return FALSE here, because it might be that ke-recv is 
		   missing. TODO: find a way to verify that
		*/
	}

	init_debug_g_type();
	init_debug_logging();

	/* initialize the prng, we need it when creating random files */
	srandom((int)getpid());

	if (!gnome_vfs_initialized()) {
		if (!gnome_vfs_init ()) {
			g_printerr ("modest: failed to init gnome-vfs\n");
			return FALSE;
		}
	}

	if (!modest_runtime_init()) {
		modest_init_uninit ();
		g_printerr ("modest: failed to initialize the modest runtime\n");
		return FALSE;
	}

	modest_plugin_factory_load_all (modest_runtime_get_plugin_factory ());

	/* do an initial guess for the device name */
	init_device_name (modest_runtime_get_conf());

	if (!modest_platform_init(argc, argv)) {
		modest_init_uninit ();
		g_printerr ("modest: failed to run platform-specific initialization\n");
		return FALSE;
	}

	/* Initialize addressbook */
	modest_address_book_init ();

	reset = modest_runtime_get_debug_flags () & MODEST_RUNTIME_DEBUG_FACTORY_SETTINGS;
	if (!init_header_columns(modest_runtime_get_conf(), reset)) {
		modest_init_uninit ();
		g_printerr ("modest: failed to init header columns\n");
		return FALSE;
	}

	init_default_settings (modest_runtime_get_conf ());

	if (!modest_init_local_folders(NULL)) {
		modest_init_uninit ();
		g_printerr ("modest: failed to init local folders\n");
		return FALSE;
	}

	if (!init_default_account_maybe (modest_runtime_get_account_mgr ())) {
		modest_init_uninit ();
		g_printerr ("modest: failed to init default account\n");
		return FALSE;
	}

	if (!init_ui (argc, argv)) {
		modest_init_uninit ();
		g_printerr ("modest: failed to init ui\n");
		return FALSE;
	}

	return _is_initialized = TRUE;
}


static gboolean
init_ui (gint argc, gchar** argv)
{
	/* Set application name */
	g_set_application_name (modest_platform_get_app_name());
	/* g_debug (modest_platform_get_app_name()); */

	/* Init stock icons */
	init_stock_icons ();

		/* Init notification system */
#ifdef MODEST_HAVE_HILDON_NOTIFY
	notify_init ("Basics");
#endif
	return TRUE;
}


gboolean
modest_init_uninit (void)
{
	if (!_is_initialized)
		return TRUE; 
	
	if (!modest_runtime_uninit())
		g_printerr ("modest: failed to uninit runtime\n");

	if (!modest_platform_uninit())
		g_printerr ("modest: failed to uninit platform\n");
	
	if (gnome_vfs_initialized()) /* apparently, this returns TRUE, even after a shutdown */
		gnome_vfs_shutdown ();

	_is_initialized = FALSE;
	return TRUE;
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
	gchar *sort_key;
	gchar *sort_value;
	GString *str;

	g_return_val_if_fail (cols, FALSE);

	key = _modest_widget_memory_get_keyname_with_double_type ("header-view",
								  type, style,
								  MODEST_WIDGET_MEMORY_PARAM_COLUMN_WIDTH);
	sort_key = _modest_widget_memory_get_keyname_with_double_type ("header-view",
								       type, style,
								       MODEST_WIDGET_MEMORY_PARAM_COLUMN_SORT);
	/* if we're not in overwrite mode, only write stuff it
	 * there was nothing before */
	if (!overwrite &&  modest_conf_key_exists(conf, key, NULL)) {
		g_free (key);
		g_free (sort_key);
		return TRUE;
	}

	/* the format is necessarily the same as the one in modest-widget-memory */
	str = g_string_new (NULL);
	for (i = 0; i != col_num; ++i) 
		g_string_append_printf (str, "%d:%d:%d ",
					cols[i].col, cols[i].width, cols[i].sort); 

	modest_conf_set_string (conf, key, str->str, NULL);
	g_free (key);
	g_string_free (str, TRUE);

	if ( col_num > 0 ) {
		gint sort_col_id;
		if (cols[0].col == MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT)
			sort_col_id = TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN;
		else
			sort_col_id = TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN;
		sort_value = g_strdup_printf("%d:%d:%d", sort_col_id, GTK_SORT_DESCENDING, 0);
		modest_conf_set_string (conf, sort_key, sort_value, NULL);
		g_free (sort_value);
	}
	g_free (sort_key);
	
	return TRUE;
}

/**
 * modest_init_header_columns:
 * @overwrite: write the setting, even if it already exists
 * 
 * will set defaults for the columns to show for folder,
 * if there are no such settings yet (in ModestWidgetMemory)
 * 
 * Returns: TRUE if succeeded, FALSE in case of error
 */
static gboolean
init_header_columns (ModestConf *conf, gboolean overwrite)
{
	int folder_type;
	gchar *key;
	
	for (folder_type = TNY_FOLDER_TYPE_UNKNOWN;
	     folder_type < TNY_FOLDER_TYPE_NUM; ++folder_type) {		
		
		switch (folder_type) {
		case TNY_FOLDER_TYPE_SENT:
		case TNY_FOLDER_TYPE_DRAFTS:
		save_header_settings (conf, folder_type,
				      MODEST_HEADER_VIEW_STYLE_DETAILS,
				      OUTBOX_COLUMNS_DETAILS,
				      G_N_ELEMENTS(OUTBOX_COLUMNS_DETAILS),
				      overwrite);
		save_header_settings (conf, folder_type,
				      MODEST_HEADER_VIEW_STYLE_TWOLINES,
				      SENT_COLUMNS_TWOLINES,
				      G_N_ELEMENTS(SENT_COLUMNS_TWOLINES),
				      overwrite);
		break;
		case TNY_FOLDER_TYPE_OUTBOX:
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
	
	key = _modest_widget_memory_get_keyname (MODEST_CONF_MAIN_PANED_KEY, 
						 MODEST_WIDGET_MEMORY_PARAM_POS);
	/* if we're not in overwrite mode, only write stuff it
	 * there was nothing before */
	if (overwrite || !modest_conf_key_exists(conf, key, NULL)) 
		modest_conf_set_float (conf, key, MODEST_MAIN_PANED_POS_PERCENTAGE, NULL);
	
	g_free (key);

	key = _modest_widget_memory_get_keyname (MODEST_CONF_MSG_PANED_KEY, 
						 MODEST_WIDGET_MEMORY_PARAM_POS);
	/* if we're not in overwrite mode, only write stuff it
	 * there was nothing before */
	if (overwrite || !modest_conf_key_exists(conf, key, NULL)) 
		modest_conf_set_float (conf, key, MODEST_MSG_PANED_POS_PERCENTAGE, NULL);
	
	g_free (key);
	return TRUE;
}

gboolean modest_init_one_local_folder (gchar *maildir_path)
{
	static const gchar* maildirs[] = {
		"cur", "new", "tmp"
	};
	
	int j;
	for (j = 0; j != G_N_ELEMENTS(maildirs); ++j) {
		gchar *dir = g_build_filename (maildir_path,
					maildirs[j],
					NULL);
		if (g_mkdir_with_parents (dir, 0755) < 0) {
			g_printerr ("modest: %s: failed to create %s\n", __FUNCTION__, dir);
			g_free (dir);
			return FALSE;
		}
		
		g_free (dir);
	}

	return TRUE;
}

/**
 * modest_init_local_folders:
 * 
 * create the Local Folders folder under cache, if they
 * do not exist yet.
 * 
 * Returns: TRUE if the folder were already there, or
 * they were created, FALSE otherwise
 */
gboolean
modest_init_local_folders (const gchar* location_filepath)
{
	gboolean retval = TRUE;

	gchar *maildir_path = modest_local_folder_info_get_maildir_path (location_filepath);

	if (location_filepath) {
		/* For instance, for memory card, just create the top-level .modest folder
		 * and one "archive" folder (so that messages can be put somewhere):
		 */

		gchar *dir = g_build_filename (maildir_path,
					       modest_local_folder_info_get_type_name(TNY_FOLDER_TYPE_ARCHIVE),
					       NULL);
		const gboolean created = modest_init_one_local_folder (dir);
		g_free(dir);

		if (!created) {
			retval = FALSE;
		}
	}
	else {
		/* Create each of the standard on-disk folders.
		 * Per-account outbox folders will be created when first needed. */
		int i;
		for (i = 0; i != G_N_ELEMENTS(LOCAL_FOLDERS); ++i) {
			gchar *dir = g_build_filename (maildir_path,
							modest_local_folder_info_get_type_name(LOCAL_FOLDERS[i]),
							NULL);
			const gboolean created = modest_init_one_local_folder (dir);
			g_free(dir);

			if (!created) {
				retval = FALSE;
			}
		}
	}

	g_free (maildir_path);
	return retval;
}

/**
 * init_default_account_maybe:
 *
 * if there are accounts defined, but there is no default account,
 * it will be defined.
 * 
 * Returns: TRUE if there was a default account already,
 *  or one has been created or there are no accounts yet,
 *  returns FALSE in case of error
 */
static gboolean
init_default_account_maybe  (ModestAccountMgr *acc_mgr)
{
	gchar *default_account;
	gboolean retval = TRUE;

	default_account =  modest_account_mgr_get_default_account (acc_mgr);
	if (!default_account)
		retval = modest_account_mgr_set_first_account_as_default (acc_mgr);
	g_free (default_account);

	return retval;
}



static void
init_debug_g_type (void)
{
	GTypeDebugFlags gflags;
	ModestRuntimeDebugFlags mflags;
	
	gflags = 0;
	mflags = modest_runtime_get_debug_flags ();

	if (mflags & MODEST_RUNTIME_DEBUG_OBJECTS)
		gflags |= G_TYPE_DEBUG_OBJECTS;
	if (mflags & MODEST_RUNTIME_DEBUG_SIGNALS)
		gflags |= G_TYPE_DEBUG_SIGNALS;

	g_type_init_with_debug_flags (gflags);
}

#ifndef DEBUG
static void 
null_log(const gchar* dom, 
	 GLogLevelFlags l, 
	 const gchar* m, 
	 gpointer d)
{
	return;
};
#endif

static void
init_debug_logging (void)
{
	ModestRuntimeDebugFlags mflags;
	mflags = modest_runtime_get_debug_flags ();

#ifndef DEBUG
	if (! (mflags & MODEST_RUNTIME_DEBUG_CODE)) {
		g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, null_log, NULL);
	}
#endif

	if (mflags & MODEST_RUNTIME_DEBUG_ABORT_ON_WARNING)
		g_log_set_always_fatal (G_LOG_LEVEL_ERROR |
					G_LOG_LEVEL_CRITICAL |
					G_LOG_LEVEL_WARNING);
}

static void
init_i18n (void)
{
	const gchar *lc_messages = setlocale (LC_MESSAGES, NULL);

	if (!lc_messages) {
		setenv ("LANGUAGE", "en_GB", 1);
		setenv ("LC_MESSAGES", "en_GB", 1);
#ifdef MODEST_PLATFORM_GNOME
	} else {
		gchar *new_lc_messages;
		new_lc_messages = g_strconcat (lc_messages, ":en_GB", NULL);
		setenv ("LANGUAGE", new_lc_messages, 1);
		setenv ("LC_MESSAGES", new_lc_messages, 1);
		g_free (new_lc_messages);
#endif
	}

	bindtextdomain (GETTEXT_PACKAGE, MODEST_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
}


/* 
 *  This function registers our custom toolbar icons, so they can be
 *  themed. The idea of this function was taken from the gtk-demo
 */
static void
init_stock_icons (void)
{
	static gboolean registered = FALSE;

	if (!registered) {
		GtkIconTheme *current_theme;
		GdkPixbuf *pixbuf;
		GtkIconFactory *factory;
		gint i;

		static GtkStockItem items[] = {
#ifndef MODEST_TOOLKIT_GTK
			{ MODEST_STOCK_SORT, "sort mail", 0, 0, NULL },
			{ MODEST_STOCK_REFRESH, "refresh mail", 0, 0, NULL },
#endif /*MODEST_TOOLKIT_GTK*/
			{ MODEST_STOCK_SPLIT_VIEW, "split view", 0, 0, NULL },
			{ MODEST_STOCK_MAIL_SEND, "send mail", 0, 0, NULL },
			{ MODEST_STOCK_NEW_MAIL, "new mail", 0, 0, NULL },
			{ MODEST_STOCK_REPLY, "reply", 0, 0, NULL },
			{ MODEST_STOCK_REPLY_ALL, "reply all", 0, 0, NULL },
			{ MODEST_STOCK_FORWARD, "forward", 0, 0, NULL },
 			{ MODEST_STOCK_DELETE, "delete", 0, 0, NULL },
		};

		static gchar *items_names [] = {
#ifndef MODEST_TOOLKIT_GTK
			MODEST_TOOLBAR_ICON_SORT,
			MODEST_TOOLBAR_ICON_REFRESH,
#endif /*MODEST_TOOLKIT_GTK*/
			MODEST_TOOLBAR_ICON_SPLIT_VIEW,
			MODEST_TOOLBAR_ICON_MAIL_SEND,
			MODEST_TOOLBAR_ICON_NEW_MAIL,
			MODEST_TOOLBAR_ICON_REPLY,
			MODEST_TOOLBAR_ICON_REPLY_ALL,
			MODEST_TOOLBAR_ICON_FORWARD,
 			MODEST_TOOLBAR_ICON_DELETE,
		};

		registered = TRUE;

		/* Register our stock items */
		gtk_stock_add (items, G_N_ELEMENTS (items));

		/* Add our custom icon factory to the list of defaults */
		factory = gtk_icon_factory_new ();
		gtk_icon_factory_add_default (factory);

		current_theme = gtk_icon_theme_get_default ();

		/* Register icons to accompany stock items */
		for (i = 0; i < G_N_ELEMENTS (items); i++) {

#ifndef MODEST_PLATFORM_GTK
			pixbuf = gtk_icon_theme_load_icon (current_theme,
							   items_names[i],
#ifdef MODEST_TOOLKIT_HILDON2
							   MODEST_ICON_SIZE_BIG,
#else
							   MODEST_ICON_SIZE_SMALL,
#endif
							   GTK_ICON_LOOKUP_NO_SVG,
							   NULL);
#else
			pixbuf = gdk_pixbuf_new_from_file (items_names[i], NULL);
#endif

			if (pixbuf != NULL) {
				GtkIconSet *icon_set;

#ifndef MODEST_TOOLKIT_HILDON2
				GdkPixbuf *transparent;
				transparent = gdk_pixbuf_add_alpha (pixbuf, TRUE, 0xff, 0xff, 0xff);
				icon_set = gtk_icon_set_new_from_pixbuf (transparent);
				g_object_unref (transparent);
#else
				icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
#endif
				gtk_icon_factory_add (factory, items[i].stock_id, icon_set);
				gtk_icon_set_unref (icon_set);
				g_object_unref (pixbuf);
			}
			else
				g_warning ("%s: failed to load %s icon", __FUNCTION__, items_names[i]);
		}
		/* Drop our reference to the factory, GTK will hold a reference. */
		g_object_unref (factory);
	}
}


static void
init_default_settings (ModestConf *conf)
{
	/* Show toolbar keys */
	if (!modest_conf_key_exists (conf, MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR, TRUE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR_FULLSCREEN, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR_FULLSCREEN, TRUE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR, TRUE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR_FULLSCREEN, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR_FULLSCREEN, TRUE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR, TRUE, NULL);
	
	if (!modest_conf_key_exists (conf, MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR_FULLSCREEN, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR_FULLSCREEN, TRUE, NULL);

	/* Editor keys */
	if (!modest_conf_key_exists (conf, MODEST_CONF_SHOW_CC, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_SHOW_CC, FALSE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_SHOW_BCC, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_SHOW_BCC, FALSE, NULL);

	/* File chooser keys */
	if (!modest_conf_key_exists (conf, MODEST_CONF_LATEST_ATTACH_FILE_PATH, NULL))
		modest_conf_set_string (conf, MODEST_CONF_LATEST_ATTACH_FILE_PATH, "", NULL);
	if (!modest_conf_key_exists (conf, MODEST_CONF_LATEST_INSERT_IMAGE_PATH, NULL))
		modest_conf_set_string (conf, MODEST_CONF_LATEST_INSERT_IMAGE_PATH, "", NULL);
	if (!modest_conf_key_exists (conf, MODEST_CONF_LATEST_SAVE_ATTACHMENT_PATH, NULL))
		modest_conf_set_string (conf, MODEST_CONF_LATEST_SAVE_ATTACHMENT_PATH, "", NULL);

	/* Global settings */
	if (!modest_conf_key_exists (conf, MODEST_CONF_AUTO_UPDATE, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_AUTO_UPDATE, TRUE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_NOTIFICATIONS, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_NOTIFICATIONS, TRUE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_AUTO_ADD_TO_CONTACTS, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_AUTO_ADD_TO_CONTACTS, TRUE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, NULL))
		modest_conf_set_int (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, MODEST_CONNECTED_VIA_WLAN_OR_WIMAX, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_UPDATE_INTERVAL, NULL))
		modest_conf_set_int (conf, MODEST_CONF_UPDATE_INTERVAL, MODEST_UPDATE_INTERVAL_30_MIN, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_MSG_SIZE_LIMIT, NULL))
		modest_conf_set_int (conf, MODEST_CONF_MSG_SIZE_LIMIT, 100, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, FALSE, NULL);

#ifdef MODEST_TOOLKIT_GTK
	/* In Gnome port, we only allow editting plain text */
	modest_conf_set_bool (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, FALSE, NULL);
#else
	if (!modest_conf_key_exists (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, TRUE, NULL);
#endif

	if (!modest_conf_key_exists (conf, MODEST_CONF_REPLY_TYPE, NULL))
		modest_conf_set_int (conf, MODEST_CONF_REPLY_TYPE, MODEST_TNY_MSG_REPLY_TYPE_QUOTE, NULL);

	if (!modest_conf_key_exists (conf, MODEST_CONF_FETCH_HTML_EXTERNAL_IMAGES, NULL))
		modest_conf_set_bool (conf, MODEST_CONF_FETCH_HTML_EXTERNAL_IMAGES, FALSE, NULL);
}


/* set the device name -- note this is an initial guess from /etc/hostname
 * on maemo-device it will most probably be replaced with the Bluetooth device
 * name later during starting (see maemo/modest-maemo-utils.[ch])
 */
static void
init_device_name (ModestConf *conf)
{
	struct utsname name;

	if (uname (&name) == 0) {
		modest_conf_set_string (modest_runtime_get_conf(),
					MODEST_CONF_DEVICE_NAME, name.nodename,
					NULL);
	} else {
		modest_conf_set_string (modest_runtime_get_conf(),
					MODEST_CONF_DEVICE_NAME,
					MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME,
					NULL);
	}
}
