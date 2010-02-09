/* Copyright (c) 2006-2009, Nokia Corporation
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

const gchar *modest_defs_dir (const gchar *string);
const gchar *modest_defs_namespace (const gchar *string);


/* Default paths. We set them this way so that we can define on runtime
 * different values */
#define MODEST_DEFAULT_DIR ".modest"
#define MODEST_DEFAULT_NAMESPACE "/apps/modest"
#define MODEST_DIR_ENV "MODEST_DIR"
#define MODEST_NAMESPACE_ENV "MODEST_GCONF_NAMESPACE"

/* Some interesting directories. NOTE, they should be prefixed
 * with $HOME; Also, except for MODEST_DIR itself, they
 * need to be prefixed with MODEST_DIR;
 * The parts of the path are separate for crossplatform
 * building of dirs from their components.
 * g_build_dir is your friend
 */
#define MODEST_DIR	                  modest_defs_dir (NULL)
#define MODEST_CACHE_DIR                  "cache"
#define MODEST_IMAGES_CACHE_DIR           "images"
#define MODEST_IMAGES_CACHE_SIZE          (1024*1024)

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

/* Without the trailing / because gnome-vfs reports mounted 
 * volume URIs without the trailing, and we want to match them: */
#define MODEST_MMC1_VOLUMEPATH_ENV	  "MMC_MOUNTPOINT" 
#define MODEST_MMC1_VOLUMEPATH_URI_PREFIX "file://"

/* configuration key definitions for modest */
#define MODEST_CONF_NAMESPACE		(modest_defs_namespace (NULL))

/* the mapping files, there are two possibilities; used in modest_utils_open_mcc_mapping_file */
#define MODEST_MCC_MAPPING                 PREFIX "/share/modest/provider-data/mcc_mapping"
#define MODEST_OPERATOR_WIZARD_MCC_MAPPING "/usr/share/operator-wizard/mcc_mapping"

#define MODEST_PROVIDER_DATA_FILE         PREFIX "/share/modest/provider-data/modest-provider-data.keyfile"  
#define MODEST_FALLBACK_PROVIDER_DATA_FILE	  PREFIX "/share/modest/provider-data/fallback-provider-data.keyfile"  


#ifndef MODEST_TOOLKIT_GTK
#ifdef MODEST_TOOLKIT_HILDON2
#define MODEST_ICON_SIZE_XSMALL           16
#define MODEST_ICON_SIZE_SMALL            24
#define MODEST_ICON_SIZE_BIG		  48
#else
#define MODEST_ICON_SIZE_XSMALL           16
#define MODEST_ICON_SIZE_SMALL            26
#define MODEST_ICON_SIZE_BIG		  64
#endif
#else
#define MODEST_ICON_SIZE_XSMALL           16
#define MODEST_ICON_SIZE_SMALL            16
#define MODEST_ICON_SIZE_BIG		  32
#endif

/* configuration key definitions for modest */
#define MODEST_ACCOUNT_SUBNAMESPACE      "/accounts"
#define MODEST_ACCOUNT_NAMESPACE         (modest_defs_namespace (MODEST_ACCOUNT_SUBNAMESPACE))
#define MODEST_CONF_DEFAULT_ACCOUNT      (modest_defs_namespace ("/default_account"))

/* Not used: #define MODEST_CONF_CONNECT_AT_STARTUP   MODEST_CONF_NAMESPACE "/connect_at_startup" */      

#define MODEST_CONF_SHOW_CC              (modest_defs_namespace ("/show_cc"))
#define MODEST_CONF_SHOW_BCC             (modest_defs_namespace ("/show_bcc"))

/* Last used folders for insert images, attach file and save attachments */
#define MODEST_CONF_LATEST_ATTACH_FILE_PATH (modest_defs_namespace ("/latest_attach_file_path"))
#define MODEST_CONF_LATEST_INSERT_IMAGE_PATH (modest_defs_namespace ("/latest_inset_image_path"))
#define MODEST_CONF_LATEST_SAVE_ATTACHMENT_PATH (modest_defs_namespace ("/latest_save_attachment_path"))

/* This is the alarmd cookie, obtained from alarm_event_add(), 
 * which apparently remains valid between application instances.
 * We store it so that we can remove it later.
 */
#define MODEST_CONF_ALARM_ID (modest_defs_namespace ("/alarm_id"))

/*
 * in the maemo case, we try to replace this
 * with the device name
 */
#define MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME  N_("Local folders")

/* the name of the device; in case of maemo this is set and updated
 * using dbus; see modest-maemo-utils.[ch]
 */
#define MODEST_CONF_DEVICE_NAME       (modest_defs_namespace ("/device_name"))


