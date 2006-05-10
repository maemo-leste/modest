/* modest-proto.h */

#ifndef __MODEST_PROTO_H__
#define __MODEST_PROTO_H__

#include <glib.h>

#define MODEST_PROTO_SENDMAIL "sendmail"
#define MODEST_PROTO_SMTP     "smtp"
#define MODEST_PROTO_POP      "pop"
#define MODEST_PROTO_IMAP     "imap"

enum {
	MODEST_PROTO_TYPE_ANY       = 0,	
	MODEST_PROTO_TYPE_TRANSPORT = 1,
	MODEST_PROTO_TYPE_STORE     = 2,
};
typedef gint ModestProtoType;

gboolean         modest_proto_is_valid     (const gchar *proto);
ModestProtoType  modest_proto_type         (const gchar *proto);

#endif /*__MODEST_SERVER_PROTO_H__*/
                            
