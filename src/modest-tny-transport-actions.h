/* modest-tny-transport-actions.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_TRANSPORT_ACTIONS_H__
#define __MODEST_TNY_TRANSPORT_ACTIONS_H__

#include <glib-object.h>
#include <tny-transport-account-iface.h>

G_BEGIN_DECLS

/* standard convenience macros */
#define MODEST_TYPE_TNY_TRANSPORT_ACTIONS             (modest_tny_transport_actions_get_type())
#define MODEST_TNY_TRANSPORT_ACTIONS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_TRANSPORT_ACTIONS,ModestTnyTransportActions))
#define MODEST_TNY_TRANSPORT_ACTIONS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_TRANSPORT_ACTIONS,GObject))
#define MODEST_IS_TNY_TRANSPORT_ACTIONS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_TRANSPORT_ACTIONS))
#define MODEST_IS_TNY_TRANSPORT_ACTIONS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_TRANSPORT_ACTIONS))
#define MODEST_TNY_TRANSPORT_ACTIONS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_TRANSPORT_ACTIONS,ModestTnyTransportActionsClass))

typedef struct _ModestTnyTransportActions      ModestTnyTransportActions;
typedef struct _ModestTnyTransportActionsClass ModestTnyTransportActionsClass;

struct _ModestTnyTransportActions {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestTnyTransportActionsClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestTnyTransportActions* obj); */
};

/* member functions */
GType        modest_tny_transport_actions_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget*, */
/*    otherwise probably a GObject*. */
GObject*    modest_tny_transport_actions_new         (void);


gboolean modest_tny_transport_actions_send_message (ModestTnyTransportActions *self,
						    TnyTransportAccountIface *transport_account,
						    const gchar *from,
						    const gchar *to,
						    const gchar *cc,
						    const gchar *bcc,
						    const gchar *subject,
						    const gchar *body);

G_END_DECLS

#endif /* __MODEST_TNY_TRANSPORT_ACTIONS_H__ */

