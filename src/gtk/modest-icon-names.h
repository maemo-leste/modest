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


/* modest-icon-names.h */

/*
 * FIXME: this should go the front-end dirs,
 * with specific icons
 */

#ifndef __MODEST_ICON_NAMES_H__
#define __MODEST_ICON_NAMES_H__


/* icons */

#define MODEST_APP_ICON				PIXMAP_PREFIX "modest-icon.png"

#define MODEST_HEADER_ICON_READ			PIXMAP_PREFIX "qgn_list_messagin_mail.png"
#define MODEST_HEADER_ICON_UNREAD		PIXMAP_PREFIX "qgn_list_messagin_mail_unread.png"
#define MODEST_HEADER_ICON_DELETED		PIXMAP_PREFIX "qgn_list_messagin_mail_deleted.png"
/* #define MODEST_HEADER_ICON_ATTACH		PIXMAP_PREFIX "qgn_list_gene_attacpap.png" */
#define MODEST_HEADER_ICON_ATTACH		"stock_attach"


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
#define MODEST_HEADER_ICON_HIGH        "stock_mail-priority-high"
#define MODEST_HEADER_ICON_LOW         "stock_mail-priority-low"
/*
 *
 */

#define MODEST_FOLDER_ICON_OPEN			"folder_open"
#define MODEST_FOLDER_ICON_CLOSED		"folder"

#define MODEST_FOLDER_ICON_ACCOUNT		"network-server"
#define MODEST_FOLDER_ICON_INBOX		"stock_inbox"
#define MODEST_FOLDER_ICON_OUTBOX		"stock_outbox"
#define MODEST_FOLDER_ICON_SENT			"stock_sent-mail"
#define MODEST_FOLDER_ICON_TRASH		"user-trash"
#define MODEST_FOLDER_ICON_JUNK			"stock_spam"
#define MODEST_FOLDER_ICON_DRAFTS		"stock_new-text"
#define MODEST_FOLDER_ICON_NORMAL		"folder"
/* #define MODEST_FOLDER_ICON_LOCAL_FOLDERS	PIXMAP_PREFIX "qgn_list_gene_fldr_cls.png" */
#define MODEST_FOLDER_ICON_LOCAL_FOLDERS	"computer"
#define MODEST_FOLDER_ICON_MMC                  "media-flash"
#define MODEST_FOLDER_ICON_MMC_FOLDER           MODEST_FOLDER_ICON_NORMAL
#define MODEST_FOLDER_ICON_REMOTE_FOLDER	MODEST_FOLDER_ICON_NORMAL

/* toolbar */
#define  MODEST_TOOLBAR_ICON_MAIL_SEND		"mail_send"
#define  MODEST_TOOLBAR_ICON_NEW_MAIL		"mail_new"
/* #define  MODEST_TOOLBAR_ICON_SEND_RECEIVE	PIXMAP_PREFIX "gtk-refresh.png"  */
#define  MODEST_TOOLBAR_ICON_REPLY		"mail_reply"
#define  MODEST_TOOLBAR_ICON_REPLY_ALL		"mail-reply-all"
#define  MODEST_TOOLBAR_ICON_FORWARD		"mail-forward"
#define  MODEST_TOOLBAR_ICON_DELETE		GTK_STOCK_DELETE
#define  MODEST_TOOLBAR_ICON_NEXT		GTK_STOCK_GO_FORWARD
#define  MODEST_TOOLBAR_ICON_PREV		GTK_STOCK_GO_BACK
#define  MODEST_TOOLBAR_ICON_STOP		GTK_STOCK_STOP
#define  MODEST_TOOLBAR_ICON_FORMAT_BULLETS     GTK_STOCK_INDENT
#define  MODEST_TOOLBAR_ICON_SPLIT_VIEW         PIXMAP_PREFIX "folder"
#define  MODEST_TOOLBAR_ICON_BOLD               GTK_STOCK_BOLD
#define  MODEST_TOOLBAR_ICON_ITALIC             GTK_STOCK_ITALIC
#define  MODEST_TOOLBAR_ICON_FIND               GTK_STOCK_FIND
#define  MODEST_TOOLBAR_ICON_DOWNLOAD_IMAGES    GTK_STOCK_MISSING_IMAGE

/* Stock icon names */
#define  MODEST_STOCK_MAIL_SEND	   "modest-stock-mail-send"
#define  MODEST_STOCK_NEW_MAIL	   "modest-stock-new-mail"
#define  MODEST_STOCK_SEND_RECEIVE "modest-stock-send-receive"
#define  MODEST_STOCK_REPLY        "modest-stock-reply"
#define  MODEST_STOCK_REPLY_ALL	   "modest-stock-reply-all"
#define  MODEST_STOCK_FORWARD      "modest-stock-forward"
#define  MODEST_STOCK_DELETE       "modest-stock-delete"
#define  MODEST_STOCK_NEXT         "modest-stock-next"
#define  MODEST_STOCK_PREV         "modest-stock-prev"
#define  MODEST_STOCK_STOP         "modest-stock-stop"
#define  MODEST_STOCK_SPLIT_VIEW   "modest-stock-split-view"

#endif  /*__MODEST_ICON_NAMES_H__*/
