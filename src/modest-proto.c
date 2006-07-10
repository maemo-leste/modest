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


#include <string.h>
#include "modest-proto.h"


gboolean
modest_proto_is_valid (const gchar *proto)
{
	int i;
	static const gchar* protos[] = {
		MODEST_PROTO_SENDMAIL,
		MODEST_PROTO_SMTP,
		MODEST_PROTO_POP,
		MODEST_PROTO_IMAP,
		NULL
	};
	
	if (!proto)
		return FALSE;

	for (i = 0; protos[i]; ++i) {
		if (strcmp(protos[i], proto) == 0)
			return TRUE;
	}
	
	return FALSE;
}


ModestProtoType
modest_proto_type (const gchar *proto)
{
	if (!modest_proto_is_valid(proto)) {
		g_warning ("invalid protocol %s", proto);
		return -1;
	}
	
	/* trick */
	if (proto[0] == 's')
		return MODEST_PROTO_TYPE_TRANSPORT;
	else
		return MODEST_PROTO_TYPE_STORE;
}
