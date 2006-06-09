/* modest-ui-glade.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_UI_GLADE_H__
#define __MODEST_UI_GLADE_H__

#include "../modest-ui.h"
#include "../modest-identity-mgr.h"
#include "../modest-window-mgr.h"
#include "../modest-tny-account-store.h"

#define MODEST_GLADE          PREFIX "/share/modest/glade/modest.glade"
#define MODEST_GLADE_MAIN_WIN "main"
#define MODEST_GLADE_EDIT_WIN "new_mail"

typedef struct _ModestUIPrivate ModestUIPrivate;
struct _ModestUIPrivate {

	ModestConf           *modest_conf;
	ModestAccountMgr     *modest_acc_mgr;
	ModestIdentityMgr    *modest_id_mgr;
	ModestWindowMgr      *modest_window_mgr;
	TnyAccountStoreIface *account_store;
	GtkWidget            *folder_view;
	GtkWidget            *header_view;

	GtkWindow            *main_window;
	GladeXML             *glade_xml;


};

#define MODEST_UI_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MODEST_TYPE_UI, \
                                       ModestUIPrivate))

#endif /* __MODEST_UI_GLADE_H__ */
