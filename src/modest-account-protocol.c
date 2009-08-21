/* Copyright (c) 2008, Nokia Corporation
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

#include <tny-simple-list.h>
#include "modest-account-protocol.h"
#include "modest-account-mgr-helpers.h"
#include "widgets/modest-default-account-settings-dialog.h"
#include "modest-runtime.h"
#include "modest-marshal.h"

enum {
	PROP_0,
	PROP_PORT,
	PROP_ALTERNATE_PORT,
	PROP_ACCOUNT_G_TYPE,
};

typedef struct _ModestAccountProtocolPrivate ModestAccountProtocolPrivate;
struct _ModestAccountProtocolPrivate {
	guint port;
	guint alternate_port;
	TnyList *account_options;
	GHashTable *custom_auth_mechs;
	GType account_g_type;

	GHashTable *account_dialogs;
};

/* 'private'/'protected' functions */
static void   modest_account_protocol_class_init (ModestAccountProtocolClass *klass);
static void   modest_account_protocol_finalize   (GObject *obj);
static void   modest_account_protocol_get_property (GObject *obj,
					    guint property_id,
					    GValue *value,
					    GParamSpec *pspec);
static void   modest_account_protocol_set_property (GObject *obj,
					    guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void   modest_account_protocol_instance_init (ModestAccountProtocol *obj);

#define MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
										 MODEST_TYPE_ACCOUNT_PROTOCOL, \
										 ModestAccountProtocolPrivate))

static TnyAccount *modest_account_protocol_create_account_default (ModestAccountProtocol *self);

static ModestAccountSettingsDialog *modest_account_protocol_create_account_settings_dialog_default (ModestAccountProtocol *self);

static ModestPairList* modest_account_protocol_get_easysetupwizard_tabs_default (ModestAccountProtocol *self);

static void modest_account_protocol_save_settings_default (ModestAccountProtocol *self, 
							   ModestAccountSettingsDialog *dialog,
							   ModestAccountSettings *settings);

static void modest_account_protocol_save_wizard_settings_default (ModestAccountProtocol *self, 
								  GList *wizard_pages,
								  ModestAccountSettings *settings);

static ModestWizardDialogResponseOverrideFunc 
modest_account_protocol_get_wizard_response_override_default (ModestAccountProtocol *self);

static void modest_account_protocol_check_support_default (ModestAccountProtocol *self,
							   ModestAccountProtocolCheckSupportFunc func,
							   gpointer userdata);
static void modest_account_protocol_cancel_check_support_default (ModestAccountProtocol *self);
static void modest_account_protocol_wizard_finished_default (ModestAccountProtocol *self);
static gboolean modest_account_protocol_is_supported_default (ModestAccountProtocol *self);
static gchar *modest_account_protocol_get_from_default (ModestAccountProtocol *self,
							const gchar *account_id,
							const gchar *mailbox);
static ModestPairList *modest_account_protocol_get_from_list_default (ModestAccountProtocol *self,
								      const gchar *account_id);
static gchar *modest_account_protocol_get_signature_default (ModestAccountProtocol *self,
							     const gchar *account_id,
							     const gchar *mailbox,
							     gboolean *has_signature);
static const GdkPixbuf *modest_account_protocol_get_icon_default (ModestAccountProtocol *self,
								  ModestAccountProtocolIconType icon_type, 
								  GObject *object, 
								  guint icon_size);

static gchar *modest_account_protocol_get_service_name_default (ModestAccountProtocol *self,
								const gchar *account_id,
								const gchar *mailbox);

static const GdkPixbuf *modest_account_protocol_get_service_icon_default (ModestAccountProtocol *self,
									  const gchar *account_id,
									  const gchar *mailbox,
									  guint icon_size);
static void modest_account_protocol_save_remote_draft_default (ModestAccountProtocol *self,
							       const gchar *account_id,
							       TnyMsg *new_msg,
							       TnyMsg *old_msg,
							       ModestAccountProtocolSaveRemoteDraftCallback callback,
							       gpointer userdata);

