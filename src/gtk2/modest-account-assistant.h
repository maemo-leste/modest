/* modest-account-assistant.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_ACCOUNT_ASSISTANT_H__
#define __MODEST_ACCOUNT_ASSISTANT_H__

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <modest-widget-factory.h>

G_BEGIN_DECLS

/* standard convenience macros */
#define MODEST_TYPE_ACCOUNT_ASSISTANT             (modest_account_assistant_get_type())
#define MODEST_ACCOUNT_ASSISTANT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_ASSISTANT,ModestAccountAssistant))
#define MODEST_ACCOUNT_ASSISTANT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_ASSISTANT,ModestAccountAssistantClass))
#define MODEST_IS_ACCOUNT_ASSISTANT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_ASSISTANT))
#define MODEST_IS_ACCOUNT_ASSISTANT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_ASSISTANT))
#define MODEST_ACCOUNT_ASSISTANT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_ASSISTANT,ModestAccountAssistantClass))

typedef struct _ModestAccountAssistant      ModestAccountAssistant;
typedef struct _ModestAccountAssistantClass ModestAccountAssistantClass;

struct _ModestAccountAssistant {
	 GtkAssistant parent;
	/* insert public members, if any */
};

struct _ModestAccountAssistantClass {
	GtkAssistantClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestAccountAssistant* obj); */
};

/* member functions */
GType        modest_account_assistant_get_type    (void) G_GNUC_CONST;

GtkWidget*    modest_account_assistant_new        (ModestWidgetFactory *factory);


G_END_DECLS

#endif /* __MODEST_ACCOUNT_ASSISTANT_H__ */

