/* Copyright (c) 2007, Nokia Corporation
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

#include <string.h>
#include <modest-account-protocol.h>
#include <modest-defs.h>
#include <modest-protocol-registry.h>
#include <modest-transport-account-decorator.h>
#include <tny-camel-pop-store-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-store-account.h>
#include <tny-camel-transport-account.h>
#include <tny-simple-list.h>

#define TAG_ALL_PROTOCOLS "__MODEST_PROTOCOL_REGISTRY_ALL_PROTOCOLS"

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-store.c 
 */
#define MODEST_ACCOUNT_OPTION_SSL_NEVER "never"
/* This is a tinymail camel-lite specific option, 
 * roughly equivalent to "always" in regular camel,
 * which is appropriate for a generic "SSL" connection option: */
#define MODEST_ACCOUNT_OPTION_SSL_WRAPPED "wrapped"
/* Not used in our UI so far: */
#define MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE "when-possible"
/* This is a tinymailcamel-lite specific option that is not in regular camel. */
#define MODEST_ACCOUNT_OPTION_SSL_TLS "tls"

/* Posssible values for tny_account_set_secure_auth_mech().
 * These might be camel-specific.
 * Really, tinymail should use an enum.
 * camel_sasl_authtype() seems to list some possible values.
 */
 
/* Note that evolution does not offer these for IMAP: */
#define MODEST_ACCOUNT_AUTH_PLAIN "PLAIN"
#define MODEST_ACCOUNT_AUTH_ANONYMOUS "ANONYMOUS"

/* Caeml's IMAP uses NULL instead for "Password".
 * Also, not that Evolution offers "Password" for IMAP, but "Login" for SMTP.*/
#define MODEST_ACCOUNT_AUTH_PASSWORD "LOGIN" 
#define MODEST_ACCOUNT_AUTH_CRAMMD5 "CRAM-MD5"

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-provider.c 
 */
#define MODEST_ACCOUNT_OPTION_USE_LSUB "use_lsub" /* Show only subscribed folders */
#define MODEST_ACCOUNT_OPTION_CHECK_ALL "check_all" /* Check for new messages in all folders */

/* 'private'/'protected' functions */
static void   modest_protocol_registry_class_init (ModestProtocolRegistryClass *klass);
static void   modest_protocol_registry_finalize   (GObject *obj);
static void   modest_protocol_registry_instance_init (ModestProtocolRegistry *obj);
static GHashTable *   modest_protocol_registry_create_tag (ModestProtocolRegistry *obj, const gchar *tag);

/* translation handlers */
static gchar * translation_is_userdata (gpointer userdata, va_list args);

typedef struct _ModestProtocolRegistryPrivate ModestProtocolRegistryPrivate;
struct _ModestProtocolRegistryPrivate {
	GHashTable *tags_table;
	GHashTable *priorities;
};

#define MODEST_PROTOCOL_REGISTRY_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
									 MODEST_TYPE_PROTOCOL_REGISTRY, \
									 ModestProtocolRegistryPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

static ModestProtocolType pop_protocol_type_id = -1;
static ModestProtocolType imap_protocol_type_id = -1;
static ModestProtocolType maildir_protocol_type_id = -1;
static ModestProtocolType mbox_protocol_type_id = -1;
static ModestProtocolType smtp_protocol_type_id = -1;
static ModestProtocolType sendmail_protocol_type_id = -1;
static ModestProtocolType none_connection_protocol_type_id = -1;
static ModestProtocolType ssl_connection_protocol_type_id = -1;
static ModestProtocolType tls_connection_protocol_type_id = -1;
static ModestProtocolType tlsop_connection_protocol_type_id = -1;
static ModestProtocolType none_auth_protocol_type_id = -1;
static ModestProtocolType password_auth_protocol_type_id = -1;
static ModestProtocolType crammd5_auth_protocol_type_id = -1;

GType
modest_protocol_registry_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestProtocolRegistryClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) modest_protocol_registry_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ModestProtocolRegistry),
			0,      /* n_preallocs */
			(GInstanceInitFunc) modest_protocol_registry_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestProtocolRegistry",
						  &my_info, 0);
	}
	return my_type;
}

static void
modest_protocol_registry_class_init (ModestProtocolRegistryClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = modest_protocol_registry_finalize;
	g_type_class_add_private (object_class,
				  sizeof(ModestProtocolRegistryPrivate));
}