/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_account_protocol_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountProtocolClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) modest_account_protocol_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ModestAccountProtocol),
			0,      /* n_preallocs */
			(GInstanceInitFunc) modest_account_protocol_instance_init,
			NULL
		};

		my_type = g_type_register_static (MODEST_TYPE_PROTOCOL,
						  "ModestAccountProtocol",
						  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_protocol_class_init (ModestAccountProtocolClass *klass)
{
	GObjectClass *object_class;
	ModestAccountProtocolClass *account_class;

	object_class = (GObjectClass *) klass;
	account_class = MODEST_ACCOUNT_PROTOCOL_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = modest_account_protocol_finalize;
	object_class->set_property = modest_account_protocol_set_property;
	object_class->get_property = modest_account_protocol_get_property;

	g_object_class_install_property (object_class,
					 PROP_PORT,
					 g_param_spec_uint ("port",
							   _("Standard port"),
							   _("The standard port for the protocol"),
							   0, G_MAXINT, 0,
							   G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_ALTERNATE_PORT,
					 g_param_spec_uint ("alternate-port",
							   _("Alternate port"),
							   _("The alternate port for the protocol (usually used in SSL)"),
							   0, G_MAXINT, 0,
							   G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_ACCOUNT_G_TYPE,
					 g_param_spec_gtype ("account-g-type",
							     _("Account factory GType"),
							     _("Account factory GType used for creating new instances."),
							     TNY_TYPE_ACCOUNT,
							     G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_type_class_add_private (object_class,
				  sizeof(ModestAccountProtocolPrivate));

	/* Virtual methods */
	account_class->create_account_settings_dialog = 
		modest_account_protocol_create_account_settings_dialog_default;
	account_class->get_easysetupwizard_tabs = 
		modest_account_protocol_get_easysetupwizard_tabs_default;
	account_class->save_settings = 
		modest_account_protocol_save_settings_default;
	account_class->save_wizard_settings = 
		modest_account_protocol_save_wizard_settings_default;
	account_class->create_account =
		modest_account_protocol_create_account_default;
	account_class->get_wizard_response_override =
		modest_account_protocol_get_wizard_response_override_default;
	account_class->is_supported =
		modest_account_protocol_is_supported_default;
	account_class->check_support =
		modest_account_protocol_check_support_default;
	account_class->cancel_check_support =
		modest_account_protocol_cancel_check_support_default;
	account_class->wizard_finished =
		modest_account_protocol_wizard_finished_default;
	account_class->get_from =
		modest_account_protocol_get_from_default;
	account_class->get_from_list =
		modest_account_protocol_get_from_list_default;
	account_class->get_signature =
		modest_account_protocol_get_signature_default;
	account_class->get_icon =
		modest_account_protocol_get_icon_default;
	account_class->get_service_name =
		modest_account_protocol_get_service_name_default;
	account_class->get_service_icon =
		modest_account_protocol_get_service_icon_default;
	account_class->save_remote_draft =
		modest_account_protocol_save_remote_draft_default;

}

static void
modest_account_protocol_instance_init (ModestAccountProtocol *obj)
{
	ModestAccountProtocolPrivate *priv;

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (obj);

	priv->port = 0;
	priv->alternate_port = 0;
	priv->account_g_type = 0;
	priv->account_options = tny_simple_list_new ();
	priv->custom_auth_mechs = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

	priv->account_dialogs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static gboolean
remove_account (const gchar *account_name, GObject *account, GObject *account_to_remove)
{
	return (account == account_to_remove);
}

static void
account_dialog_weak_handler (ModestAccountProtocol *self, GObject *where_the_object_was)
{
	ModestAccountProtocolPrivate *priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);

	g_hash_table_foreach_remove (priv->account_dialogs, (GHRFunc) remove_account, where_the_object_was);
}

static gboolean
dialogs_remove (const gchar *account_name, GObject *account_dialog, ModestAccountProtocol *self)
{
	g_object_weak_unref (account_dialog, (GWeakNotify) account_dialog_weak_handler, self);

	return TRUE;
}

static void   
modest_account_protocol_finalize   (GObject *obj)
{
	ModestAccountProtocol *protocol = MODEST_ACCOUNT_PROTOCOL (obj);
	ModestAccountProtocolPrivate *priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (protocol);

	if (priv->account_dialogs) {
		g_hash_table_foreach_remove (priv->account_dialogs, (GHRFunc) dialogs_remove, obj);
		g_hash_table_destroy (priv->account_dialogs);
	}

	if (priv->account_options)
		g_object_unref (priv->account_options);
	priv->account_options = NULL;

	if (priv->custom_auth_mechs)
		g_hash_table_destroy (priv->custom_auth_mechs);
	priv->custom_auth_mechs = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void   
modest_account_protocol_get_property (GObject *obj,
				      guint property_id,
				      GValue *value,
				      GParamSpec *pspec)
{
	ModestAccountProtocol *protocol = MODEST_ACCOUNT_PROTOCOL (obj);
	ModestAccountProtocolPrivate *priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (protocol);

	switch (property_id) {
	case PROP_PORT:
		g_value_set_uint (value, priv->port);
		break;
	case PROP_ALTERNATE_PORT:
		g_value_set_uint (value, priv->alternate_port);
		break;
	case PROP_ACCOUNT_G_TYPE:
		g_value_set_gtype (value, priv->account_g_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}

static void   
modest_account_protocol_set_property (GObject *obj,
				      guint property_id,
				      const GValue *value,
				      GParamSpec *pspec)
{
	ModestAccountProtocol *protocol = MODEST_ACCOUNT_PROTOCOL (obj);

	switch (property_id) {
	case PROP_PORT:
		modest_account_protocol_set_port (protocol, g_value_get_uint (value));
		break;
	case PROP_ALTERNATE_PORT:
		modest_account_protocol_set_alternate_port (protocol, g_value_get_uint (value));
		break;
	case PROP_ACCOUNT_G_TYPE:
		modest_account_protocol_set_account_g_type (protocol, g_value_get_gtype (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
	}

}


ModestProtocol*
modest_account_protocol_new (const gchar *name, const gchar *display_name,
			     guint port, guint alternate_port,
			     GType account_g_type)
{
	return g_object_new (MODEST_TYPE_ACCOUNT_PROTOCOL, 
			     "display-name", display_name, "name", name, 
			     "port", port, "alternate-port", alternate_port,
			     "account-g-type", account_g_type,
			     NULL);
}

guint
modest_account_protocol_get_port (ModestAccountProtocol *self)
{
	ModestAccountProtocolPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), 0);

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	
	return priv->port;
}

void         
modest_account_protocol_set_port (ModestAccountProtocol *self,
				  guint port)
{
	ModestAccountProtocolPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	priv->port = port;
}


guint
modest_account_protocol_get_alternate_port (ModestAccountProtocol *self)
{
	ModestAccountProtocolPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), 0);

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	
	return priv->alternate_port;
}

void         
modest_account_protocol_set_alternate_port (ModestAccountProtocol *self,
					    guint alternate_port)
{
	ModestAccountProtocolPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	priv->alternate_port = alternate_port;
}

GType
modest_account_protocol_get_account_g_type (ModestAccountProtocol *self)
{
	ModestAccountProtocolPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), 0);

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	
	return priv->account_g_type;
}

TnyList *
modest_account_protocol_get_account_options (ModestAccountProtocol *self)
{
	TnyList *result;
	ModestAccountProtocolPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);
	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	result = tny_list_copy (priv->account_options);

	return result;
}

void
modest_account_protocol_set_account_options (ModestAccountProtocol *self,
					     TnyList *list)
{
	ModestAccountProtocolPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));
	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	if (priv->account_options) {
		g_object_unref (priv->account_options);
		priv->account_options = NULL;
	}
	priv->account_options = tny_list_copy (list);
}

gboolean
modest_account_protocol_has_custom_secure_auth_mech (ModestAccountProtocol *self, 
						     ModestProtocolType auth_protocol_type)
{
	ModestAccountProtocolPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), FALSE);
	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	return g_hash_table_lookup_extended (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type), NULL, NULL);
}

