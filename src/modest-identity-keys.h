#ifndef __MODEST_IDENTITY_KEYS_H__
#define __MODEST_IDENTITY_KEYS_H__

#include "modest-conf-keys.h"

#define MODEST_IDENTITY_DEFAULT_IDENTITY  "myidentity"


/* configuration key definitions for modest */
#define MODEST_IDENTITY_NAMESPACE        MODEST_CONF_NAMESPACE "/" "identities"

/* user identity keys */
#define MODEST_IDENTITY_DISPLAY_NAME      "display_name"   /* string */
#define MODEST_IDENTITY_EMAIL             "email"          /* string */
#define MODEST_IDENTITY_EMAIL_ALTERNATIVES "email_alternatives" /* string */
#define MODEST_IDENTITY_REPLYTO           "replyto"        /* string */
#define MODEST_IDENTITY_SIGNATURE         "signature"      /* string */
#define MODEST_IDENTITY_USE_SIGNATURE     "use_signature"  /* boolean */
#define MODEST_IDENTITY_ID_VIA            "id_via"         /* string */
#define MODEST_IDENTITY_USE_ID_VIA        "use_id_via"     /* boolean */
/* MISSING: everything related to gpg */

#endif /*__MODEST_IDENTITY_KEYS_H__*/