static void
modest_protocol_registry_instance_init (ModestProtocolRegistry *obj)
{
	ModestProtocolRegistryPrivate *priv;

	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (obj);

	priv->tags_table = g_hash_table_new_full (g_str_hash, g_str_equal, 
						  g_free, (GDestroyNotify) g_hash_table_unref);
	priv->priorities = g_hash_table_new (g_direct_hash, g_direct_equal);
	
	modest_protocol_registry_create_tag (obj, TAG_ALL_PROTOCOLS);
}

static void   
modest_protocol_registry_finalize   (GObject *obj)
{
	ModestProtocolRegistryPrivate *priv;

	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (obj);

	/* Free hash tables */
	if (priv->tags_table)
		g_hash_table_unref (priv->tags_table);
	if (priv->priorities)
		g_hash_table_unref (priv->priorities);

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}


ModestProtocolRegistry*
modest_protocol_registry_new ()
{
	return g_object_new (MODEST_TYPE_PROTOCOL_REGISTRY, NULL);
}

void
modest_protocol_registry_add (ModestProtocolRegistry *self, ModestProtocol *protocol, gint priority, const gchar *first_tag, ...)
{
	va_list list;
	GSList *tags_list = NULL, *node = NULL;
	const gchar *va_string;
	GHashTable *tag_table;
	ModestProtocolRegistryPrivate *priv;

	g_return_if_fail (MODEST_IS_PROTOCOL_REGISTRY (self));
	g_return_if_fail (MODEST_IS_PROTOCOL (protocol));
	g_return_if_fail (first_tag != NULL);
	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, TAG_ALL_PROTOCOLS);
	g_hash_table_insert (tag_table, GINT_TO_POINTER (modest_protocol_get_type_id (protocol)), g_object_ref (protocol));

	g_hash_table_insert (priv->priorities, GINT_TO_POINTER (modest_protocol_get_type_id (protocol)), GINT_TO_POINTER (priority));

	tags_list = g_slist_prepend (tags_list, (gpointer) first_tag);
	va_start (list, first_tag);
	while ((va_string = va_arg (list, const gchar *)) != NULL) {
		tags_list = g_slist_prepend (tags_list, (gpointer) va_string);
	}
	va_end (list);

	for (node = tags_list; node != NULL; node = g_slist_next (node)) {

		tag_table = g_hash_table_lookup (priv->tags_table, node->data);
		if (tag_table == NULL)
			tag_table = modest_protocol_registry_create_tag (self, node->data);
		g_hash_table_insert (tag_table, GINT_TO_POINTER (modest_protocol_get_type_id (protocol)), g_object_ref (protocol));
	}
	g_slist_free (tags_list);
}

GSList *
modest_protocol_registry_get_all (ModestProtocolRegistry *self)
{
	ModestProtocolRegistryPrivate *priv;

	g_return_val_if_fail (MODEST_IS_PROTOCOL_REGISTRY (self), NULL);
	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	return modest_protocol_registry_get_by_tag (self, TAG_ALL_PROTOCOLS);
	
}

static void
add_protocol_to_list (ModestProtocolType key, ModestProtocol *protocol, GSList **list)
{
	*list = g_slist_prepend (*list, protocol);
}

static gint
compare_protocols (ModestProtocol *a, ModestProtocol *b, GHashTable *priorities)
{
	ModestProtocolType a_type, b_type;
	gint result;

	a_type = modest_protocol_get_type_id (a);
	b_type = modest_protocol_get_type_id (b);

	result = g_hash_table_lookup (priorities, GINT_TO_POINTER (a_type)) - g_hash_table_lookup (priorities, GINT_TO_POINTER (b_type));
	if (result == 0) {
		const gchar *a_display_name;
		const gchar *b_display_name;

		a_display_name = modest_protocol_get_display_name (a);
		b_display_name = modest_protocol_get_display_name (b);
		result = g_utf8_collate (a_display_name, b_display_name);
	}
	return result;
}

GSList *
modest_protocol_registry_get_by_tag (ModestProtocolRegistry *self, const gchar *tag)
{
	ModestProtocolRegistryPrivate *priv;
	GHashTable *tag_table;
	GSList *result;

	g_return_val_if_fail (MODEST_IS_PROTOCOL_REGISTRY (self), NULL);
	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return NULL;
	}

	result = NULL;
	g_hash_table_foreach (tag_table, (GHFunc) add_protocol_to_list, &result);

	result = g_slist_sort_with_data (result, (GCompareDataFunc) compare_protocols, priv->priorities);

	return result;

}