const gchar *
modest_account_protocol_get_custom_secure_auth_mech (ModestAccountProtocol *self, 
						     ModestProtocolType auth_protocol_type)
{
	ModestAccountProtocolPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);
	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	return (const gchar *) g_hash_table_lookup (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type));
}

void
modest_account_protocol_set_custom_secure_auth_mech (ModestAccountProtocol *self, ModestProtocolType auth_protocol_type, const gchar *secure_auth_mech)
{
	ModestAccountProtocolPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));
	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	g_hash_table_replace (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type), g_strdup (secure_auth_mech));
}

void
modest_account_protocol_unset_custom_secure_auth_mech (ModestAccountProtocol *self, ModestProtocolType auth_protocol_type)
{
	ModestAccountProtocolPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));
	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);	

	g_hash_table_remove (priv->custom_auth_mechs, GINT_TO_POINTER (auth_protocol_type));
}


void         
modest_account_protocol_set_account_g_type (ModestAccountProtocol *self,
					    GType account_g_type)
{
	ModestAccountProtocolPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	priv->account_g_type = account_g_type;
}

static TnyAccount *
modest_account_protocol_create_account_default (ModestAccountProtocol *self)
{
	ModestAccountProtocolPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	if (priv->account_g_type > 0) {
		return g_object_new (priv->account_g_type, NULL);
	} else {
		return NULL;
	}
}

