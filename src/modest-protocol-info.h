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

#ifndef __MODEST_PROTOCOL_INFO_H__
#define __MODEST_PROTOCOL_INFO_H__

#include <modest-pair.h>

G_BEGIN_DECLS

/* NOTE: be careful to check modest-protocol-info.c
 * if you make any changes here */
typedef enum {
	MODEST_PROTOCOL_UNKNOWN,
	
	MODEST_PROTOCOL_TRANSPORT_SENDMAIL, 
	MODEST_PROTOCOL_TRANSPORT_SMTP,
	
	MODEST_PROTOCOL_STORE_POP,
	MODEST_PROTOCOL_STORE_IMAP,
	MODEST_PROTOCOL_STORE_MAILDIR,
	MODEST_PROTOCOL_STORE_MBOX,     

	MODEST_PROTOCOL_SECURITY_SSL,   
	MODEST_PROTOCOL_SECURITY_TLS,

	MODEST_PROTOCOL_AUTH_NONE,
	MODEST_PROTOCOL_AUTH_PASSWORD,

	MODEST_PROTOCOL_NUM
} ModestProtocol;


typedef enum {
	MODEST_PROTOCOL_TYPE_UNKNOWN,
	MODEST_PROTOCOL_TYPE_TRANSPORT,
	MODEST_PROTOCOL_TYPE_STORE,
	MODEST_PROTOCOL_TYPE_SECURITY,
	MODEST_PROTOCOL_TYPE_AUTH,

	MODEST_PROTOCOL_TYPE_NUM
} ModestProtocolType;

 
/**
 * modest_protocol_info_get_list:
 * @type: the type of list you want, it should NOT be MODEST_PROTOCOL_TYPE_UNKNOWN
 *
 * return the list of protocols of the given @type.
 * the elements of the returned list are ModestProtocols (use GPOINTER_TO_INT to get it)
 *  
 * Returns: a list of protocols of the given @type; after use, it should be freed
 * with g_slist_free. The elements should not be freed, as there is no memory allocated
 * for them.
 */
GSList*   modest_protocol_info_get_protocol_list (ModestProtocolType type);


/**
 * modest_protocol_info_get_list:
 * @type: the type of list you want, it should NOT be MODEST_PROTOCOL_TYPE_UNKNOWN
 *
 * return the list of <protocol,display-name>-tupels of the given @type.
 * the elements of the returned list are ModestPairs
 * this is a convenience function for use with ModestComboBox
 *  
 * Returns: a list of protocols of the given @type; after use, it should be freed
 * with modest_pair_list_free
 */
ModestPairList*   modest_protocol_info_get_protocol_pair_list (ModestProtocolType type);


/**
 * modest_protocol_info_get_protocol_type:
 * @proto: the ModestProtocol you'd like to query for its type
 *
 * return ModestProtocolType for the protocol
 *  
 * Returns: the protocol type or MODEST_PROTOCOL_TYPE_UNKNOWN
 */
ModestProtocolType modest_protocol_info_get_protocol_type (ModestProtocol proto);


/**
 * modest_protocol_info_get_protocol_type:
 * @name: the name of the  ModestProtocol
 *
 * return the id of the protocol with the given name
 *  
 * Returns: the id of the protocol or MODEST_PROTOCOL_UNKNOWN
 */
ModestProtocol modest_protocol_info_get_protocol (const gchar* name);


/**
 * modest_protocol_info_get_protocol_name:
 * @proto: the protocol you are looking for
 *
 * return the string id of the proto (such as "imap", or "tls")
 *  
 * Returns: string id of the proto as a constant string, that should NOT be modified or freed
 */
const gchar* modest_protocol_info_get_protocol_name (ModestProtocol proto);

/**
 * modest_protocol_info_get_protocol_display_name:
 * @proto: the protocol you are looking for
 *
 * return the string id of the proto (such as "imap", or "tls")
 *  
 * Returns: string id of the proto as a constant string, that should NOT be modified or freed
 *
 */
const gchar* modest_protocol_info_get_protocol_display_name (ModestProtocol proto);



/**
 * modest_protocol_info_protocol_is_local_store:
 * @proto: the protocol
 *
 * is this protocol a local store protocol?
 *  
 * Returns: TRUE if it is a local store, FALSE otherwise
 *
 */
gboolean modest_protocol_info_protocol_is_local_store (ModestProtocol proto);




G_END_DECLS
#endif /* __MODEST_PROTOCOL_INFO_H__ */

