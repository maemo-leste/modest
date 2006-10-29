/* modest-protocol-mgr.c */

/* insert (c)/licensing information) */

#include "modest-protocol-mgr.h"
#include <string.h> /* strcmp */
#include <modest-pair.h>

/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_protocol_mgr_class_init (ModestProtocolMgrClass *klass);
static void modest_protocol_mgr_init       (ModestProtocolMgr *obj);
static void modest_protocol_mgr_finalize   (GObject *obj);
/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestProtocolMgrPrivate ModestProtocolMgrPrivate;
struct _ModestProtocolMgrPrivate {
	GSList *transport_protos;
	GSList *store_protos;
	GSList *security_protos;
	GSList *auth_protos;
};
#define MODEST_PROTOCOL_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_PROTOCOL_MGR, \
                                                 ModestProtocolMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_protocol_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestProtocolMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_protocol_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestProtocolMgr),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_protocol_mgr_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestProtocolMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_protocol_mgr_class_init (ModestProtocolMgrClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_protocol_mgr_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestProtocolMgrPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_protocol_mgr_init (ModestProtocolMgr *obj)
{
	ModestProtocolMgrPrivate *priv;
	priv = MODEST_PROTOCOL_MGR_GET_PRIVATE (obj);
	
	priv->transport_protos = NULL;
	priv->store_protos     = NULL;
	priv->security_protos  = NULL;
	priv->auth_protos      = NULL;
}




static void
modest_protocol_mgr_finalize (GObject *obj)
{
	ModestProtocolMgrPrivate *priv;
	priv = MODEST_PROTOCOL_MGR_GET_PRIVATE (obj);
	
	priv->transport_protos =
		modest_pair_gslist_destroy (priv->transport_protos);
	priv->store_protos = 
		modest_pair_gslist_destroy (priv->store_protos);
	priv->security_protos = 
		modest_pair_gslist_destroy (priv->security_protos);
	priv->auth_protos = 
		modest_pair_gslist_destroy (priv->auth_protos);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestProtocolMgr*
modest_protocol_mgr_new (void)
{
	return MODEST_PROTOCOL_MGR(g_object_new(MODEST_TYPE_PROTOCOL_MGR, NULL));
}



const GSList*
modest_protocol_mgr_get_transport_protocols (ModestProtocolMgr* self)
{
	ModestProtocolMgrPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_PROTOCOL_MGR_GET_PRIVATE (self);
	
	if (!priv->transport_protos) {
		priv->transport_protos =
			g_slist_append (priv->transport_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_TRANSPORT_SENDMAIL,
						(gpointer)_("Sendmail"), FALSE));
		priv->transport_protos =
			g_slist_append (priv->transport_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_TRANSPORT_SMTP,
						(gpointer)_("SMTP-server"), FALSE));
	}
	return priv->transport_protos;
}


const GSList*
modest_protocol_mgr_get_store_protocols (ModestProtocolMgr* self)
{
	ModestProtocolMgrPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_PROTOCOL_MGR_GET_PRIVATE (self);
	
	if (!priv->store_protos) {
		priv->store_protos = 
			g_slist_append (priv->store_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_STORE_POP,
						(gpointer)_("POP3"), FALSE));
		priv->store_protos = 
			g_slist_append (priv->store_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_STORE_IMAP,
						(gpointer)_("IMAP v4"), FALSE));
		priv->store_protos = 
			g_slist_append (priv->store_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_STORE_MBOX,
						(gpointer)_("Mbox"), FALSE));
		priv->store_protos = 
			g_slist_append (priv->store_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_STORE_MAILDIR,
						(gpointer)_("Maildir"), FALSE));
	}
	return priv->store_protos;
}

const GSList*
modest_protocol_mgr_get_security_protocols (ModestProtocolMgr* self)
{
	ModestProtocolMgrPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_PROTOCOL_MGR_GET_PRIVATE (self);
	
	if (!priv->security_protos) {
		priv->security_protos = 
			g_slist_append (priv->security_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_SECURITY_NONE,
						(gpointer)_("None"), FALSE));
		priv->security_protos = 
			g_slist_append (priv->security_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_SECURITY_TLS,
						(gpointer)_("TLS"), FALSE));
		priv->security_protos = 
			g_slist_append (priv->security_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_SECURITY_SSL,
						(gpointer)_("SSL"), FALSE));
	}
	return priv->security_protos;
}



const GSList*
modest_protocol_mgr_get_auth_protocols (ModestProtocolMgr* self)
{
	ModestProtocolMgrPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_PROTOCOL_MGR_GET_PRIVATE (self);
	
	if (!priv->auth_protos) {
		priv->auth_protos = 
			g_slist_append (priv->auth_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_AUTH_NONE,
						(gpointer)_("None"), FALSE));
		priv->auth_protos =
			g_slist_append (priv->auth_protos,
					(gpointer)modest_pair_new(
						(gpointer)MODEST_PROTOCOL_AUTH_PASSWORD,
						(gpointer)_("Password"), FALSE));
	}
	return priv->auth_protos;
}




gboolean
modest_protocol_mgr_protocol_is_valid (ModestProtocolMgr* self, const gchar* proto,
				       ModestProtocolType type)
{
	gboolean found = FALSE;
	
	switch (type) {
	case MODEST_PROTOCOL_TYPE_ANY:

	case MODEST_PROTOCOL_TYPE_TRANSPORT:
		found = found
			|| strcmp(proto, MODEST_PROTOCOL_TRANSPORT_SENDMAIL) == 0 
			|| strcmp(proto, MODEST_PROTOCOL_TRANSPORT_SMTP) == 0;
		if (found || type != MODEST_PROTOCOL_TYPE_ANY)
			break;
	case MODEST_PROTOCOL_TYPE_STORE:
		found = found
			|| strcmp(proto, MODEST_PROTOCOL_STORE_POP) == 0 
			|| strcmp(proto, MODEST_PROTOCOL_STORE_IMAP) == 0
			|| strcmp(proto, MODEST_PROTOCOL_STORE_MAILDIR) == 0 
			|| strcmp(proto, MODEST_PROTOCOL_STORE_MBOX) == 0;
		if (found || type != MODEST_PROTOCOL_TYPE_ANY)
			break;
		
	case MODEST_PROTOCOL_TYPE_SECURITY:
		found = found
			|| strcmp(proto, MODEST_PROTOCOL_SECURITY_NONE) == 0 
			|| strcmp(proto, MODEST_PROTOCOL_SECURITY_SSL) == 0
			|| strcmp(proto, MODEST_PROTOCOL_SECURITY_TLS) == 0;
		if (found || type != MODEST_PROTOCOL_TYPE_ANY)
			break;
		
	case MODEST_PROTOCOL_TYPE_AUTH:
		found = found
			|| strcmp(proto, MODEST_PROTOCOL_AUTH_NONE) == 0 
			|| strcmp(proto, MODEST_PROTOCOL_AUTH_PASSWORD) == 0;
		break;
	default:
		g_warning ("invalid protocol type %d", type);
	}
	
	return found;
}