TnyAccount *
modest_account_protocol_create_account (ModestAccountProtocol *self)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->create_account (self);	
}

/* This is a template method for getting the account settings
   dialog. It calls create_account_settings that must be implemented
   by subclasses and then perform several common operations with the
   dialog */
ModestAccountSettingsDialog *
modest_account_protocol_get_account_settings_dialog (ModestAccountProtocol *self,
						     const gchar *account_name)
{
	ModestAccountSettingsDialog *dialog;
	ModestAccountSettings *settings;
	ModestAccountProtocolPrivate *priv;

	priv = MODEST_ACCOUNT_PROTOCOL_GET_PRIVATE (self);
	dialog = g_hash_table_lookup (priv->account_dialogs, account_name);

	if (dialog == NULL) {

		dialog = MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->create_account_settings_dialog (self);
	
		/* Load settings */
		settings = modest_account_mgr_load_account_settings (modest_runtime_get_account_mgr (), 
								     account_name);
		modest_account_settings_dialog_load_settings (dialog, settings);
	
		/* Close dialog on response */
		g_signal_connect_swapped (dialog,
					  "response",
					  G_CALLBACK (gtk_widget_destroy),
					  dialog);

		g_hash_table_insert (priv->account_dialogs, g_strdup (account_name), dialog);
		g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) account_dialog_weak_handler, self);
	}

	return dialog;
}

ModestPairList*
modest_account_protocol_get_easysetupwizard_tabs (ModestAccountProtocol *self)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_easysetupwizard_tabs (self);
}


void 
modest_account_protocol_save_settings (ModestAccountProtocol *self, 
				       ModestAccountSettingsDialog *dialog,
				       ModestAccountSettings *settings)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->save_settings (self, dialog, settings);
}

void 
modest_account_protocol_save_wizard_settings (ModestAccountProtocol *self, 
					      GList *wizard_pages,
					      ModestAccountSettings *settings)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->save_wizard_settings (self, wizard_pages, settings);
}

/* Default implementations */
static ModestAccountSettingsDialog *
modest_account_protocol_create_account_settings_dialog_default (ModestAccountProtocol *self)
{
	return modest_default_account_settings_dialog_new ();
}

static ModestPairList*
modest_account_protocol_get_easysetupwizard_tabs_default (ModestAccountProtocol *self)
{
	g_warning ("You must implement get_easysetupwizard_tabs");
	return NULL;
}

static void 
modest_account_protocol_save_settings_default (ModestAccountProtocol *self, 
					       ModestAccountSettingsDialog *dialog,
					       ModestAccountSettings *settings)
{
	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));
	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS_DIALOG (dialog));
	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	g_warning ("You must implement save_settings");
}

static void 
modest_account_protocol_save_wizard_settings_default (ModestAccountProtocol *self, 
						      GList *wizard_pages,
						      ModestAccountSettings *settings)
{
	g_return_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self));
	g_return_if_fail (wizard_pages);
	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	g_warning ("You must implement save_wizard_settings");
}

static ModestWizardDialogResponseOverrideFunc
modest_account_protocol_get_wizard_response_override_default (ModestAccountProtocol *self)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return NULL;
}

ModestWizardDialogResponseOverrideFunc
modest_account_protocol_get_wizard_response_override (ModestAccountProtocol *self)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_wizard_response_override (self);	
}

static gboolean
modest_account_protocol_is_supported_default (ModestAccountProtocol *self)
{
	return TRUE;
}

gboolean
modest_account_protocol_is_supported (ModestAccountProtocol *self)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->is_supported (self);
}

