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
\ * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
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
#include <glib.h>

G_BEGIN_DECLS

/** Transport and Store protocols. */
typedef enum {
	MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN,
	MODEST_PROTOCOL_TRANSPORT_SENDMAIL,
	MODEST_PROTOCOL_TRANSPORT_SMTP,

	MODEST_PROTOCOL_STORE_POP,
	MODEST_PROTOCOL_STORE_IMAP,
	MODEST_PROTOCOL_STORE_MAILDIR,
	MODEST_PROTOCOL_STORE_MBOX
} ModestTransportStoreProtocol;

/** Secure connection methods. */
typedef enum {    
	MODEST_PROTOCOL_CONNECTION_NORMAL,
	MODEST_PROTOCOL_CONNECTION_SSL,   
	MODEST_PROTOCOL_CONNECTION_TLS,
	MODEST_PROTOCOL_CONNECTION_TLS_OP
} ModestConnectionProtocol;

/** Secure authentication methods. */
typedef enum {    
	MODEST_PROTOCOL_AUTH_NONE,
	MODEST_PROTOCOL_AUTH_PASSWORD,
	MODEST_PROTOCOL_AUTH_CRAMMD5
} ModestAuthProtocol;




/**
 * modest_protocol_info_get_transport_store_protocol_pair_list:
 * 
 * return the list of <protocol,display-name>-tupels of protocols.
 * The elements of the returned list are ModestPairs
 * This is a convenience function for use with ModestComboBox
 *  
 * Returns: a list of protocols. After use, it should be freed
 * with modest_pair_list_free
 */
ModestPairList*
modest_protocol_info_get_transport_store_protocol_pair_list ();

/**
 * modest_protocol_info_get_auth_protocol_pair_list:
 * 
 * return the list of <protocol,display-name>-tupels of protocols.
 * The elements of the returned list are ModestPairs
 * This is a convenience function for use with ModestComboBox
 *  
 * Returns: a list of protocols. After use, it should be freed
 * with modest_pair_list_free
 */
ModestPairList* modest_protocol_info_get_auth_protocol_pair_list (void);


/**
 * modest_protocol_info_get_connection_protocol_pair_list:
 * 
 * return the list of <protocol,display-name>-tupels of protocols.
 * The elements of the returned list are ModestPairs
 * This is a convenience function for use with ModestComboBox
 *  
 * Returns: a list of protocols. After use, it should be freed
 * with modest_pair_list_free
 */
ModestPairList* modest_protocol_info_get_connection_protocol_pair_list (void);
	
	
/**
 * modest_protocol_info_get_transport_store_protocol:
 * @name: the name of the  #ModestTransportStoreProtocol
 *
 * return the id of the protocol with the given name
 *  
 * Returns: the id of the protocol or MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN
 */
ModestTransportStoreProtocol modest_protocol_info_get_transport_store_protocol (const gchar* name);

/**
 * modest_protocol_info_get_auth_protocol:
 * @name: The name of the #ModestAuthProtocol
 *
 * Returns the ID of the protocol with the given name
 *
 * Returns: The ID of the protocol or MODEST_PROTOCOL_AUTH_NONE
 */
ModestAuthProtocol modest_protocol_info_get_auth_protocol (const gchar* name);

/**
 * modest_protocol_info_get_connection_protocol:
 * @name: The name of the #ModestConnectionProtocol
 *
 * Returns the ID of the protocol with the given name
 *
 * Returns: The ID of the protocol or MODEST_CONNECTION_PROTOCOL_NORMAL
 */
ModestConnectionProtocol modest_protocol_info_get_connection_protocol (const gchar* name);

/**
 * modest_protocol_info_get_transport_store_protocol_name:
 * @proto: the protocol you are looking for
 * 
 * return the string id of the proto (such as "imap", or "smtp")
 *  
 * Returns: string id of the proto as a constant string, that should NOT be modified or freed
 */
const gchar* modest_protocol_info_get_transport_store_protocol_name (ModestTransportStoreProtocol proto);

/**
 * modest_protocol_info_get_auth_protocol_name:
 * @proto: the protocol you are looking for
 * 
 * return the string id of the proto (such as "password", or "cram-md5")
 *  
 * Returns: string id of the proto as a constant string, that should NOT be modified or freed
 */
const gchar* modest_protocol_info_get_auth_protocol_name (ModestAuthProtocol proto);

/*
 * modest_protocol_get_auth_protocol_pair_list:
 *
 * Get the list of support authentication methods supported by modest including 
 * the display names of those.
 *
 * Returns: List of method/display name pairs
 */
ModestPairList* modest_protocol_info_get_auth_protocol_pair_list (void);

/**
 * modest_protocol_info_get_auth_protocol_name:
 * @proto: the protocol you are looking for
 * 
 * return the string id of the proto (such as "ssl", or "tls")
 *   
 * Returns: string id of the proto as a constant string, that should NOT be modified or freed
 */
const gchar*
modest_protocol_info_get_connection_protocol_name (ModestAuthProtocol proto);


/**
 * modest_protocol_info_protocol_is_local_store:
 * @proto: the protocol
 *
 * is this protocol a store protocol?
 *  
 * Returns: TRUE if it is a local store, FALSE otherwise
 *
 */
gboolean modest_protocol_info_protocol_is_store (ModestTransportStoreProtocol proto);


/**
 * modest_protocol_info_protocol_is_local_store:
 * @proto: the protocol
 *
 * is this protocol a local store protocol?
 *  
 * Returns: TRUE if it is a local store, FALSE otherwise
 *
 */
gboolean modest_protocol_info_protocol_is_local_store (ModestTransportStoreProtocol proto);


/**
 * modest_protocol_info_is_secure:
 * @protocol
 * 
 * is the protocol connection secure (e.g encrypted)?
 * 
 * Returns: TRUE if it is secure, FALSE otherwise
 *
 */
gboolean modest_protocol_info_is_secure(ModestConnectionProtocol protocol);

/**
 * modest_protocol_info_auth_is_secure:
 * @protocol
 * 
 * is the protocol authentication secure (e.g encrypted)?
 * 
 * Returns: TRUE if it is secure, FALSE otherwise
 *
 */
gboolean modest_protocol_info_auth_is_secure(ModestAuthProtocol protocol);


G_END_DECLS
#endif /* __MODEST_PROTOCOL_INFO_H__ */

