#ifndef __MODEST_ACCOUNT_KEYS_H__
#define __MODEST_ACCOUNT_KEYS_H__

#include "modest-conf-keys.h"

#define MODEST_CONF_DEFAULT_ACCOUNT      "/default_account"


/* configuration key definitions for modest */
#define MODEST_ACCOUNT_NAMESPACE         MODEST_CONF_NAMESPACE "/" "accounts"
#define MODEST_SERVER_ACCOUNT_NAMESPACE  MODEST_CONF_NAMESPACE "/" "server_accounts"

/* per-account data */
#define MODEST_ACCOUNT_DISPLAY_NAME      "display_name"      /* string */
#define MODEST_ACCOUNT_STORE_ACCOUNT     "store_account"     /* string */
#define MODEST_ACCOUNT_TRANSPORT_ACCOUNT "transport_account" /* string */

/* server account keys */
#define MODEST_ACCOUNT_PASSWORD          "password"          /* string */
#define MODEST_ACCOUNT_HOSTNAME          "hostname"          /* string */
#define MODEST_ACCOUNT_USERNAME          "username"          /* string */
#define MODEST_ACCOUNT_PROTO             "proto"             /* string */

#define MODEST_ACCOUNT_LEAVE_ON_SERVER   "leave_on_server"   /* boolean */
#define MODEST_ACCOUNT_PREFERRED_CNX     "preferred_cnx"     /* string */

#endif /*__MODEST_CONF_KEYS_H__*/