static void
modest_account_protocol_check_support_default (ModestAccountProtocol *self,
					       ModestAccountProtocolCheckSupportFunc func,
					       gpointer userdata)
{
	if (func)
		func (self, TRUE, userdata);
}

void
modest_account_protocol_check_support (ModestAccountProtocol *self,
				       ModestAccountProtocolCheckSupportFunc func,
				       gpointer userdata)
{
	MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->check_support (self, func, userdata);
}

static void
modest_account_protocol_cancel_check_support_default (ModestAccountProtocol *self)
{
	return;
}

void
modest_account_protocol_cancel_check_support (ModestAccountProtocol *self)
{
	MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->cancel_check_support (self);
}

static void
modest_account_protocol_wizard_finished_default (ModestAccountProtocol *self)
{
	return;
}

void
modest_account_protocol_wizard_finished (ModestAccountProtocol *self)
{
	MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->wizard_finished (self);
}

gchar *
modest_account_protocol_get_from (ModestAccountProtocol *self,
				  const gchar *account_id,
				  const gchar *mailbox)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_from (self, account_id, mailbox);
}
static gchar *
modest_account_protocol_get_from_default (ModestAccountProtocol *self,
					  const gchar *account_id,
					  const gchar *mailbox)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return NULL;
}

ModestPairList *
modest_account_protocol_get_from_list (ModestAccountProtocol *self,
				       const gchar *account_id)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_from_list (self, account_id);
}
static ModestPairList *
modest_account_protocol_get_from_list_default (ModestAccountProtocol *self,
					       const gchar *account_id)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return NULL;
}

gchar *
modest_account_protocol_get_signature (ModestAccountProtocol *self,
				       const gchar *account_id,
				       const gchar *mailbox,
				       gboolean *has_signature)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_signature (self, account_id, mailbox, has_signature);
}

static gchar *
modest_account_protocol_get_signature_default (ModestAccountProtocol *self,
					       const gchar *account_id,
					       const gchar *mailbox,
					       gboolean *has_signature)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);
	if (has_signature)
		*has_signature = FALSE;

	return NULL;
}

const GdkPixbuf*
modest_account_protocol_get_icon (ModestAccountProtocol *self,
				  ModestAccountProtocolIconType icon_type,
				  GObject *object,
				  guint icon_size)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_icon (self, icon_type, object, icon_size);
}

static const GdkPixbuf * 
modest_account_protocol_get_icon_default (ModestAccountProtocol *self, ModestAccountProtocolIconType icon_type, 
					  GObject *object, guint icon_size)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return NULL;
}

gchar *
modest_account_protocol_get_service_name (ModestAccountProtocol *self,
					  const gchar *account_id,
					  const gchar *mailbox)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_service_name (self, account_id, mailbox);
}

static gchar *
modest_account_protocol_get_service_name_default (ModestAccountProtocol *self,
						  const gchar *account_id,
						  const gchar *mailbox)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return NULL;
}

const GdkPixbuf *
modest_account_protocol_get_service_icon (ModestAccountProtocol *self,
					  const gchar *account_id,
					  const gchar *mailbox,
					  guint icon_size)
{
	return MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->get_service_icon (self, account_id, mailbox, icon_size);
}

static const GdkPixbuf *
modest_account_protocol_get_service_icon_default (ModestAccountProtocol *self,
						  const gchar *account_id,
						  const gchar *mailbox,
						  guint icon_size)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNT_PROTOCOL (self), NULL);

	return NULL;
}

void
modest_account_protocol_save_remote_draft (ModestAccountProtocol *self,
					   const gchar *account_id,
					   TnyMsg *new_msg,
					   TnyMsg *old_msg,
					   ModestAccountProtocolSaveRemoteDraftCallback callback,
					   gpointer userdata)
{
	MODEST_ACCOUNT_PROTOCOL_GET_CLASS (self)->save_remote_draft (self, account_id, 
								     new_msg, old_msg,
								     callback, userdata);
}

static void
modest_account_protocol_save_remote_draft_default (ModestAccountProtocol *self,
						   const gchar *account_id,
						   TnyMsg *new_msg,
						   TnyMsg *old_msg,
						   ModestAccountProtocolSaveRemoteDraftCallback callback,
						   gpointer userdata)
{
	if (callback) {
		callback (self, NULL, account_id, NULL, new_msg, old_msg, userdata);
	}
}

