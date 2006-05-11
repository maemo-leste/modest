/* tinymail - Tiny Mail
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with self library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* TODO: Refactory this type to libtinymailui-gtk */

#include <gtk/gtk.h>
#include <tny-password-dialog.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyPasswordDialogPriv TnyPasswordDialogPriv;

struct _TnyPasswordDialogPriv
{
	GtkEntry *pwd_entry;
	GtkLabel *prompt_label;
};

#define TNY_PASSWORD_DIALOG_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_PASSWORD_DIALOG, TnyPasswordDialogPriv))


/**
 * tny_password_dialog_set_prompt:
 * @self: A #TnyPasswordDialog object
 * @prompt: The prompt to set
 * 
 **/
void
tny_password_dialog_set_prompt (TnyPasswordDialog *self, const gchar *prompt)
{
	TnyPasswordDialogPriv *priv = TNY_PASSWORD_DIALOG_GET_PRIVATE (self);

	gtk_label_set_text (priv->prompt_label, prompt);

	return;
}

/**
 * tny_password_dialog_get_password:
 * @self: A #TnyPasswordDialog object
 *
 * Return value: The password (read-only) as typed by the user
 **/
const gchar*
tny_password_dialog_get_password (TnyPasswordDialog *self)
{
	TnyPasswordDialogPriv *priv = TNY_PASSWORD_DIALOG_GET_PRIVATE (self);

	return gtk_entry_get_text (priv->pwd_entry);
}

/**
 * tny_summary_window_new:
 * 
 *
 * Return value: A new #TnyPasswordDialog instance implemented for Gtk+
 **/
TnyPasswordDialog*
tny_password_dialog_new (void)
{
	TnyPasswordDialog *self = g_object_new (TNY_TYPE_PASSWORD_DIALOG, NULL);

	return self;
}

static void
tny_password_dialog_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyPasswordDialog *self = (TnyPasswordDialog *)instance;
	TnyPasswordDialogPriv *priv = TNY_PASSWORD_DIALOG_GET_PRIVATE (self);

	gtk_dialog_add_buttons (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK,
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

	gtk_window_set_title (GTK_WINDOW (self), "Password input");

	/* TODO: Add key icon or something */

	priv->pwd_entry = GTK_ENTRY (gtk_entry_new ());
	priv->prompt_label = GTK_LABEL (gtk_label_new (""));

	gtk_entry_set_visibility (priv->pwd_entry, FALSE);

	gtk_widget_show (GTK_WIDGET (priv->pwd_entry));
	gtk_widget_show (GTK_WIDGET (priv->prompt_label));

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (self)->vbox), 
		GTK_WIDGET (priv->prompt_label), TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (self)->vbox), 
		GTK_WIDGET (priv->pwd_entry), TRUE, TRUE, 0);

	return;
}

static void
tny_password_dialog_finalize (GObject *object)
{
	TnyPasswordDialog *self = (TnyPasswordDialog *)object;	
	
	(*parent_class->finalize) (object);

	return;
}


static void 
tny_password_dialog_class_init (TnyPasswordDialogClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_password_dialog_finalize;

	g_type_class_add_private (object_class, sizeof (TnyPasswordDialogPriv));

	return;
}

GType 
tny_password_dialog_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (TnyPasswordDialogClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_password_dialog_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyPasswordDialog),
		  0,      /* n_preallocs */
		  tny_password_dialog_instance_init    /* instance_init */
		};

		type = g_type_register_static (GTK_TYPE_DIALOG,
			"TnyPasswordDialog",
			&info, 0);

	}

	return type;
}
