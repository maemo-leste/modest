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

#include <config.h>
#include <glib/gi18n.h>

/* Some interesting directories. NOTE, they should be prefixed
 * with $HOME; Also, except for MODEST_DIR itself, they
 * need to be prefixed with MODEST_DIR;
 * The parts of the path are separate for crossplatform
 * building of dirs from their components.
 * g_build_dir is your friend
 */
#define MODEST_DIR	                  ".modest"
#define MODEST_CACHE_DIR                  "cache"

#define MODEST_LOCAL_FOLDERS_ACCOUNT_ID   "local_folders"
#define MODEST_LOCAL_FOLDERS_ACCOUNT_NAME MODEST_LOCAL_FOLDERS_ACCOUNT_ID

/** Sent, Drafts, etc, are on-disk 
 * but not outbox, because outbox is a virtual folder merged from the 
 * various outboxes/<account-name>/outbox folders.
 */
#define MODEST_LOCAL_FOLDERS_MAILDIR      MODEST_LOCAL_FOLDERS_ACCOUNT_ID

/** There is an outboxes/<account-name>/outbox/ folder for each account,
 * though we merge them so that the user sees only one outbox.
 */
#define MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDER_ACCOUNT_ID_PREFIX "outboxes"
#define MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDERS_MAILDIR MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDER_ACCOUNT_ID_PREFIX

#define MODEST_MMC_ACCOUNT_ID             "mcc"

/* FIXME: get these from the environment */
/* Without the trailing / because gnome-vfs reports mounted 
 * volume URIs without the trailing, and we want to match them: */
#define MODEST_MCC1_VOLUMEPATH	  "/media/mmc1" 
#define MODEST_MCC1_VOLUMEPATH_URI "file://" MODEST_MCC1_VOLUMEPATH

/* configuration key definitions for modest */
#define MODEST_CONF_NAMESPACE		"/apps/modest"

/* the mapping files, there are two possibilities; used in modest_maemo_open_mcc_mapping_file */
#define MODEST_MCC_MAPPING                 PREFIX "/share/modest/provider-data/mcc_mapping"
#define MODEST_OPERATOR_WIZARD_MCC_MAPPING "/usr/share/operator-wizard/mcc_mapping"

#define MODEST_PROVIDER_DATA_FILE         PREFIX "/share/modest/provider-data/modest-provider-data.keyfile"  
#define MODEST_MAEMO_PROVIDER_DATA_FILE	  PREFIX "/share/modest/provider-data/maemo-provider-data.keyfile"  


#define MODEST_ICON_SIZE_SMALL            26
#define MODEST_ICON_SIZE_BIG		  64

/* configuration key definitions for modest */
#define MODEST_ACCOUNT_NAMESPACE         MODEST_CONF_NAMESPACE "/accounts"
#define MODEST_CONF_DEFAULT_ACCOUNT      MODEST_CONF_NAMESPACE "/default_account"

/* Not used: #define MODEST_CONF_CONNECT_AT_STARTUP   MODEST_CONF_NAMESPACE "/connect_at_startup" */      

#define MODEST_CONF_SHOW_CC              MODEST_CONF_NAMESPACE "/show_cc"           
#define MODEST_CONF_SHOW_BCC             MODEST_CONF_NAMESPACE "/show_bcc"           

/* This is the alarmd cookie, obtained from alarm_event_add(), 
 * which apparently remains valid between application instances.
 * We store it so that we can remove it later.
 */
#define MODEST_CONF_ALARM_ID MODEST_CONF_NAMESPACE "/alarm_id"

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
#define MODEST_CONF_FOLDER_VIEW_KEY      "folder-view"
#define MODEST_CONF_HEADER_VIEW_KEY      "header-view"
#define MODEST_CONF_MAIN_PANED_KEY       "modest-main-paned"
#define MODEST_CONF_MSG_PANED_KEY        "modest-msg-paned"
#define MODEST_CONF_FOLDER_PANED_KEY     "modest-folder-paned"
#define MODEST_CONF_MAIN_WINDOW_KEY      "modest-main-window"
#define MODEST_CONF_EDIT_WINDOW_KEY      "modest-edit-msg-window"
#define MODEST_CONF_MSG_VIEW_WINDOW_KEY  "modest-msg-view-window"

#define MODEST_SERVER_ACCOUNT_NAMESPACE  MODEST_CONF_NAMESPACE "/" "server_accounts"

