/* Copyright (c) 2007, Nokia Corporation
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


#include <string.h>
#include <stdio.h>
#include <config.h>
#include <glib/gi18n.h>
#include <tny-error.h>
#include <modest-tny-local-folders-account.h>
#include <modest-tny-outbox-account.h>
#include <modest-tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-merge-folder.h>
#include <tny-simple-list.h>
#include <tny-gtk-lockable.h>

G_DEFINE_TYPE (ModestTnyLocalFoldersAccount, 
	modest_tny_local_folders_account, 
	TNY_TYPE_CAMEL_STORE_ACCOUNT);

#define TNY_LOCAL_FOLDERS_ACCOUNT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT, ModestTnyLocalFoldersAccountPrivate))

typedef struct _ModestTnyLocalFoldersAccountPrivate ModestTnyLocalFoldersAccountPrivate;

struct _ModestTnyLocalFoldersAccountPrivate
{
	TnyMergeFolder *outbox_folder;
};

static void         get_folders    (TnyFolderStore *self, 
				    TnyList *list, 
				    TnyFolderStoreQuery *query,
				    gboolean refresh, 
				    GError **err);

static TnyFolder*   create_folder  (TnyFolderStore *self, 
				    const gchar *name, 
				    GError **err);

enum {
	OUTBOX_DELETED_SIGNAL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void
modest_tny_local_folders_account_finalize (GObject *object)
{
	ModestTnyLocalFoldersAccountPrivate *priv;

	priv = TNY_LOCAL_FOLDERS_ACCOUNT_GET_PRIVATE (object);
	if (priv->outbox_folder) {
		g_object_unref (priv->outbox_folder);
		priv->outbox_folder = NULL;
	}
	G_OBJECT_CLASS (modest_tny_local_folders_account_parent_class)->finalize (object);
}

static void
modest_tny_local_folders_account_class_init (ModestTnyLocalFoldersAccountClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestTnyLocalFoldersAccountPrivate));

	object_class->finalize = modest_tny_local_folders_account_finalize;

	/* Signals */

	/* Note that this signal is removed before unsetting my own
	   reference to outbox, this means that by the time of this
	   call, modest_tny_local_folders_account_get_merged_outbox is
	   still valid. The reason is that the listeners of the signal
	   might want to do something with the outbox instance */
	signals[OUTBOX_DELETED_SIGNAL] = g_signal_new
	("outbox-deleted", MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT,
	G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET
	(ModestTnyLocalFoldersAccountClass, outbox_deleted), NULL,
	NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	/* Override virtual functions from the parent class: */
	TNY_CAMEL_STORE_ACCOUNT_CLASS(klass)->get_folders = get_folders;
	TNY_CAMEL_STORE_ACCOUNT_CLASS(klass)->create_folder = create_folder;
}

static void
modest_tny_local_folders_account_init (ModestTnyLocalFoldersAccount *self)
{
	/* Do nothing */
}

ModestTnyLocalFoldersAccount*
modest_tny_local_folders_account_new (void)
{
  return g_object_new (MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT, NULL);
}

/**********************************************************/
/*          TnyCamelStoreAccount functions redefinitions  */
/**********************************************************/
static gboolean 
modest_tny_local_folders_account_query_passes (TnyFolderStoreQuery *query, TnyFolder *folder)
{
	gboolean retval = FALSE;

	if (query && (tny_list_get_length (tny_folder_store_query_get_items (query)) > 0)) {
		TnyList *items = tny_folder_store_query_get_items (query);
		TnyIterator *iterator;
		iterator = tny_list_create_iterator (items);

		while (!tny_iterator_is_done (iterator))
		{
			TnyFolderStoreQueryItem *item = (TnyFolderStoreQueryItem*) tny_iterator_get_current (iterator);
			if (item) {
				TnyFolderStoreQueryOption options = tny_folder_store_query_item_get_options (item);
				const regex_t *regex = tny_folder_store_query_item_get_regex (item);

				if ((options & TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED) &&
			 	   tny_folder_is_subscribed (folder))
					retval = TRUE;

				if ((options & TNY_FOLDER_STORE_QUERY_OPTION_UNSUBSCRIBED) &&
				    !(tny_folder_is_subscribed (folder)))
					retval = TRUE;

				if (regex && options & TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME)
			 	   if (regexec (regex, tny_folder_get_name (folder), 0, NULL, 0) == 0)
					retval = TRUE;

				if (regex && options & TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID)
			  	  if (regexec (regex, tny_folder_get_id (folder), 0, NULL, 0) == 0)
					retval = TRUE;

				g_object_unref (G_OBJECT (item));
			}

			tny_iterator_next (iterator);
		}
		 
		g_object_unref (G_OBJECT (iterator));
		g_object_unref (G_OBJECT (items));
	} else
		retval = TRUE;

	return retval;
}

