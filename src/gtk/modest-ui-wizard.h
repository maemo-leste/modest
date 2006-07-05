/* modest-ui-wizard.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_UI_WIZARD_H__
#define __MODEST_UI_WIZARD_H__

#include "modest-ui-glade.h"

/**
 * wizard_account_dialog:
 * @modest-ui: a ModestUI instance
 *
 * Handle the dialog window acting as account wizard. The wizard allows
 * the creation of identities and server accounts.
 */
void
wizard_account_dialog(ModestUI *modest_ui);

/**
 * new_wizard_account:
 * @GtkWidget: The widget by which this CALLBACK is called.
 * @gpointer: A ModestUI is needed as second argument.
 */
void
new_wizard_account (GtkWidget *,
                    gpointer);

#endif /* __MODEST_UI_WIZARD_H__ */

