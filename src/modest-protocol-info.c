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
	ModestProtocol   proto;
	const gchar*     name;
	const gchar*     display_name;
} ProtocolInfo;

static const ProtocolInfo ProtocolMap[] = {
	{ MODEST_PROTOCOL_TRANSPORT_SENDMAIL, "sendmail", N_("Sendmail") },
	{ MODEST_PROTOCOL_TRANSPORT_SMTP,     "smtp",     N_("SMTP Server") },
	
	{ MODEST_PROTOCOL_STORE_POP,          "pop",      N_("POP3") },
	{ MODEST_PROTOCOL_STORE_IMAP,         "imap",     N_("IMAPv4") },
	{ MODEST_PROTOCOL_STORE_MAILDIR,      "maildir",  N_("Maildir") },
	{ MODEST_PROTOCOL_STORE_MBOX,         "mbox",     N_("MBox") }
};
const guint PROTOCOL_SECURITY_MAP_SIZE = sizeof(ProtocolMap)/sizeof(ProtocolInfo);

static const ProtocolInfo ProtocolSecurityMap[] = {
	{ MODEST_PROTOCOL_SECURITY_NONE,      "none",     N_("None") },   
	{ MODEST_PROTOCOL_SECURITY_SSL,       "ssl",      N_("SSL") },   
	{ MODEST_PROTOCOL_SECURITY_TLS,       "tls",      N_("TLS") },
	{ MODEST_PROTOCOL_SECURITY_TLS_OP,    "tls-op",   N_("TLS when possible") } /* op stands for optional */
};
const guint PROTOCOL_MAP_SIZE = sizeof(ProtocolSecurityMap)/sizeof(ProtocolInfo);

static const ProtocolInfo ProtocolAuthMap[] = {
	{ MODEST_PROTOCOL_AUTH_NONE,          "none",     N_("None") },
	{ MODEST_PROTOCOL_AUTH_PASSWORD,      "password", N_("Password") },
	{ MODEST_PROTOCOL_AUTH_CRAMMD5,       "cram-md5", N_("Cram MD5") }
};
const guint PROTOCOL_AUTH_MAP_SIZE = sizeof(ProtocolAuthMap)/sizeof(ProtocolInfo);



#if 0
GSList*
modest_protocol_info_get_protocol_list (ModestProtocolType type)
{
	GSList *proto_list = NULL;
	int i;
	
	g_return_val_if_fail (type > MODEST_PROTOCOL_TYPE_UNKNOWN &&
			      type < MODEST_PROTOCOL_TYPE_NUM,
			      NULL);
 		
	for (i = 0; i != PROTOCOL_MAP_SIZE; ++i) {
		ProtocolInfo info = (ProtocolInfo)ProtocolMap[i];
		if (modest_protocol_info_get_protocol_type(info.proto) == type)
			proto_list = g_slist_append (proto_list, GINT_TO_POINTER(info.proto));
	}
	return proto_list;
}
#endif

static ModestPairList*
modest_protocol_info_get_pair_list (const ProtocolInfo* map, guint size)
{
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
modest_protocol_info_get_protocol_pair_list ()
{
	return modest_protocol_info_get_pair_list (ProtocolMap, PROTOCOL_MAP_SIZE);
}

ModestPairList*
modest_protocol_info_get_protocol_security_pair_list ()
{
	return modest_protocol_info_get_pair_list (ProtocolSecurityMap, PROTOCOL_SECURITY_MAP_SIZE);
}


ModestPairList*
modest_protocol_info_get_protocol_auth_pair_list ()
{
	return modest_protocol_info_get_pair_list (ProtocolAuthMap, PROTOCOL_AUTH_MAP_SIZE);
}



ModestProtocolType
modest_protocol_info_get_protocol_type (ModestProtocol proto)
{
	switch (proto) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		return MODEST_PROTOCOL_TYPE_TRANSPORT;
		
	case MODEST_PROTOCOL_STORE_POP:
	case MODEST_PROTOCOL_STORE_IMAP:
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		return MODEST_PROTOCOL_TYPE_STORE;

	default:
		return MODEST_PROTOCOL_TYPE_UNKNOWN;
	}
}


ModestProtocol
modest_protocol_info_get_protocol (const gchar* name)
{
	int i;
	g_return_val_if_fail (name, MODEST_PROTOCOL_UNKNOWN);

	for (i = 0; i != PROTOCOL_MAP_SIZE; ++i) {
		ProtocolInfo info = (ProtocolInfo)ProtocolMap[i];
		if (strcmp(name, info.name) == 0)
			return info.proto;
	}
	
	return MODEST_PROTOCOL_UNKNOWN;
}




/* get either the name or the display_name for the protocol */
static const gchar*
get_protocol_string (ModestProtocol proto, gboolean get_name)
{
	int i;
	g_return_val_if_fail (modest_protocol_info_get_protocol_type(proto) !=
			      MODEST_PROTOCOL_TYPE_UNKNOWN, NULL);
	
	for (i = 0; i != PROTOCOL_MAP_SIZE; ++i) {
		ProtocolInfo info = (ProtocolInfo)ProtocolMap[i];
		if (info.proto == proto)
			return get_name ? info.name : info.display_name;	
	}
	g_return_val_if_reached (NULL);
}

const gchar*
modest_protocol_info_get_protocol_name (ModestProtocol proto)
{
	return get_protocol_string (proto, TRUE);
}


const gchar*
modest_protocol_info_get_protocol_display_name (ModestProtocol proto)
{
	return get_protocol_string (proto, FALSE);
}


gboolean
modest_protocol_info_protocol_is_local_store (ModestProtocol proto)
{
	g_return_val_if_fail (modest_protocol_info_get_protocol_type (proto) !=
			      MODEST_PROTOCOL_TYPE_UNKNOWN, FALSE);

	/* may add MH later */
	return proto == MODEST_PROTOCOL_STORE_MBOX || proto == MODEST_PROTOCOL_STORE_MAILDIR;
}


