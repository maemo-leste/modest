/* Copyright (c) 2006, 2008 Nokia Corporation
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


#include "modest-serversecurity-picker.h"
#include <modest-runtime.h>
#include <modest-account-protocol.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

G_DEFINE_TYPE (ModestServersecurityPicker, modest_serversecurity_picker, HILDON_TYPE_PICKER_BUTTON);

#define MODEST_SERVERSECURITY_PICKER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_SERVERSECURITY_PICKER, ModestServersecurityPickerPrivate))

typedef struct _ModestServersecurityPickerPrivate ModestServersecurityPickerPrivate;

struct _ModestServersecurityPickerPrivate
{
	GtkTreeModel *model;
	ModestProtocolType protocol;
};

static void
modest_serversecurity_picker_finalize (GObject *object)
{
	ModestServersecurityPickerPrivate *priv = MODEST_SERVERSECURITY_PICKER_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (modest_serversecurity_picker_parent_class)->finalize (object);
}

static void
modest_serversecurity_picker_class_init (ModestServersecurityPickerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestServersecurityPickerPrivate));

	object_class->finalize = modest_serversecurity_picker_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* a string */
	MODEL_COL_ID = 1 /* an int. */
};

static void
modest_serversecurity_picker_init (ModestServersecurityPicker *self)
{
	ModestServersecurityPickerPrivate *priv = MODEST_SERVERSECURITY_PICKER_GET_PRIVATE (self);

	priv->model = NULL;

}

static gchar *
touch_selector_print_func (HildonTouchSelector *selector, gpointer userdata)
{
	GtkTreeIter iter;
	if (hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &iter)) {
		GtkTreeModel *model;
		GValue value = {0,};
		
		model = hildon_touch_selector_get_model (HILDON_TOUCH_SELECTOR (selector), 0);
		gtk_tree_model_get_value (model, &iter, MODEL_COL_NAME, &value);
		return g_value_dup_string (&value);
	}
	return NULL;
}

ModestServersecurityPicker*
modest_serversecurity_picker_new (HildonSizeType size,
				  HildonButtonArrangement arrangement)
{
	ModestServersecurityPicker *self;
	ModestServersecurityPickerPrivate *priv;
	GtkCellRenderer *renderer;
	GtkWidget *selector;

	self = g_object_new (MODEST_TYPE_SERVERSECURITY_PICKER, 
			     "arrangement", arrangement,
			     "size", size,
			     NULL);
	priv = MODEST_SERVERSECURITY_PICKER_GET_PRIVATE (self);

	/* Create a tree model,
	 * with a string for the name, and an ID for the servertype.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	selector = hildon_touch_selector_new ();
	hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), GTK_TREE_MODEL (priv->model),
					     renderer, "text", MODEL_COL_NAME, NULL);

	hildon_touch_selector_set_model (HILDON_TOUCH_SELECTOR (selector), 0, GTK_TREE_MODEL (priv->model));
	hildon_touch_selector_set_print_func (HILDON_TOUCH_SELECTOR (selector), (HildonTouchSelectorPrintFunc) touch_selector_print_func);

	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));

	/* For theming purpouses. Widget name must end in Button-finger */
	gtk_widget_set_name ((GtkWidget *) self, "ModestServersecurityPickerButton-finger");

	return self;
}

/* Fill the picker box with appropriate choices.
 * #picker: The picker box.
 * @protocol: IMAP or POP.
 */
void modest_serversecurity_picker_fill (ModestServersecurityPicker *picker, ModestProtocolType protocol_type)
{
	ModestServersecurityPickerPrivate *priv;
	ModestProtocol *protocol;
	GtkWidget *selector;

	priv = MODEST_SERVERSECURITY_PICKER_GET_PRIVATE (picker);
	priv->protocol = protocol_type; /* Remembered for later. */
	protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (),
								  protocol_type);
	
	/* Remove any existing rows: */
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);
	
	GtkTreeIter iter;
	gtk_list_store_append (liststore, &iter);
	/* TODO: This logical ID is not in the .po file: */
	gtk_list_store_set (liststore, &iter, MODEL_COL_ID, (gint) MODEST_PROTOCOLS_CONNECTION_NONE, MODEL_COL_NAME, _("mcen_fi_advsetup_other_security_none"), -1);
	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (picker)));
	hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, &iter, TRUE);
	hildon_button_set_value (HILDON_BUTTON (picker), 
				 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
	
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_ID, (gint)MODEST_PROTOCOLS_CONNECTION_TLS, MODEL_COL_NAME, _("mcen_fi_advsetup_other_security_normal"), -1);
	
	/* Add security choices with protocol-specific names, as in the UI spec:
	 * (Note: Changing the title seems pointless. murrayc) */
	gchar *protocol_name = modest_protocol_get_translation (protocol, MODEST_PROTOCOL_TRANSLATION_SSL_PROTO_NAME);
	if (protocol_name) {
		gtk_list_store_append (liststore, &iter);
		gtk_list_store_set (liststore, &iter, MODEL_COL_ID, (gint)MODEST_PROTOCOLS_CONNECTION_SSL, MODEL_COL_NAME, protocol_name, -1);
		g_free (protocol_name);
	} else {
		/* generic fallback */
		gtk_list_store_append (liststore, &iter);
		gtk_list_store_set (liststore, &iter, MODEL_COL_ID, (gint)MODEST_PROTOCOLS_CONNECTION_SSL, MODEL_COL_NAME, _("mcen_fi_advsetup_other_security_ssl"), -1);
	}
}