static void
get_folders (TnyFolderStore *self, 
	     TnyList *list, 
	     TnyFolderStoreQuery *query, 
	     gboolean refresh, 
	     GError **err)
{
	TnyCamelStoreAccountClass *parent_class;
	ModestTnyLocalFoldersAccountPrivate *priv;

	/* Call the base class implementation: */
	parent_class = g_type_class_peek_parent (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT_GET_CLASS (self));
	parent_class->get_folders (self, list, query, refresh, err);
	
	/* Add our extra folder only if it passes the query */
	priv = TNY_LOCAL_FOLDERS_ACCOUNT_GET_PRIVATE (self);
		
	if (priv->outbox_folder && 
	    modest_tny_local_folders_account_query_passes (query, TNY_FOLDER (priv->outbox_folder)))
		tny_list_prepend (list, G_OBJECT (priv->outbox_folder));
}

static TnyFolder*
create_folder (TnyFolderStore *self, 
	       const gchar *name, 
	       GError **err)
{
	TnyCamelStoreAccountClass *parent_class;

	parent_class = g_type_class_peek_parent (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT_GET_CLASS (self));

	/* If the folder name is been used by our extra folders */
	if (modest_tny_local_folders_account_folder_name_in_use (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (self), name)) {
		g_set_error (err, TNY_ERROR_DOMAIN, 
			     TNY_SERVICE_ERROR_FOLDER_CREATE,
			     "Folder name already in use");
		return NULL;
	}

	/* Call the base class implementation: */
	return parent_class->create_folder (self, name, err);
}

/*****************************/
/*      Public methods       */ 
/*****************************/
gboolean
modest_tny_local_folders_account_folder_name_in_use (ModestTnyLocalFoldersAccount *self,
						     const gchar *name)
{
	ModestTnyLocalFoldersAccountPrivate *priv;
	gchar *down_name;
	const gchar *type_name;
	gboolean retval;
	
	/* Check that we're not trying to create/rename any folder
	   with the same name that our OUTBOX, DRAFT, SENT */
	priv = TNY_LOCAL_FOLDERS_ACCOUNT_GET_PRIVATE (self);
	down_name = g_utf8_strdown (name, strlen (name));

	type_name = modest_local_folder_info_get_type_name (TNY_FOLDER_TYPE_OUTBOX);
	if (!strcmp (type_name, down_name)) {
		retval = TRUE;
	} else {
		type_name = modest_local_folder_info_get_type_name (TNY_FOLDER_TYPE_DRAFTS);
		if (!strcmp (type_name, down_name)) {
			retval = TRUE;
		} else {
			type_name = modest_local_folder_info_get_type_name (TNY_FOLDER_TYPE_SENT);
			if (!strcmp (type_name, down_name)) {
				retval = TRUE;
			} else {
				retval = FALSE;
			}
		}
	}
	g_free (down_name);

	return retval;
}

void
modest_tny_local_folders_account_add_folder_to_outbox (ModestTnyLocalFoldersAccount *self, 
						       TnyFolder *per_account_outbox)
{
	ModestTnyLocalFoldersAccountPrivate *priv;

	g_return_if_fail (MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (self));
	g_return_if_fail (TNY_IS_FOLDER (per_account_outbox));

	priv = TNY_LOCAL_FOLDERS_ACCOUNT_GET_PRIVATE (self);

	/* Create on-demand */
	if (!priv->outbox_folder) {
		priv->outbox_folder = (TnyMergeFolder *) 
			tny_merge_folder_new_with_ui_locker (_("mcen_me_folder_outbox"), 
							     tny_gtk_lockable_new ());
		/* Set type to outbox */
		tny_merge_folder_set_folder_type (priv->outbox_folder, 
						  TNY_FOLDER_TYPE_OUTBOX);
	}

	/* Add outbox to the global OUTBOX folder */
	tny_merge_folder_add_folder (priv->outbox_folder, per_account_outbox);
}

void 
modest_tny_local_folders_account_remove_folder_from_outbox (ModestTnyLocalFoldersAccount *self, 
							    TnyFolder *per_account_outbox)
{
	ModestTnyLocalFoldersAccountPrivate *priv;
	TnyList *merged_folders = NULL;

	g_return_if_fail (MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (self));
	g_return_if_fail (TNY_IS_FOLDER (per_account_outbox));

	priv = TNY_LOCAL_FOLDERS_ACCOUNT_GET_PRIVATE (self);

	/* Remove outbox from the global OUTBOX folder */
	tny_merge_folder_remove_folder (priv->outbox_folder, per_account_outbox);

	/* If there is no folder in the outbox the delete it */
	merged_folders = tny_simple_list_new ();
	tny_merge_folder_get_folders (priv->outbox_folder, merged_folders);
	if (tny_list_get_length (merged_folders) == 0) {
		/* Emit signal */
		g_signal_emit ((GObject *)self, signals[OUTBOX_DELETED_SIGNAL], 0);

		/* Unref my own reference */
		g_object_unref (priv->outbox_folder);
		priv->outbox_folder = NULL;
	}
	g_object_unref (merged_folders);
}

TnyFolder *
modest_tny_local_folders_account_get_merged_outbox (ModestTnyLocalFoldersAccount *self)
{
	ModestTnyLocalFoldersAccountPrivate *priv;
	g_return_val_if_fail (MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (self), NULL);

	priv = TNY_LOCAL_FOLDERS_ACCOUNT_GET_PRIVATE (self);
	if (priv->outbox_folder)
		return TNY_FOLDER(g_object_ref (priv->outbox_folder));
	else
		return NULL;
}
