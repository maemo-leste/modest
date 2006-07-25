/* modest-account-assistant.c */

/* insert (c)/licensing information) */

#include "modest-account-assistant.h"
#include <string.h>

/* 'private'/'protected' functions */
static void                          modest_account_assistant_class_init    (ModestAccountAssistantClass *klass);
static void                          modest_account_assistant_init          (ModestAccountAssistant *obj);
static void                          modest_account_assistant_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountAssistantPrivate ModestAccountAssistantPrivate;
struct _ModestAccountAssistantPrivate {
	ModestWidgetFactory *factory;
	
	GtkWidget *entry_full_name;
	GtkWidget *entry_email;
	GtkWidget *entry_server_name;

	GtkWidget *pop_imap_config;
	GtkWidget *mbox_maildir_config;
	

};
#define MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                      MODEST_TYPE_ACCOUNT_ASSISTANT, \
                                                      ModestAccountAssistantPrivate))
/* globals */
static GtkAssistantClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_account_assistant_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountAssistantClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_account_assistant_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountAssistant),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_account_assistant_init,
		};
		my_type = g_type_register_static (GTK_TYPE_ASSISTANT,
		                                  "ModestAccountAssistant",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_assistant_class_init (ModestAccountAssistantClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_assistant_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountAssistantPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}



static void
add_intro_page (ModestAccountAssistant *assistant)
{
	GtkWidget *page;
	GtkWidget *label;
	
	page = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new (
		_("Welcome to the account assistant\n\n"
		  "It will help to set up a new e-mail account\n"));
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 6);
	gtk_widget_show_all (page);
	
	gtk_assistant_append_page (GTK_ASSISTANT(assistant), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(assistant), page,
				      _("Modest Account Assistant"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(assistant), page,
				     GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(assistant),
					 page, TRUE);
}


static void
set_current_page_complete (ModestAccountAssistant *self, gboolean complete)
{
	GtkWidget *page;
	gint pageno;

	pageno = gtk_assistant_get_current_page (GTK_ASSISTANT(self));
	page   = gtk_assistant_get_nth_page (GTK_ASSISTANT(self), pageno);

	gtk_assistant_set_page_complete (GTK_ASSISTANT(self), page, complete);
}


static void
identity_page_update_completeness (GtkEditable *editable,
				   ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	const gchar *txt;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	txt = gtk_entry_get_text (GTK_ENTRY(priv->entry_full_name));
	if (!txt || strlen(txt) == 0) {
		set_current_page_complete (self, FALSE);
		return;
	}

	/* FIXME: regexp check for email address */
	txt = gtk_entry_get_text (GTK_ENTRY(priv->entry_email));
	if (!txt || strlen(txt) == 0) {
		set_current_page_complete (self, FALSE);
		return;
	}
	set_current_page_complete (self, TRUE);
}


static void
add_identity_page (ModestAccountAssistant *self)
{
	GtkWidget *page;
	GtkWidget *label;
	GtkWidget *table;

	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	priv->entry_full_name = gtk_entry_new_with_max_length (40);
	priv->entry_email     = gtk_entry_new_with_max_length (40);

	page = gtk_vbox_new (FALSE, 6);

	label = gtk_label_new (
		_("Please enter your name and your e-mail address below.\n\n"));
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 6);
	
	table = gtk_table_new (2,2, FALSE);
	gtk_table_attach_defaults (GTK_TABLE(table),gtk_label_new (_("Full name")),
				   0,1,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),gtk_label_new (_("E-mail address")),
				   0,1,1,2);

	gtk_table_attach_defaults (GTK_TABLE(table),priv->entry_full_name,
				   1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->entry_email,
				   1,2,1,2);

	g_signal_connect (G_OBJECT(priv->entry_full_name), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	g_signal_connect (G_OBJECT(priv->entry_email), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	
	gtk_box_pack_start (GTK_BOX(page), table, FALSE, FALSE, 6);
	gtk_widget_show_all (page);
	
	gtk_assistant_append_page (GTK_ASSISTANT(self), page);
	
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Identity"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, FALSE);
}	

/*

static void
mbox_maildir_configuration (ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	GtkLabel *label;
	GtkWidget *box, *hbox, *chooser;
	
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	box = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (_("Configuration"));

	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 6);

	label = gtk_label_new (_("Path"));
	chooser = gtk_file_chooser_button_new (_("(none)"),
						 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), chooser, FALSE, FALSE, 6);

	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 6);
	
	gtk_box_pack_start (GTK_BOX(page), box, FALSE, FALSE, 6);
	
	table = gtk_table_new (2,2, FALSE);
}

*/

static void
add_receiving_page (ModestAccountAssistant *self)
{
	GtkWidget *page;
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *box;

	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	priv->entry_server_name   = gtk_entry_new_with_max_length (40);
	
	page = gtk_vbox_new (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(page),
			    gtk_label_new (
				    _("Please select among the following options.\n")),
			    FALSE, FALSE, 6);

	box = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(box),
			    gtk_label_new(_("Server type")),
			    FALSE,FALSE,6);
	gtk_box_pack_start (GTK_BOX(box),
			    modest_widget_factory_get_store_combo (priv->factory),
			    FALSE,FALSE,6);
	gtk_box_pack_start (GTK_BOX(page), box, FALSE,FALSE, 6);

	gtk_box_pack_start (GTK_BOX(page), gtk_hseparator_new(), TRUE, TRUE, 6);


	gtk_widget_show_all (page);
	gtk_assistant_append_page (GTK_ASSISTANT(self), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Receiving mail"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_INTRO);

	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, TRUE);

}



