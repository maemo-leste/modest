/* modest-account-settings-dialog-iface.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_ACCOUNT_SETTINGS_DIALOG_H__
#define __MODEST_ACCOUNT_SETTINGS_DIALOG_H__

/* other include files */
#include <glib.h>
#include <glib-object.h>
#include "modest-account-settings.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG             (modest_account_settings_dialog_get_type())
#define MODEST_ACCOUNT_SETTINGS_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG,ModestAccountSettingsDialog))
#define MODEST_IS_ACCOUNT_SETTINGS_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG))
#define MODEST_ACCOUNT_SETTINGS_DIALOG_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE((inst),MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG,ModestAccountSettingsDialogClass))

typedef struct _ModestAccountSettingsDialog      ModestAccountSettingsDialog;
typedef struct _ModestAccountSettingsDialogClass ModestAccountSettingsDialogClass;

struct _ModestAccountSettingsDialogClass {
	GTypeInterface parent;

	/* the 'vtable': declare function pointers here, eg.: */
	void (*load_settings) (ModestAccountSettingsDialog *dialog, ModestAccountSettings *settings);
};

GType    modest_account_settings_dialog_get_type    (void) G_GNUC_CONST;

void     modest_account_settings_dialog_load_settings (ModestAccountSettingsDialog *dialog, 
						       ModestAccountSettings *settings);

G_END_DECLS

#endif /* __MODEST_ACCOUNT_SETTINGS_DIALOG_H__ */

