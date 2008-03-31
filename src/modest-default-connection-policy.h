#ifndef MODEST_DEFAULT_CONNECTION_POLICY_H
#define MODEST_DEFAULT_CONNECTION_POLICY_H

#include <glib-object.h>

#include <tny-connection-policy.h>

G_BEGIN_DECLS

#define MODEST_TYPE_DEFAULT_CONNECTION_POLICY             (modest_default_connection_policy_get_type ())
#define MODEST_DEFAULT_CONNECTION_POLICY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_DEFAULT_CONNECTION_POLICY, ModestDefaultConnectionPolicy))
#define MODEST_DEFAULT_CONNECTION_POLICY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MODEST_TYPE_DEFAULT_CONNECTION_POLICY, ModestDefaultConnectionPolicyClass))
#define MODEST_IS_DEFAULT_CONNECTION_POLICY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_DEFAULT_CONNECTION_POLICY))
#define MODEST_IS_DEFAULT_CONNECTION_POLICY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MODEST_TYPE_DEFAULT_CONNECTION_POLICY))
#define MODEST_DEFAULT_CONNECTION_POLICY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MODEST_TYPE_DEFAULT_CONNECTION_POLICY, ModestDefaultConnectionPolicyClass))

typedef struct _ModestDefaultConnectionPolicy ModestDefaultConnectionPolicy;
typedef struct _ModestDefaultConnectionPolicyClass ModestDefaultConnectionPolicyClass;


struct _ModestDefaultConnectionPolicy
{
	GObject parent;

};

struct _ModestDefaultConnectionPolicyClass
{
	GObjectClass parent_class;
};

GType modest_default_connection_policy_get_type (void);
TnyConnectionPolicy* modest_default_connection_policy_new (void);

G_END_DECLS

#endif
