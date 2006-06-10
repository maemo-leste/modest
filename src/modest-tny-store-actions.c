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


void
modest_tny_store_actions_update_folders (TnyStoreAccountIface *storage_account)
{

	const TnyListIface* folders;
	TnyIteratorIface* ifolders;
	const TnyMsgFolderIface *cur_folder;

	folders = tny_store_account_iface_get_folders (storage_account, 
						       TNY_STORE_ACCOUNT_FOLDER_TYPE_SUBSCRIBED);
	
	ifolders = tny_list_iface_create_iterator (folders);
	
	for (cur_folder = TNY_MSG_FOLDER_IFACE(tny_iterator_iface_first (ifolders)); 
	     tny_iterator_iface_has_next (ifolders); 
	     tny_iterator_iface_next (ifolders)) {
		
		cur_folder = TNY_MSG_FOLDER_IFACE(tny_iterator_iface_current (ifolders));
		tny_msg_folder_iface_refresh (cur_folder);
	}
	
	g_object_unref (ifolders);
}