static void
add_protocol_to_pair_list (ModestProtocolType type_id, ModestProtocol *protocol, GSList **list)
{
	*list = g_slist_append (*list,
				(gpointer) modest_pair_new (
					(gpointer) modest_protocol_get_name (protocol),
					(gpointer) modest_protocol_get_display_name (protocol),
					FALSE));
}

ModestPairList *
modest_protocol_registry_get_pair_list_by_tag (ModestProtocolRegistry *self, const gchar *tag)
{
	ModestProtocolRegistryPrivate *priv;
	GHashTable *tag_table;
	GSList *result = NULL;

	g_return_val_if_fail (MODEST_IS_PROTOCOL_REGISTRY (self), NULL);
	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return NULL;
	}

	g_hash_table_foreach (tag_table, (GHFunc) add_protocol_to_pair_list, &result);

	return result;
}

static gboolean
find_protocol_by_name (ModestProtocolType type_id,
		       ModestProtocol *protocol,
		       const gchar *name)
{
	return (strcmp (name, modest_protocol_get_name (protocol)) == 0);
}

ModestProtocol *
modest_protocol_registry_get_protocol_by_name (ModestProtocolRegistry *self, 
					       const gchar *tag, 
					       const gchar *name)
{
	ModestProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_return_val_if_fail (MODEST_IS_PROTOCOL_REGISTRY (self), NULL);
	g_return_val_if_fail (tag, NULL);
	g_return_val_if_fail (name, NULL);

	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return NULL;
	}
	
	return g_hash_table_find (tag_table, (GHRFunc) find_protocol_by_name, (gpointer) name);
}

ModestProtocol *
modest_protocol_registry_get_protocol_by_type (ModestProtocolRegistry *self, ModestProtocolType type_id)
{
	ModestProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_return_val_if_fail (MODEST_IS_PROTOCOL_REGISTRY (self), NULL);
	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, TAG_ALL_PROTOCOLS);
	if (tag_table == NULL) {
		return NULL;
	}
	
	return g_hash_table_lookup (tag_table, GINT_TO_POINTER (type_id));
}

gboolean 
modest_protocol_registry_protocol_type_has_tag (ModestProtocolRegistry *self, ModestProtocolType type_id, const gchar *tag)
{
	ModestProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_return_val_if_fail (MODEST_IS_PROTOCOL_REGISTRY (self), FALSE);
	g_return_val_if_fail (tag != NULL, FALSE);
	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);

	tag_table = g_hash_table_lookup (priv->tags_table, tag);
	if (tag_table == NULL) {
		return FALSE;
	}
	
	return (g_hash_table_lookup (tag_table, GINT_TO_POINTER (type_id))!= NULL);
	
}

static GHashTable *
modest_protocol_registry_create_tag (ModestProtocolRegistry *self, const gchar *tag)
{
	ModestProtocolRegistryPrivate *priv;
	GHashTable *tag_table;

	g_assert (tag != NULL);
	priv = MODEST_PROTOCOL_REGISTRY_GET_PRIVATE (self);
	tag_table = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
					   NULL, g_object_unref);
	g_hash_table_insert (priv->tags_table, g_strdup (tag), tag_table);

	return tag_table;
}

static gchar * 
translation_is_userdata (gpointer userdata, va_list args)
{
	va_list dest;
	gchar *result;

	G_VA_COPY (dest, args);
	result = g_strdup_vprintf (_(userdata), dest);
	va_end (dest);

	return result;
}

static gchar * 
translation_is_userdata_no_param (gpointer userdata, va_list args)
{
	gchar *result;

	result = g_strdup (_(userdata));

	return result;
}


