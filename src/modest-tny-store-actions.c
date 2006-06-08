/* modest-tny-store-actions.c */

/* insert (c)/licensing information) */

#include <tny-msg.h>
#include <tny-msg-iface.h>			
#include <tny-msg-header.h>
#include <tny-msg-header-iface.h>
#include <tny-account-iface.h>	
#include <tny-account-store-iface.h>
#include <tny-store-account-iface.h>	
#include <tny-store-account.h>
#include <tny-stream-camel.h>
#include <string.h>
#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>


#include <glib.h>
#include "modest-tny-store-actions.h"


/* 'private'/'protected' functions */
static void    modest_tny_store_actions_class_init   (ModestTnyStoreActionsClass *klass);
static void    modest_tny_store_actions_init         (ModestTnyStoreActions *obj);
static void    modest_tny_store_actions_finalize     (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyStoreActionsPrivate ModestTnyStoreActionsPrivate;

struct _ModestTnyStoreActionsPrivate {
	gboolean active;
};

#define MODEST_TNY_STORE_ACTIONS_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                          MODEST_TYPE_TNY_STORE_ACTIONS, \
                                                          ModestTnyStoreActionsPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

void
modest_tny_store_actions_update_folders (ModestTnyStoreActions *self,
					   TnyStoreAccountIface *storage_account)
{
	const TnyListIface* folders;
	TnyIteratorIface* ifolders;
	gpointer *cur_folder;
#if 0	
	folders = tny_store_account_iface_get_folders (storage_account, 
												  TNY_STORE_ACCOUNT_FOLDER_TYPE_SUBSCRIBED);
	
	ifolders = tny_list_iface_create_iterator (folders);
	
	for (cur_folder = tny_iterator_iface_first (ifolders); 
		              tny_iterator_iface_has_next (ifolders); 
	                  tny_iterator_iface_next (ifolders))
	{
		cur_folder = tny_iterator_iface_current (ifolders);
//		tny_msg_folder_iface_refresh (cur_folder);
	}
#endif
}


GType
modest_tny_store_actions_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyStoreActionsClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_store_actions_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyStoreActions),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_store_actions_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestTnyStoreActions",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_tny_store_actions_class_init (ModestTnyStoreActionsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_store_actions_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyStoreActionsPrivate));
	
	klass->update_folders = modest_tny_store_actions_update_folders;
	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_tny_store_actions_init (ModestTnyStoreActions *obj)
{
/* uncomment the following if you init any of the private data */
/* 	ModestTnyStoreActionsPrivate *priv = MODEST_TNY_STORE_ACTIONS_GET_PRIVATE(obj); */

/* 	initialize this object, eg.: */
/* 	priv->frobnicate_mode = FALSE; */
}

static void
modest_tny_store_actions_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
}

GObject*
modest_tny_store_actions_new (void)
{
	return G_OBJECT(g_object_new(MODEST_TYPE_TNY_STORE_ACTIONS, NULL));
}

