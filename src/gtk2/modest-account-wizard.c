/* modest-account-wizard.c */

/* insert (c)/licensing information) */

#include "modest-account-wizard.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                       modest_account_wizard_class_init    (ModestAccountWizardClass *klass);
static void                       modest_account_wizard_init          (ModestAccountWizard *obj);
static void                       modest_account_wizard_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountWizardPrivate ModestAccountWizardPrivate;
struct _ModestAccountWizardPrivate {
	/* my private members go here, eg. */
	/* gboolean frobnicate_mode; */
};
#define MODEST_ACCOUNT_WIZARD_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_ACCOUNT_WIZARD, \
                                                   ModestAccountWizardPrivate))
/* globals */
static GtkAssistantClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_account_wizard_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountWizardClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_account_wizard_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountWizard),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_account_wizard_init,
		};
		my_type = g_type_register_static (GTK_TYPE_ASSISTANT,
		                                  "ModestAccountWizard",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_wizard_class_init (ModestAccountWizardClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_wizard_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountWizardPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}



static GtkWidget*
first_page ()
{
	GtkWidget *label;

	const gchar* txt =
		_("Welcome to the Modest Setup Wizard\n\n"
		  "This wizard will help you to set up email accounts.\n"
		  "Push the 'Next'-button to start");

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(label), txt);

	return label;
}




static void
modest_account_wizard_init (ModestAccountWizard *obj)
{
	GtkWidget *page1;
	int pageno;
	
	page1 = first_page ();
	pageno = gtk_assistant_append_page (GTK_ASSISTANT(obj), page1);

	gtk_assistant_set_page_type (GTK_ASSISTANT(obj),
				     page1,
				     GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title (GTK_ASSISTANT(obj),
				      page1, _("Account Wizard"));

	gtk_assistant_set_page_complete  (GTK_ASSISTANT(obj), page1, TRUE);
	
	/* without this next one, we crash...
	 * http://bugzilla.gnome.org/show_bug.cgi?id=347048
	 */
	//k_assistant_set_current_page (GTK_ASSISTANT(obj), pageno);

	
}

static void
modest_account_wizard_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
}

GtkWidget*
modest_account_wizard_new (void)
{
	return GTK_WIDGET(g_object_new(MODEST_TYPE_ACCOUNT_WIZARD, NULL));
}
