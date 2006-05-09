#ifndef __MODEST_ACCOUNT_KEYS_H__
#define __MODEST_ACCOUNT_KEYS_H__

#include "modest-conf-keys.h"

#define MODEST_CONF_DEFAULT_ACCOUNT     "/default_account"

/* configuration key definitions for modest */
#define MODEST_ACCOUNT_NAMESPACE	MODEST_CONF_NAMESPACE "/" "accounts"

/* per-account data */
#define MODEST_ACCOUNT_DISPLAY_NAME	"display_name"                  /* string */
#define MODEST_ACCOUNT_DEFAULT		"default"			/* bool */

/* "transport" or "store" */

#define MODEST_ACCOUNT_TYPE            "type"                      /* string */
#define MODEST_ACCOUNT_TYPE_STORE      "store"
#define MODEST_ACCOUNT_TYPE_TRANSPORT  "transport"


#define MODEST_ACCOUNT_PROTO           "proto"		 /* string */
#define MODEST_ACCOUNT_PROTO_POP       "pop"		 /* string */
#define MODEST_ACCOUNT_PROTO_IMAP      "imap"		 /* string */


#define MODEST_ACCOUNT_PASSWORD        "password"       /* string */
#define MODEST_ACCOUNT_SERVER          "server"		 /* string */
#define MODEST_ACCOUNT_USER            "user"       	 /* string */

#define MODEST_ACCOUNT_LEAVE_ON_SERVER       "leave_on_server"     	 /* boolean */ \
#define MODEST_ACCOUNT_PREFERRED_CNX         "preferred_cnx"     	 /* string */

#endif /*__MODEST_CONF_KEYS_H__*/