static void
add_sending_page (ModestAccountAssistant *self)
{
	GtkWidget *page;
	GtkWidget *label;
	GtkWidget *table;

	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	//priv->combo_server_type   = gtk_entry_new_with_max_length (40);
	priv->entry_server_name   = gtk_entry_new_with_max_length (40);

	page = gtk_vbox_new (FALSE, 6);

	label = gtk_label_new (
		_("Please enter your name and your e-mail address below.\n"));
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 6);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup (GTK_LABEL(label),_("<b>Required</b>"));
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 6);

	gtk_widget_show_all (page);

	gtk_assistant_append_page (GTK_ASSISTANT(self), page);

	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Sending mail"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_CONFIRM);

	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, FALSE);

}





static void
modest_account_assistant_init (ModestAccountAssistant *obj)
{	
	ModestAccountAssistantPrivate *priv;
		
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(obj);

	priv->factory = NULL;	
}

static void
modest_account_assistant_finalize (GObject *obj)
{
	ModestAccountAssistantPrivate *priv;
		
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(obj);

	if (priv->factory) {
		g_object_unref (G_OBJECT(priv->factory));
		priv->factory = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



GtkWidget*
modest_account_assistant_new (ModestWidgetFactory *factory)
{
	GObject *obj;
	ModestAccountAssistant *self;
	ModestAccountAssistantPrivate *priv;
		
	g_return_val_if_fail (factory, NULL);
	
	obj  = g_object_new(MODEST_TYPE_ACCOUNT_ASSISTANT, NULL);
	self = MODEST_ACCOUNT_ASSISTANT(obj);

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	g_object_ref (factory);
	priv->factory = factory;
	
	add_intro_page (self);
	add_identity_page (self); 
	add_receiving_page (self); 
	add_sending_page (self); 

	gtk_assistant_set_current_page (GTK_ASSISTANT(self), 0);
	gtk_window_set_title (GTK_WINDOW(self),
			      _("Modest Account Wizard"));
	gtk_window_set_resizable (GTK_WINDOW(self), TRUE); 	
	gtk_window_set_default_size (GTK_WINDOW(self), 400, 400);

	gtk_window_set_modal (GTK_WINDOW(self), TRUE);

	return GTK_WIDGET(self);
}
