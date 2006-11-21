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


#ifndef __MODEST_ACCOUNT_KEYS_H__
#define __MODEST_ACCOUNT_KEYS_H__

#include "modest-conf-keys.h"

#define MODEST_CONF_DEFAULT_ACCOUNT      "/default_account"


/* configuration key definitions for modest */
#define MODEST_ACCOUNT_NAMESPACE         MODEST_CONF_NAMESPACE "/" "accounts"
#define MODEST_SERVER_ACCOUNT_NAMESPACE  MODEST_CONF_NAMESPACE "/" "server_accounts"
#define MODEST_CONF_REPLY_TYPE           "reply_type"        /*  int  */
#define MODEST_CONF_FORWARD_TYPE         "forward_type"      /*  int  */

/* per-account data */
#define MODEST_ACCOUNT_DISPLAY_NAME      "display_name"      /* string */
#define MODEST_ACCOUNT_STORE_ACCOUNT     "store_account"     /* string */
#define MODEST_ACCOUNT_TRANSPORT_ACCOUNT "transport_account" /* string */
#define MODEST_ACCOUNT_FULLNAME		 "fullname"
#define MODEST_ACCOUNT_EMAIL             "email"

/* server account keys */
#define MODEST_ACCOUNT_PASSWORD          "password"          /* string */
#define MDOEST_ACCOUNT_REMEMBER_PWD	 "remember_pwd"      /* boolean */
#define MODEST_ACCOUNT_HOSTNAME          "hostname"          /* string */
#define MODEST_ACCOUNT_USERNAME          "username"          /* string */
#define MODEST_ACCOUNT_PROTO             "proto"             /* string */
#define MODEST_ACCOUNT_ENABLED		 "enabled"	     /* boolean */

#define MODEST_ACCOUNT_LEAVE_ON_SERVER   "leave_on_server"   /* boolean */
#define MODEST_ACCOUNT_PREFERRED_CNX     "preferred_cnx"     /* string */

#endif /*__MODEST_CONF_KEYS_H__*/
