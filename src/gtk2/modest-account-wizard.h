/* modest-account-wizard.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_ACCOUNT_WIZARD_H__
#define __MODEST_ACCOUNT_WIZARD_H__

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_WIZARD             (modest_account_wizard_get_type())
#define MODEST_ACCOUNT_WIZARD(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_WIZARD,ModestAccountWizard))
#define MODEST_ACCOUNT_WIZARD_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_WIZARD,GtkAssistant))
#define MODEST_IS_ACCOUNT_WIZARD(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_WIZARD))
#define MODEST_IS_ACCOUNT_WIZARD_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_WIZARD))
#define MODEST_ACCOUNT_WIZARD_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_WIZARD,ModestAccountWizardClass))

typedef struct _ModestAccountWizard      ModestAccountWizard;
typedef struct _ModestAccountWizardClass ModestAccountWizardClass;

struct _ModestAccountWizard {
	 GtkAssistant parent;
	/* insert public members, if any */
};

struct _ModestAccountWizardClass {
	GtkAssistantClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestAccountWizard* obj); */
};

/* member functions */
GType        modest_account_wizard_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget* */
GtkWidget*   modest_account_wizard_new         (void);

/* fill in other public functions, eg.: */
/* 	void       modest_account_wizard_do_something (ModestAccountWizard *self, const gchar* param); */
/* 	gboolean   modest_account_wizard_has_foo      (ModestAccountWizard *self, gint value); */


G_END_DECLS

#endif /* __MODEST_ACCOUNT_WIZARD_H__ */

