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


/* modest-proto.h */

#ifndef __MODEST_PROTO_H__
#define __MODEST_PROTO_H__

#include <glib.h>


#define MODEST_PROTO_SENDMAIL "sendmail"
#define MODEST_PROTO_SMTP     "smtp"

#define MODEST_PROTO_NONE     "none"
#define MODEST_PROTO_POP      "pop"
#define MODEST_PROTO_IMAP     "imap"
#define MODEST_PROTO_MAILDIR  "maildir"
#define MODEST_PROTO_MBOX     "mbox"

enum {
	MODEST_PROTO_TYPE_ANY       = 0,	
	MODEST_PROTO_TYPE_TRANSPORT = 1,
	MODEST_PROTO_TYPE_STORE     = 2,
};
typedef gint ModestProtoType;

/**
 * modest_proto_is_valid:
 * @proto: a string describing the protocol
 * @store_proto: is this a store proto?
 * 
 * checks if proto is a valid protocol of the given type
 *
 * Returns: TRUE if proto is valid, FALSE otherwise
 */
gboolean         modest_proto_is_valid     (const gchar *proto, gboolean store_proto);

/**
 * modest_proto_type:
 * @proto: a string describing the protocol
 *
 * converts the string proto into a ModestProtoType
 *
 * Returns: a valid ModestProtoType corresponding to proto
 */
ModestProtoType  modest_proto_type         (const gchar *proto);

/**
 * modest_store_protos:
 *
 * return a list of all available store protos
 *
 * Returns: a newly allocated, NULL-terminated list of of store protocols
 */
const gchar**     modest_proto_store_protos       (void);


/**
 * modest_transport_protos:
 *
 * return a list of all available store protos
 *
 * Returns: a newly allocated, NULL-terminated list of of store protocols
 */
const gchar**     modest_proto_transport_protos   (void);



#endif /*__MODEST_SERVER_PROTO_H__*/
                            