/* place for widget settings */
#define MODEST_CONF_WIDGET_SUBNAMESPACE  "/widgets"
#define MODEST_CONF_WIDGET_NAMESPACE     (modest_defs_namespace (MODEST_CONF_WIDGET_SUBNAMESPACE))
#define MODEST_CONF_FOLDER_VIEW_KEY      "folder-view"
#define MODEST_CONF_HEADER_VIEW_KEY      "header-view"
#define MODEST_CONF_MAIN_PANED_KEY       "modest-main-paned"
#define MODEST_CONF_MSG_PANED_KEY        "modest-msg-paned"
#define MODEST_CONF_FOLDER_PANED_KEY     "modest-folder-paned"
#define MODEST_CONF_MAIN_WINDOW_KEY      "modest-main-window"
#define MODEST_CONF_EDIT_WINDOW_KEY      "modest-edit-msg-window"
#define MODEST_CONF_MSG_VIEW_WINDOW_KEY  "modest-msg-view-window"

#define MODEST_SERVER_ACCOUNT_SUBNAMESPACE "/server_accounts"
#define MODEST_SERVER_ACCOUNT_NAMESPACE  (modest_defs_namespace (MODEST_SERVER_ACCOUNT_SUBNAMESPACE))

/* show toolbar settings */
#define MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR \
	(modest_defs_namespace (MODEST_CONF_WIDGET_SUBNAMESPACE "/"	\
				MODEST_CONF_MAIN_WINDOW_KEY "/show_toolbar"))
#define MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR \
	(modest_defs_namespace (MODEST_CONF_WIDGET_SUBNAMESPACE "/"	\
				MODEST_CONF_EDIT_WINDOW_KEY "/show_toolbar"))
#define MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR \
	(modest_defs_namespace (MODEST_CONF_WIDGET_SUBNAMESPACE "/"	\
				MODEST_CONF_MSG_VIEW_WINDOW_KEY "/show_toolbar"))
#define MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR_FULLSCREEN \
	(modest_defs_namespace (MODEST_CONF_WIDGET_SUBNAMESPACE "/"	\
				MODEST_CONF_MAIN_WINDOW_KEY "/show_toolbar_fullscreen"))
#define MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR_FULLSCREEN			\
	(modest_defs_namespace (MODEST_CONF_WIDGET_SUBNAMESPACE "/"	\
				MODEST_CONF_EDIT_WINDOW_KEY "/show_toolbar_fullscreen"))
#define MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR_FULLSCREEN		\
	(modest_defs_namespace (MODEST_CONF_WIDGET_SUBNAMESPACE "/"	\
				MODEST_CONF_MSG_VIEW_WINDOW_KEY "/show_toolbar_fullscreen"))

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
#define MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST \
	(modest_defs_namespace ("/specific_smtp")) /* one list used for all accounts. */
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
#define MODEST_ACCOUNT_HAS_NEW_MAILS     "has_new_mails"     /* boolean */

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
#define MODEST_CONF_AUTO_UPDATE (modest_defs_namespace ("/auto_update")) /* bool */
#define MODEST_CONF_UPDATE_WHEN_CONNECTED_BY (modest_defs_namespace ("/update_when_connected_by")) /* int */
#define MODEST_CONF_UPDATE_INTERVAL (modest_defs_namespace ("/update_interval")) /* int */
#define MODEST_CONF_MSG_SIZE_LIMIT (modest_defs_namespace ("/msg_size_limit")) /* int */
#define MODEST_CONF_PLAY_SOUND_MSG_ARRIVE (modest_defs_namespace ("/play_sound_msg_arrive")) /* bool */
#define MODEST_CONF_PREFER_FORMATTED_TEXT (modest_defs_namespace ("/prefer_formatted_text")) /* bool */
#define MODEST_CONF_REPLY_TYPE           (modest_defs_namespace ("/reply_type"))        /*  int  */
#define MODEST_CONF_FORWARD_TYPE         (modest_defs_namespace  ("/forward_type"))      /*  int  */
#define MODEST_CONF_NOTIFICATIONS (modest_defs_namespace ("/notifications")) /* bool */
#define MODEST_CONF_AUTO_ADD_TO_CONTACTS (modest_defs_namespace ("/auto_add_to_contacs")) /* bool */

/* hidden global settings */
#define MODEST_CONF_FETCH_HTML_EXTERNAL_IMAGES (modest_defs_namespace ("/fetch_external_images")) /* bool */

/* Notification ids */
#define MODEST_CONF_NOTIFICATION_IDS (modest_defs_namespace ("/notification_ids"))      /* list of ints */
#define MODEST_ACCOUNT_NOTIFICATION_IDS "notification_ids"      /* list of ints */

#ifdef MODEST_TOOLKIT_HILDON2
#define MODEST_EXAMPLE_EMAIL_ADDRESS _("mcen_va_example_email_address")
#else
#define MODEST_EXAMPLE_EMAIL_ADDRESS "first.last@example.com"
#endif


/* max size of message we still allow to save/send when we're in low-mem
 * condition
 */
#define MODEST_MAX_LOW_MEMORY_MESSAGE_SIZE (25*1024)
#define MODEST_MAX_ATTACHMENT_SIZE (15*1024*1024)

#endif /*__MODEST_DEFS_H__*/
