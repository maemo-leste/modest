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


#ifndef __MODEST_IDENTITY_KEYS_H__
#define __MODEST_IDENTITY_KEYS_H__

#include "modest-conf-keys.h"

#define MODEST_IDENTITY_DEFAULT_IDENTITY  "default"


/* configuration key definitions for modest */
#define MODEST_IDENTITY_NAMESPACE        MODEST_CONF_NAMESPACE "/" "identities"

/* user identity keys */
#define MODEST_IDENTITY_DISPLAY_NAME      "display_name"   /* string */
#define MODEST_IDENTITY_REALNAME          "realname"       /* string */
#define MODEST_IDENTITY_EMAIL             "email"          /* string */
#define MODEST_IDENTITY_EMAIL_ALTERNATIVES "email_alternatives" /* string */
#define MODEST_IDENTITY_REPLYTO           "replyto"        /* string */
#define MODEST_IDENTITY_SIGNATURE         "signature"      /* string */
#define MODEST_IDENTITY_USE_SIGNATURE     "use_signature"  /* boolean */
#define MODEST_IDENTITY_ID_VIA            "id_via"         /* string */
#define MODEST_IDENTITY_USE_ID_VIA        "use_id_via"     /* boolean */
/* MISSING: everything related to gpg */

#endif /*__MODEST_IDENTITY_KEYS_H__*/
