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


/* modest-tny-icon-names.h */


#ifndef __MODEST_TNY_ICON_NAMES_H__
#define __MODEST_TNY_ICON_NAMES_H__

/* icons */

#define MODEST_APP_ICON				"general_email"
#define MODEST_APP_MSG_VIEW_ICON		"email_message_viewer"
#define MODEST_APP_MSG_EDIT_ICON		"email_message_editor"

#define MODEST_HEADER_ICON_READ			""
#define MODEST_HEADER_ICON_UNREAD		""
#define MODEST_HEADER_ICON_DELETED		""
#define MODEST_HEADER_ICON_ATTACH		"email_attachment"
#define MODEST_HEADER_ICON_HIGH	                "email_message_high_priority"
#define MODEST_HEADER_ICON_LOW   		"email_message_low_priority"

/*
 * until we have the custom cell renderer, we use the hacked icons below;
 * don't remove!!!
 */ 
#define MODEST_HEADER_ICON_ATTACH_HIGH_PRIORITY PIXMAP_PREFIX"modest_high_attachment.png"
#define MODEST_HEADER_ICON_ATTACH_LOW_PRIORITY  PIXMAP_PREFIX"modest_low_attachment.png"
#define MODEST_HEADER_ICON_ATTACH_NORM_PRIORITY PIXMAP_PREFIX"modest_normal_attachment.png"
#define MODEST_HEADER_ICON_HIGH_PRIORITY        PIXMAP_PREFIX"modest_high_no_attachment.png"
#define MODEST_HEADER_ICON_LOW_PRIORITY         PIXMAP_PREFIX"modest_low_no_attachment.png"
#define MODEST_HEADER_ICON_NORM_PRIORITY        PIXMAP_PREFIX"modest_normal_no_attachment.png"
/*
 *
 */

#define MODEST_FOLDER_ICON_OPEN			"qgn_list_gene_fldr_opn"
#define MODEST_FOLDER_ICON_CLOSED		"qgn_list_gene_fldr_cls"

#define MODEST_FOLDER_ICON_ACCOUNT		"general_web"
#define MODEST_FOLDER_ICON_INBOX		"email_inbox"
#define MODEST_FOLDER_ICON_OUTBOX		"general_outbox"
#define MODEST_FOLDER_ICON_SENT			"general_sent"
#define MODEST_FOLDER_ICON_TRASH		"general_folder"
#define MODEST_FOLDER_ICON_JUNK			"general_folder"
#define MODEST_FOLDER_ICON_DRAFTS		"email_message_drafts"
#define MODEST_FOLDER_ICON_NORMAL		"general_folder"
#define MODEST_FOLDER_ICON_LOCAL_FOLDERS	"general_device_root_folder"
#define MODEST_FOLDER_ICON_MMC                  "general_removable_memory_card"
#define MODEST_FOLDER_ICON_MMC_FOLDER           "general_removable_memory_card"
#define MODEST_FOLDER_ICON_REMOTE_FOLDER	"email_remote_folder"

/* toolbar */
#define  MODEST_TOOLBAR_ICON_MAIL_SEND		"email_message_send"
#define  MODEST_TOOLBAR_ICON_NEW_MAIL		"email_message_editor"
#define  MODEST_TOOLBAR_ICON_SEND_RECEIVE	"general_refresh"
#define  MODEST_TOOLBAR_ICON_REPLY		"email_message_reply"
#define  MODEST_TOOLBAR_ICON_REPLY_ALL		"email_message_reply_all"
#define  MODEST_TOOLBAR_ICON_FORWARD		"email_message_forward"
#define  MODEST_TOOLBAR_ICON_DELETE		"general_delete"
#define  MODEST_TOOLBAR_ICON_FORMAT_BULLETS     ""
#define  MODEST_TOOLBAR_ICON_SPLIT_VIEW         "general_foldertree"
#define  MODEST_TOOLBAR_ICON_SORT               "general_sort"
#define  MODEST_TOOLBAR_ICON_REFRESH            "general_refresh"
#define  MODEST_TOOLBAR_ICON_MOVE_TO_FOLDER     "general_move_to_folder"
#define  MODEST_TOOLBAR_ICON_BOLD               "general_bold"
#define  MODEST_TOOLBAR_ICON_ITALIC             "general_italic"
#define  MODEST_TOOLBAR_ICON_NEXT               "general_forward"
#define  MODEST_TOOLBAR_ICON_PREV               "general_back"
#define  MODEST_TOOLBAR_ICON_FIND               "general_search"
#define  MODEST_TOOLBAR_ICON_DOWNLOAD_IMAGES    "email_download_external_images"

/* Stock icon names */
#define  MODEST_STOCK_MAIL_SEND    "modest-stock-mail-send"
#define  MODEST_STOCK_NEW_MAIL     "modest-stock-new-mail"
#define  MODEST_STOCK_SEND_RECEIVE "modest-stock-send-receive"
#define  MODEST_STOCK_REPLY        "modest-stock-reply"
#define  MODEST_STOCK_REPLY_ALL    "modest-stock-reply-all"
#define  MODEST_STOCK_FORWARD      "modest-stock-forward"
#define  MODEST_STOCK_DELETE       "modest-stock-delete"
#define  MODEST_STOCK_NEXT         "modest-stock-next"
#define  MODEST_STOCK_PREV         "modest-stock-prev"
#define  MODEST_STOCK_STOP         "modest-stock-stop"
#define  MODEST_STOCK_SPLIT_VIEW   "modest-stock-split-view"
#define  MODEST_STOCK_SORT         "modest-stock-sort"
#define  MODEST_STOCK_REFRESH      "modest-stock-refresh"

#endif  /*__MODEST_TNY_ICON_NAMES_H__*/
