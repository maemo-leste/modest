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

#define MODEST_APP_ICON				"qgn_list_messagin"
#define MODEST_APP_MSG_VIEW_ICON		"qgn_list_messagin_viewer"
#define MODEST_APP_MSG_EDIT_ICON		"qgn_list_messagin_editor"

#define MODEST_HEADER_ICON_READ			"qgn_list_messagin_mail"
#define MODEST_HEADER_ICON_UNREAD		"qgn_list_messagin_mail_unread"
#define MODEST_HEADER_ICON_DELETED		"qgn_list_messagin_mail_deleted"
#define MODEST_HEADER_ICON_ATTACH		"qgn_list_gene_attacpap"

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

#define MODEST_FOLDER_ICON_ACCOUNT		"qgn_list_browser"
#define MODEST_FOLDER_ICON_INBOX		"qgn_list_messagin_inbox"
#define MODEST_FOLDER_ICON_OUTBOX		"qgn_list_messagin_outbox"
#define MODEST_FOLDER_ICON_SENT			"qgn_list_messagin_sent"
#define MODEST_FOLDER_ICON_TRASH		"qgn_toolb_messagin_delete"
#define MODEST_FOLDER_ICON_JUNK			"qgn_toolb_messagin_delete"
#define MODEST_FOLDER_ICON_DRAFTS		"qgn_list_messagin_drafts"
#define MODEST_FOLDER_ICON_NORMAL		"qgn_list_gene_fldr_cls"
#define MODEST_FOLDER_ICON_LOCAL_FOLDERS	"qgn_list_shell_mydevice"
#define MODEST_FOLDER_ICON_MMC                  "qgn_list_gene_mmc"


/* toolbar */
#define  MODEST_TOOLBAR_ICON_MAIL_SEND		"qgn_list_messagin_sent"
#define  MODEST_TOOLBAR_ICON_NEW_MAIL		"qgn_toolb_messagin_new"
#define  MODEST_TOOLBAR_ICON_SEND_RECEIVE	"qgn_toolb_messagin_sendreceive"
#define  MODEST_TOOLBAR_ICON_REPLY		"qgn_toolb_messagin_reply"
#define  MODEST_TOOLBAR_ICON_REPLY_ALL		"qgn_toolb_messagin_replytoall"
#define  MODEST_TOOLBAR_ICON_FORWARD		"qgn_toolb_messagin_forward"
#define  MODEST_TOOLBAR_ICON_DELETE		"qgn_toolb_gene_deletebutton"
#define  MODEST_TOOLBAR_ICON_FORMAT_BULLETS     "qgn_list_gene_bullets"
#define  MODEST_TOOLBAR_ICON_SPLIT_VIEW         "qgn_toolb_rss_fldonoff"
#define  MODEST_TOOLBAR_ICON_SORT               "qgn_list_sort"
#define  MODEST_TOOLBAR_ICON_REFRESH            "qgn_toolb_gene_refresh"
#define  MODEST_TOOLBAR_ICON_MOVE_TO_FOLDER     "qgn_toolb_gene_movetofldr"

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
