/* modest-ui-account-setup.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_UI_ACCOUNT_SETUP_H__
#define __MODEST_UI_ACCOUNT_SETUP_H__

#include "modest-ui-glade.h"

/**
 * account_settings:
 * @GtkWidget: The widget by which this CALLBACK is called.
 * @gpointer: user data set when the signal handler was connected. A
 * ModestUI is needed here.
 */
void
account_settings (GtkWidget *,
                  gpointer);

#endif /* __MODEST_UI_ACCOUNT_SETUP_H__ */

