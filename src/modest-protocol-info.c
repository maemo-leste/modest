/* Copyright (c) 2006, Nokia Corporation
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

#include <glib/gi18n.h>
#include <string.h> /* strcmp */
#include <modest-protocol-info.h>
#include <modest-pair.h>


typedef struct {
	gint   proto;
	const gchar*     name;
	const gchar*     display_name;
} ProtocolInfo;

static const ProtocolInfo TransportStoreProtocolMap[] = {
	{ MODEST_PROTOCOL_TRANSPORT_SENDMAIL, "sendmail", N_("Sendmail") },
	{ MODEST_PROTOCOL_TRANSPORT_SMTP,     "smtp",     N_("SMTP Server") },
	
	{ MODEST_PROTOCOL_STORE_POP,          "pop",      N_("POP3") },
	{ MODEST_PROTOCOL_STORE_IMAP,         "imap",     N_("IMAPv4") },
	{ MODEST_PROTOCOL_STORE_MAILDIR,      "maildir",  N_("Maildir") },
	{ MODEST_PROTOCOL_STORE_MBOX,         "mbox",     N_("MBox") }
};

static const ProtocolInfo ConnectionProtocolMap[] = {
	{ MODEST_PROTOCOL_CONNECTION_NORMAL,    "none",     N_("None") },   
	{ MODEST_PROTOCOL_CONNECTION_SSL,       "ssl",      N_("SSL") },   
	{ MODEST_PROTOCOL_CONNECTION_TLS,       "tls",      N_("TLS") },
	{ MODEST_PROTOCOL_CONNECTION_TLS_OP,    "tls-op",   N_("TLS when possible") }
	/* op stands for optional */
};


/* FIXME: these names must match those of tny_camel_account_get_supported_secure_auth */
static const ProtocolInfo AuthProtocolMap[] = {
	{ MODEST_PROTOCOL_AUTH_NONE,          "none",     N_("None") },
	{ MODEST_PROTOCOL_AUTH_PASSWORD,      "Password", N_("Password") },
	{ MODEST_PROTOCOL_AUTH_CRAMMD5,       "cram-md5", N_("Cram MD5") }
};


static ModestPairList*
get_protocol_pair_list (const ProtocolInfo* map, guint size)
{
	g_return_val_if_fail (map, NULL);

	ModestPairList *proto_list = NULL;	
	int i;
	for (i = 0; i != size; ++i) {
		const ProtocolInfo info = map[i];
		proto_list = g_slist_append (proto_list,
					     (gpointer)modest_pair_new(
						     (gpointer)info.name,
						     (gpointer)info.display_name,
						     FALSE));			
	}
	return proto_list;
}


ModestPairList*
modest_protocol_info_get_transport_store_protocol_pair_list ()
{
	return get_protocol_pair_list (TransportStoreProtocolMap,
		G_N_ELEMENTS(TransportStoreProtocolMap));
}

ModestPairList*
modest_protocol_info_get_auth_protocol_pair_list ()
{
	return get_protocol_pair_list (AuthProtocolMap,
		G_N_ELEMENTS(AuthProtocolMap));
}

ModestPairList*
modest_protocol_info_get_connection_protocol_pair_list ()
{
	return get_protocol_pair_list (ConnectionProtocolMap,
		G_N_ELEMENTS(ConnectionProtocolMap));
}
	

ModestTransportStoreProtocol
modest_protocol_info_get_transport_store_protocol (const gchar* name)
{
	const ProtocolInfo *map = TransportStoreProtocolMap;
	const guint size = G_N_ELEMENTS(TransportStoreProtocolMap);
	int i;
	
	g_return_val_if_fail (name, MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN);
	
	g_return_val_if_fail (map, MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN);

	for (i = 0; i != size; ++i) {
		const ProtocolInfo info = map[i];
		if (strcmp(name, info.name) == 0)
			return info.proto;
	}
	
	return MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN;
}



/* get either the name or the display_name for the protocol */
static const gchar*
get_protocol_string (gint proto, const ProtocolInfo* map, guint size, gboolean get_name)
{
	g_return_val_if_fail (map, NULL);
	
	int i;
	for (i = 0; i != size; ++i) {
		ProtocolInfo info = map[i];
		if (info.proto == proto)
			return get_name ? info.name : info.display_name;	
	}
	g_return_val_if_reached (NULL);
}

const gchar*
modest_protocol_info_get_transport_store_protocol_name (ModestTransportStoreProtocol proto)
{
	return get_protocol_string (proto, TransportStoreProtocolMap,
		G_N_ELEMENTS(TransportStoreProtocolMap), TRUE);
}

const gchar*
modest_protocol_info_get_auth_protocol_name (ModestAuthProtocol proto)
{
	return get_protocol_string (proto, AuthProtocolMap,
		G_N_ELEMENTS(AuthProtocolMap), TRUE);
}

const gchar*
modest_protocol_info_get_connection_protocol_name (ModestAuthProtocol proto)
{
	return get_protocol_string (proto, ConnectionProtocolMap,
		G_N_ELEMENTS(ConnectionProtocolMap), TRUE);
}


gboolean
modest_protocol_info_protocol_is_local_store (ModestTransportStoreProtocol proto)
{
	return proto == MODEST_PROTOCOL_STORE_MBOX || proto == MODEST_PROTOCOL_STORE_MAILDIR;
}



gboolean
modest_protocol_info_protocol_is_store (ModestTransportStoreProtocol proto)
{
	return proto == MODEST_PROTOCOL_STORE_MBOX || proto == MODEST_PROTOCOL_STORE_MAILDIR ||
		proto == MODEST_PROTOCOL_STORE_POP || proto == MODEST_PROTOCOL_STORE_IMAP;
}