/* show toolbar settings */
#define MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR MODEST_CONF_WIDGET_NAMESPACE "/" MODEST_CONF_MAIN_WINDOW_KEY "/show_toolbar"
#define MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR MODEST_CONF_WIDGET_NAMESPACE "/" MODEST_CONF_EDIT_WINDOW_KEY "/show_toolbar"
#define MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR MODEST_CONF_WIDGET_NAMESPACE "/" MODEST_CONF_MSG_VIEW_WINDOW_KEY "/show_toolbar"
#define MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR_FULLSCREEN MODEST_CONF_WIDGET_NAMESPACE "/" MODEST_CONF_MAIN_WINDOW_KEY "/show_toolbar_fullscreen"
#define MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR_FULLSCREEN MODEST_CONF_WIDGET_NAMESPACE "/" MODEST_CONF_EDIT_WINDOW_KEY "/show_toolbar_fullscreen"
#define MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR_FULLSCREEN MODEST_CONF_WIDGET_NAMESPACE "/" MODEST_CONF_MSG_VIEW_WINDOW_KEY "/show_toolbar_fullscreen"

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
#define MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST MODEST_CONF_NAMESPACE "/specific_smtp" /* one list used for all accounts. */
#define MODEST_ACCOUNT_USE_CONNECTION_SPECIFIC_SMTP  "use_specific_smtp" /* boolean */

/* server account keys */
#define MODEST_ACCOUNT_PASSWORD          "password"          /* string */
#define MODEST_ACCOUNT_REMEMBER_PWD	 "remember_pwd"      /* boolean */
#define MODEST_ACCOUNT_HOSTNAME          "hostname"          /* string */
#define MODEST_ACCOUNT_USERNAME          "username"          /* string */
#define MODEST_ACCOUNT_USERNAME_HAS_SUCCEEDED          "username_succeeded"          /* string */
#define MODEST_ACCOUNT_USE_SIGNATURE         "use_signature"         /* boolean */
#define MODEST_ACCOUNT_SIGNATURE         "signature"         /* string */

/* Only used for mbox and maildir accounts: */
#define MODEST_ACCOUNT_URI		 "uri"	             /* string */

#define MODEST_ACCOUNT_PROTO             "proto"             /* string */
#define MODEST_ACCOUNT_ENABLED		 "enabled"	     /* boolean */
#define MODEST_ACCOUNT_TYPE		 "type"	             /* string */
#define MODEST_ACCOUNT_LAST_UPDATED      "last_updated"      /* int */

#define MODEST_ACCOUNT_LEAVE_ON_SERVER   "leave_on_server"   /* boolean */
#define MODEST_ACCOUNT_PREFERRED_CNX     "preferred_cnx"     /* string */
#define MODEST_ACCOUNT_PORT		         "port"	             /* int */

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
#define MODEST_ACCOUNT_SECURITY_VALUE_NORMAL "normal" /* Meaning "Normal (TLS)", as in our UI spec. */ 
#define MODEST_ACCOUNT_SECURITY_VALUE_SSL "ssl"

/* Macros for different text formats in mail editor */
#define MODEST_FILE_FORMAT_PLAIN_TEXT 0
#define MODEST_FILE_FORMAT_FORMATTED_TEXT 1

/* Global settings */
#define MODEST_CONF_AUTO_UPDATE MODEST_CONF_NAMESPACE "/auto_update" /* bool */
#define MODEST_CONF_UPDATE_WHEN_CONNECTED_BY MODEST_CONF_NAMESPACE "/update_when_connected_by" /* int */
#define MODEST_CONF_UPDATE_INTERVAL MODEST_CONF_NAMESPACE "/update_interval" /* int */
#define MODEST_CONF_MSG_SIZE_LIMIT MODEST_CONF_NAMESPACE "/msg_size_limit" /* int */
#define MODEST_CONF_PLAY_SOUND_MSG_ARRIVE MODEST_CONF_NAMESPACE "/play_sound_msg_arrive" /* bool */
#define MODEST_CONF_PREFER_FORMATTED_TEXT MODEST_CONF_NAMESPACE "/prefer_formatted_text" /* bool */
#define MODEST_CONF_REPLY_TYPE           MODEST_CONF_NAMESPACE "/reply_type"        /*  int  */
#define MODEST_CONF_FORWARD_TYPE         MODEST_CONF_NAMESPACE "/forward_type"      /*  int  */

/* Notification ids */
#define MODEST_CONF_NOTIFICATION_IDS MODEST_CONF_NAMESPACE "/notification_ids"      /* list of ints */


#define MODEST_EXAMPLE_EMAIL_ADDRESS "first.last@example.com"

#endif /*__MODEST_DEFS_H__*/
