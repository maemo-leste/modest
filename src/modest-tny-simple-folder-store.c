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
#include <glib.h>
#include <glib/gi18n-lib.h>

#include <modest-tny-simple-folder-store.h>

static void
tny_folder_store_init (TnyFolderStoreIface *klass);

G_DEFINE_TYPE_EXTENDED (ModestTnySimpleFolderStore, 
	modest_tny_simple_folder_store, 
	G_TYPE_OBJECT,
	0,
	G_IMPLEMENT_INTERFACE (TNY_TYPE_FOLDER_STORE, tny_folder_store_init));

#define TNY_SIMPLE_FOLDER_STORE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_TNY_SIMPLE_FOLDER_STORE, ModestTnySimpleFolderStorePrivate))

typedef struct _ModestTnySimpleFolderStorePrivate ModestTnySimpleFolderStorePrivate;

struct _ModestTnySimpleFolderStorePrivate
{
	GSList *list_folders;
};

static void
modest_tny_simple_folder_store_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (modest_tny_simple_folder_store_parent_class)->dispose)
    G_OBJECT_CLASS (modest_tny_simple_folder_store_parent_class)->dispose (object);
}

static void
modest_tny_simple_folder_store_finalize (GObject *object)
{
  G_OBJECT_CLASS (modest_tny_simple_folder_store_parent_class)->finalize (object);
  
  ModestTnySimpleFolderStorePrivate *priv = 
		TNY_SIMPLE_FOLDER_STORE_GET_PRIVATE (object);
		
  GSList *iter = priv->list_folders;
  while (iter)
  {
  	TnyFolder *folder = (TnyFolder*)iter->data;
  	if (folder) {
  		g_object_unref (folder);
  		iter->data = NULL;
  	}
  		
  	iter = g_slist_next (iter);
  }

  g_slist_free (priv->list_folders);
  priv->list_folders = NULL;
}

static void
modest_tny_simple_folder_store_class_init (ModestTnySimpleFolderStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ModestTnySimpleFolderStorePrivate));

  object_class->dispose = modest_tny_simple_folder_store_dispose;
  object_class->finalize = modest_tny_simple_folder_store_finalize;
}

static void
modest_tny_simple_folder_store_init (ModestTnySimpleFolderStore *self)
{
}

ModestTnySimpleFolderStore*
modest_tny_simple_folder_store_new (void)
{
  return g_object_new (MODEST_TYPE_TNY_SIMPLE_FOLDER_STORE, NULL);
}


static void
modest_tny_simple_folder_store_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
}

static TnyFolder*
modest_tny_simple_folder_store_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	return NULL;
}

static void
modest_tny_simple_folder_store_get_folders (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, GError **err)
{
  ModestTnySimpleFolderStorePrivate *priv = 
		TNY_SIMPLE_FOLDER_STORE_GET_PRIVATE (self);
		
  if (!list)
    return;
    
  GSList *iter = priv->list_folders;
  while (iter)
  {
  	TnyFolder *folder = (TnyFolder*)iter->data;
  	if (folder) {
  		tny_list_append (list, G_OBJECT (folder));
  	}
  		
  	iter = g_slist_next (iter);
  }
  
}

static void
modest_tny_simple_folder_store_get_folders_async (TnyFolderStore *self, TnyList *list, TnyGetFoldersCallback callback, TnyFolderStoreQuery *query, TnyStatusCallback status_callback, gpointer user_data)
{
}

static void
modest_tny_simple_folder_store_add_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
}

static void
modest_tny_simple_folder_store_remove_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
}

static void
tny_folder_store_init (TnyFolderStoreIface *klass)
{
	klass->remove_folder_func = modest_tny_simple_folder_store_remove_folder;
	klass->create_folder_func = modest_tny_simple_folder_store_create_folder;
	klass->get_folders_func = modest_tny_simple_folder_store_get_folders;
	klass->get_folders_async_func = modest_tny_simple_folder_store_get_folders_async;
	klass->add_observer_func = modest_tny_simple_folder_store_add_observer;
	klass->remove_observer_func = modest_tny_simple_folder_store_remove_observer;
}


void
modest_tny_simple_folder_store_add_folder (ModestTnySimpleFolderStore *store, 
	TnyFolder *folder)
{
	ModestTnySimpleFolderStorePrivate *priv = 
		TNY_SIMPLE_FOLDER_STORE_GET_PRIVATE (store);
		
	/* Check that it isn't already in the list: */
	GSList *exists = g_slist_find (priv->list_folders, folder);
	if (exists)
		return;
		
	/* Add it: */
	/* The reference is released in finalize: */
	priv->list_folders = g_slist_append (priv->list_folders, folder);
	g_object_ref (folder);
}