void 
modest_protocol_registry_set_to_default (ModestProtocolRegistry *self)
{
	ModestProtocol *protocol;
	TnyList *account_options;
	TnyPair *pair;

	protocol = modest_account_protocol_new ("sendmail", N_("Sendmail"),
						0, 0,
						TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT);
	sendmail_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_registry_add (self, protocol, 1,
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_account_protocol_new ("smtp", N_("SMTP Server"),
						25, 465,
						MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR);
	smtp_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_CONNECT_ERROR, translation_is_userdata, "emev_ib_ui_smtp_server_invalid", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, translation_is_userdata, "emev_ib_ui_smtp_server_invalid", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_AUTH_ERROR, translation_is_userdata, "emev_ni_ui_smtp_authentication_fail_error", NULL);
	modest_protocol_registry_add (self, protocol, 2,
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_account_protocol_new ("pop", _("mail_fi_emailtype_pop3"),
						110, 995,
						TNY_TYPE_CAMEL_POP_STORE_ACCOUNT);
	pop_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_CONNECT_ERROR, translation_is_userdata, "emev_ni_ui_pop3_msg_connect_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_AUTH_ERROR, translation_is_userdata, "emev_ni_ui_pop3_msg_connect_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, translation_is_userdata, "emev_ni_ui_pop3_msg_connect_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE, translation_is_userdata_no_param, "emev_ni_ui_pop3_msg_recv_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE_LOST_HEADER, translation_is_userdata, "emev_ni_ui_pop3_msg_recv_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_SSL_PROTO_NAME, translation_is_userdata_no_param, "mcen_fi_advsetup_other_security_securepop3s", NULL);
	modest_protocol_registry_add (self, protocol, 3,
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_HAS_LEAVE_ON_SERVER_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_FORBID_INCOMING_XFERS,
				      MODEST_PROTOCOL_REGISTRY_STORE_LIMIT_HEADER_WINDOW,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_account_protocol_new ("imap", _("mail_fi_emailtype_imap"),
						143, 993,
						TNY_TYPE_CAMEL_IMAP_STORE_ACCOUNT);
	imap_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_DELETE_MAILBOX, translation_is_userdata, "emev_nc_delete_mailboximap", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_CONNECT_ERROR, translation_is_userdata, "emev_ni_ui_imap_connect_server_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_AUTH_ERROR, translation_is_userdata, "emev_ni_ui_imap_connect_server_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, translation_is_userdata, "emev_ni_ui_imap_connect_server_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE, translation_is_userdata, "emev_ni_ui_imap_message_not_available_in_server", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE_LOST_HEADER, translation_is_userdata, "emev_ni_ui_pop3_msg_recv_error", NULL);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_SSL_PROTO_NAME, translation_is_userdata_no_param, "mcen_fi_advsetup_other_security_secureimap4s", NULL);
	account_options = tny_simple_list_new ();
	pair = tny_pair_new (MODEST_ACCOUNT_OPTION_USE_LSUB, "");
	tny_list_append (account_options, G_OBJECT (pair));
	g_object_unref (pair);
	pair = tny_pair_new (MODEST_ACCOUNT_OPTION_CHECK_ALL, "");
	tny_list_append (account_options, G_OBJECT (pair));
	g_object_unref (pair);
	modest_account_protocol_set_account_options (MODEST_ACCOUNT_PROTOCOL (protocol), account_options);
	g_object_unref (account_options);
	modest_protocol_registry_add (self, protocol, 4,
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS,
				      MODEST_PROTOCOL_REGISTRY_STORE_LIMIT_HEADER_WINDOW,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_account_protocol_new ("maildir", N_("Maildir"),
						0, 0,
						TNY_TYPE_CAMEL_STORE_ACCOUNT);
	maildir_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, 
					 translation_is_userdata_no_param, "emev_nc_mailbox_notavailable", NULL);
	modest_protocol_registry_add (self, protocol, 5, 
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_LOCAL_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_account_protocol_new ("mbox", N_("MBox"),
						0, 0,
						TNY_TYPE_CAMEL_STORE_ACCOUNT);
	mbox_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set_translation (protocol, MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, 
					 translation_is_userdata_no_param, "emev_nc_mailbox_notavailable", NULL);
	modest_protocol_registry_add (self, protocol, 6,
				      MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_LOCAL_STORE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_protocol_new ("none", N_("None"));
	modest_protocol_set (protocol, MODEST_PROTOCOL_SECURITY_ACCOUNT_OPTION, MODEST_ACCOUNT_OPTION_SSL_NEVER);
	none_connection_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_registry_add (self, protocol, 7,
				      MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_protocol_new ("ssl", N_("SSL"));
	modest_protocol_set (protocol, MODEST_PROTOCOL_SECURITY_ACCOUNT_OPTION, MODEST_ACCOUNT_OPTION_SSL_WRAPPED);
	ssl_connection_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_registry_add (self, protocol, 8,
				      MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_USE_ALTERNATE_PORT,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_protocol_new ("tls", N_("TLS"));
	modest_protocol_set (protocol, MODEST_PROTOCOL_SECURITY_ACCOUNT_OPTION, MODEST_ACCOUNT_OPTION_SSL_TLS);
	tls_connection_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_registry_add (self, protocol, 9, 
				      MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_protocol_new ("tls-op", N_("TLS when possible"));
	modest_protocol_set (protocol, MODEST_PROTOCOL_SECURITY_ACCOUNT_OPTION, MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE);
	tlsop_connection_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_registry_add (self, protocol, 10,
				      MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_protocol_new (MODEST_ACCOUNT_AUTH_MECH_VALUE_NONE, _("mcen_fi_advsetup_smtp_none"));
	none_auth_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set (protocol, MODEST_PROTOCOL_AUTH_ACCOUNT_OPTION, MODEST_ACCOUNT_AUTH_PLAIN);
	modest_protocol_registry_add (self, protocol, 11,
				      MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_protocol_new (MODEST_ACCOUNT_AUTH_MECH_VALUE_PASSWORD, _("mcen_fi_advsetup_smtp_login"));
	password_auth_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set (protocol, MODEST_PROTOCOL_AUTH_ACCOUNT_OPTION, MODEST_ACCOUNT_AUTH_PASSWORD);
	modest_protocol_registry_add (self, protocol, 12,
				      MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	protocol = modest_protocol_new (MODEST_ACCOUNT_AUTH_MECH_VALUE_CRAMMD5, _("mcen_fi_advsetup_smtp_cram_md5"));
	crammd5_auth_protocol_type_id = modest_protocol_get_type_id (protocol);
	modest_protocol_set (protocol, MODEST_PROTOCOL_AUTH_ACCOUNT_OPTION, MODEST_ACCOUNT_AUTH_CRAMMD5);
	modest_protocol_registry_add (self, protocol, 13,
				      MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
				      MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
				      NULL);
	g_object_unref (protocol);

	/* set the custom auth mechs. We do this after creating all the protocols, because if we don't, then we
	 * won't register the auth protocol type id's properly */

	/* IMAP and POP need at least a password,
	 * which camel uses if we specify NULL.
	 * Camel use a password for IMAP or POP if we specify NULL,
	 * For IMAP, at least it will report an error if we use "Password", "Login" or "Plain".
	 * (POP is know to report an error for Login too. Probably Password and Plain too.) */
	protocol = modest_protocol_registry_get_protocol_by_type (self, MODEST_PROTOCOLS_STORE_IMAP);
	modest_account_protocol_set_custom_secure_auth_mech (MODEST_ACCOUNT_PROTOCOL (protocol), MODEST_PROTOCOLS_AUTH_NONE, NULL);
	modest_account_protocol_set_custom_secure_auth_mech (MODEST_ACCOUNT_PROTOCOL (protocol), MODEST_PROTOCOLS_AUTH_PASSWORD, NULL);
	protocol = modest_protocol_registry_get_protocol_by_type (self, MODEST_PROTOCOLS_STORE_POP);
	modest_account_protocol_set_custom_secure_auth_mech (MODEST_ACCOUNT_PROTOCOL (protocol), MODEST_PROTOCOLS_AUTH_NONE, NULL);
	modest_account_protocol_set_custom_secure_auth_mech (MODEST_ACCOUNT_PROTOCOL (protocol), MODEST_PROTOCOLS_AUTH_PASSWORD, NULL);
}

ModestProtocolType
modest_protocol_registry_get_imap_type_id (void)
{
	return imap_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_pop_type_id (void)
{
	return pop_protocol_type_id;
}

ModestProtocolType
modest_protocol_registry_get_maildir_type_id (void)
{
	return maildir_protocol_type_id;
}

ModestProtocolType
modest_protocol_registry_get_mbox_type_id (void)
{
	return mbox_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_smtp_type_id (void)
{
	return smtp_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_sendmail_type_id (void)
{
	return sendmail_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_none_connection_type_id (void)
{
	return none_connection_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_ssl_connection_type_id (void)
{
	return ssl_connection_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_tls_connection_type_id (void)
{
	return tls_connection_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_tlsop_connection_type_id (void)
{
	return tlsop_connection_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_none_auth_type_id (void)
{
	return none_auth_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_password_auth_type_id (void)
{
	return password_auth_protocol_type_id;
}

ModestProtocolType 
modest_protocol_registry_get_crammd5_auth_type_id (void)
{
	return crammd5_auth_protocol_type_id;
}

