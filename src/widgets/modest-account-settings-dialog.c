/* modest-account-settings-dialog-iface.c */
/* insert (c)/licensing information) */

#include "modest-account-settings-dialog.h"

static void modest_account_settings_dialog_base_init (gpointer g_class);

void
modest_account_settings_dialog_load_settings (ModestAccountSettingsDialog *self,
					    ModestAccountSettings *settings)
{
	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS_DIALOG (self));
	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	return MODEST_ACCOUNT_SETTINGS_DIALOG_GET_IFACE(self)->load_settings (self, settings);
}

static void
modest_account_settings_dialog_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
	/* create interface signals here */
		initialized = TRUE;
	}
}
GType
modest_account_settings_dialog_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountSettingsDialogClass),
			modest_account_settings_dialog_base_init,		/* base init */
			NULL,		/* base finalize */
			NULL,		/* class_init */
			NULL,		/* class finalize */
			NULL,		/* class data */
			0,
			0,		/* n_preallocs */
			NULL,		/* instance init */
		};
		my_type = g_type_register_static (G_TYPE_INTERFACE,
		                                  "ModestAccountSettingsDialog",
		                                  &my_info, 0);
		g_type_interface_add_prerequisite (my_type, G_TYPE_OBJECT);
	}
	return my_type;
}