static gint get_port_for_security (ModestProtocolType protocol_type, ModestProtocolType security_type)
{
	/* See the UI spec, section Email Wizards, Incoming Details [MSG-WIZ001]: */
	gint result = 0;
	ModestProtocol *protocol, *security;
	ModestProtocolRegistry *protocol_registry;
	gboolean use_alternate_port;

	protocol_registry = modest_runtime_get_protocol_registry ();
	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry, protocol_type);
	security = modest_protocol_registry_get_protocol_by_type (protocol_registry, security_type);

	g_return_val_if_fail ((security != NULL && protocol != NULL), 0);

	use_alternate_port = modest_protocol_registry_protocol_type_has_tag (protocol_registry, security_type,
									     MODEST_PROTOCOL_REGISTRY_USE_ALTERNATE_PORT);

	/* Get the default port number for this protocol with this security: */
	if (use_alternate_port) {
		result = modest_account_protocol_get_alternate_port (MODEST_ACCOUNT_PROTOCOL (protocol));
	} else {
		result = modest_account_protocol_get_port (MODEST_ACCOUNT_PROTOCOL (protocol));
	}

	return result;
}

/**
 * Returns the selected serversecurity, 
 * or MODEST_PROTOCOLS_CONNECTION_NONE if no serversecurity was selected.
 */
ModestProtocolType
modest_serversecurity_picker_get_active_serversecurity (ModestServersecurityPicker *picker)
{
	GtkTreeIter active;
	gboolean found;
	GtkWidget *selector;

	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (picker)));

	found = hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &active);
	if (found) {
		ModestServersecurityPickerPrivate *priv = MODEST_SERVERSECURITY_PICKER_GET_PRIVATE (picker);

		ModestProtocolType serversecurity = MODEST_PROTOCOLS_CONNECTION_NONE;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_ID, &serversecurity, -1);
		return serversecurity;	
	}

	return MODEST_PROTOCOLS_CONNECTION_NONE; /* Failed. */
}

/**
 * Returns the default port to be used for the selected serversecurity, 
 * or 0 if no serversecurity was selected.
 */
gint
modest_serversecurity_picker_get_active_serversecurity_port (ModestServersecurityPicker *picker)
{
	ModestServersecurityPickerPrivate *priv = MODEST_SERVERSECURITY_PICKER_GET_PRIVATE (picker);
	
	ModestProtocolType security = modest_serversecurity_picker_get_active_serversecurity 
		(picker);
	return get_port_for_security (priv->protocol, security);
}
	
/* This allows us to pass more than one piece of data to the signal handler,
 * and get a result: */
typedef struct 
{
		ModestServersecurityPicker* self;
		ModestProtocolType id;
		gboolean found;
} ForEachData;

static gboolean
on_model_foreach_select_id(GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	ForEachData *state = (ForEachData*)(user_data);
	
	/* Select the item if it has the matching ID: */
	ModestProtocolType id = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	gtk_tree_model_get (model, iter, MODEL_COL_ID, &id, -1); 
	if(id == state->id) {
		GtkWidget *selector;
		selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (state->self)));
		hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, iter, TRUE);
		hildon_button_set_value (HILDON_BUTTON (state->self),
					 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
		
		state->found = TRUE;
		return TRUE; /* Stop walking the tree. */
	}
	
	return FALSE; /* Keep walking the tree. */
}

/**
 * Selects the specified serversecurity, 
 * or MODEST_PROTOCOLS_CONNECTION_NONE if no serversecurity was selected.
 */
gboolean
modest_serversecurity_picker_set_active_serversecurity (ModestServersecurityPicker *picker,
							ModestProtocolType serversecurity)
{
	ModestServersecurityPickerPrivate *priv = MODEST_SERVERSECURITY_PICKER_GET_PRIVATE (picker);
	
	/* Create a state instance so we can send two items of data to the signal handler: */
	ForEachData *state = g_new0 (ForEachData, 1);
	state->self = picker;
	state->id = serversecurity;
	state->found = FALSE;
	
	/* Look at each item, and select the one with the correct ID: */
	gtk_tree_model_foreach (priv->model, &on_model_foreach_select_id, state);

	const gboolean result = state->found;
	
	/* Free the state instance: */
	g_free(state);
	
	return result;
}

