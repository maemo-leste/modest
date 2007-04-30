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


#ifndef __MODEST_DEFS_H__
#define __MODEST_DEFS_H__

#include <glib/gi18n.h>

/* some interesting dirs. NOTE, they should be prefixed
 * with $HOME; also, except MODEST_DIR itself, they
 * need to be prefixed with MODEST_DIR;
 * why these seperate things? well, this is for crossplatform
 * building of dirs from their components..
 * g_build_dir is your friend
 */
#define MODEST_DIR	                  ".modest"
#define MODEST_CACHE_DIR                  "cache"

#define MODEST_LOCAL_FOLDERS_ACCOUNT_ID   "local_folders"
#define MODEST_LOCAL_FOLDERS_ACCOUNT_NAME MODEST_LOCAL_FOLDERS_ACCOUNT_ID
#define MODEST_LOCAL_FOLDERS_MAILDIR      MODEST_LOCAL_FOLDERS_ACCOUNT_ID

#define MODEST_MMC_ACCOUNT_ID             "mcc"
#define MODEST_MCC_ACCOUNT_MAILDIR	  "/media/mmc1/.Maildir"

/* configuration key definitions for modest */
#define MODEST_CONF_NAMESPACE		"/apps/modest"

/* configuration key definitions for modest */
#define MODEST_ACCOUNT_NAMESPACE         MODEST_CONF_NAMESPACE "/accounts"
#define MODEST_CONF_DEFAULT_ACCOUNT      MODEST_CONF_NAMESPACE "/default_account"

#define MODEST_CONF_REPLY_TYPE           MODEST_CONF_NAMESPACE "/reply_type"        /*  int  */
#define MODEST_CONF_FORWARD_TYPE         MODEST_CONF_NAMESPACE "/forward_type"      /*  int  */

#define MODEST_CONF_PREFER_FORMATTED_TEXT MODEST_CONF_NAMESPACE "/prefer_formatted_text"

#define MODEST_CONF_CONNECT_AT_STARTUP   MODEST_CONF_NAMESPACE "/connect_at_startup"      

#define MODEST_CONF_SHOW_CC              MODEST_CONF_NAMESPACE "/show_cc"           
#define MODEST_CONF_SHOW_BCC             MODEST_CONF_NAMESPACE "/show_bcc"           
#define MODEST_CONF_SHOW_TOOLBAR         MODEST_CONF_NAMESPACE "/show_toolbar"
#define MODEST_CONF_SHOW_TOOLBAR_FULLSCREEN         MODEST_CONF_NAMESPACE "/show_toolbar_fullscreen"

/*
 * in the maemo case, we try to replace this
 * with the device name
 */
#define MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME  N_("Local folders")

/* the name of the device; in case of maemo this is set and updated
 * using dbus; see modest-maemo-utils.[ch]
 */
#define MODEST_CONF_DEVICE_NAME       MODEST_CONF_NAMESPACE "/device_name"


/* place for widget settings */
#define MODEST_CONF_WIDGET_NAMESPACE     MODEST_CONF_NAMESPACE "/widgets"

#define MODEST_SERVER_ACCOUNT_NAMESPACE  MODEST_CONF_NAMESPACE "/" "server_accounts"


/* per-account data */
#define MODEST_ACCOUNT_DISPLAY_NAME      "display_name"      /* string */
#define MODEST_ACCOUNT_STORE_ACCOUNT     "store_account"     /* string */
#define MODEST_ACCOUNT_TRANSPORT_ACCOUNT "transport_account" /* string */
#define MODEST_ACCOUNT_FULLNAME		 "fullname"          /* string */
#define MODEST_ACCOUNT_EMAIL             "email"             /* string */

/* This is a list of strings, with each strings, 
 * alernating between a connection name, followed by a corresponding server account name.
 * That's not pretty, but it's nicer than dealing with escaping of a = separator if 
 * putting them both in one string. */
#define MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST "specific_smtp"

/* server account keys */
#define MODEST_ACCOUNT_PASSWORD          "password"          /* string */
#define MODEST_ACCOUNT_REMEMBER_PWD	 "remember_pwd"      /* boolean */
#define MODEST_ACCOUNT_HOSTNAME          "hostname"          /* string */
#define MODEST_ACCOUNT_USERNAME          "username"          /* string */
#define MODEST_ACCOUNT_USE_SIGNATURE         "use_signature"         /* boolean */
#define MODEST_ACCOUNT_SIGNATURE         "signature"         /* string */
#define MODEST_ACCOUNT_URI		 "uri"	             /* string */

#define MODEST_ACCOUNT_PROTO             "proto"             /* string */
#define MODEST_ACCOUNT_ENABLED		 "enabled"	     /* boolean */
#define MODEST_ACCOUNT_TYPE		 "type"	             /* string */
#define MODEST_ACCOUNT_LAST_UPDATED      "last_updated"      /* int */

#define MODEST_ACCOUNT_LEAVE_ON_SERVER   "leave_on_server"   /* boolean */
#define MODEST_ACCOUNT_PREFERRED_CNX     "preferred_cnx"     /* string */
#define MODEST_ACCOUNT_PORT		         "port"	             /* int */
#define MODEST_ACCOUNT_OPTIONS		 "options"	     /* list */

#define MODEST_ACCOUNT_AUTH_MECH	 "auth_mech"	     /* string */
#define MODEST_ACCOUNT_AUTH_MECH_VALUE_NONE "none"
#define MODEST_ACCOUNT_AUTH_MECH_VALUE_PASSWORD "password"
#define MODEST_ACCOUNT_AUTH_MECH_VALUE_CRAMMD5 "cram-md5"

#define MODEST_ACCOUNT_RETRIEVE	 "retrieve"	     /* string */
#define MODEST_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY "headers-only"
#define MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES "messages"
#define MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES_AND_ATTACHMENTS "messages-and-attachments"

#define MODEST_ACCOUNT_LIMIT_RETRIEVE	 "limit-retrieve"	     /* int */

#define MODEST_ACCOUNT_SECURITY "security"
#define MODEST_ACCOUNT_SECURITY_VALUE_NONE "none"
#define MODEST_ACCOUNT_SECURITY_VALUE_NORMAL "normal"
#define MODEST_ACCOUNT_SECURITY_VALUE_SSL "ssl"


/* Options keys that correspond directly to camel options.
 * These probably should not be written to user configuration, 
 * because they are specific API implementation details. */
/* Keys and values used in the text items in the options GSList: */
#define MODEST_ACCOUNT_OPTION_SSL "use_ssl"
#define MODEST_ACCOUNT_OPTION_SSL_NEVER "never"
#define MODEST_ACCOUNT_OPTION_SSL_ALWAYS "always"
#define MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE "when-possible"

#define MODEST_ACCOUNT_OPTION_USE_LSUB "use_lsub"
#define MODEST_ACCOUNT_OPTION_CHECK_ALL "check_all"

/* Macros for different text formats in mail editor */
#define MODEST_FILE_FORMAT_PLAIN_TEXT 0
#define MODEST_FILE_FORMAT_FORMATTED_TEXT 1

#endif /*__MODEST_DEFS_H__*/
