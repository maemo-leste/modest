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



