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
#define MODEST_HEADER_ICON_ATTACH		PIXMAP_PREFIX "qgn_list_gene_attacpap.png"


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
#define MODEST_HEADER_ICON_HIGH        PIXMAP_PREFIX"modest_high_no_attachment.png"
#define MODEST_HEADER_ICON_LOW         PIXMAP_PREFIX"modest_low_no_attachment.png"
/*
 *
 */

#define MODEST_FOLDER_ICON_OPEN			PIXMAP_PREFIX "qgn_list_gene_fldr_opn.png"
#define MODEST_FOLDER_ICON_CLOSED		PIXMAP_PREFIX "qgn_list_gene_fldr_cls.png"

#define MODEST_FOLDER_ICON_ACCOUNT		PIXMAP_PREFIX "qgn_addr_icon_user_group.png"
#define MODEST_FOLDER_ICON_INBOX		PIXMAP_PREFIX "qgn_list_messagin_inbox.png"
#define MODEST_FOLDER_ICON_OUTBOX		PIXMAP_PREFIX "qgn_list_messagin_outbox.png"
#define MODEST_FOLDER_ICON_SENT			PIXMAP_PREFIX "qgn_list_messagin_sent.png"
#define MODEST_FOLDER_ICON_TRASH		PIXMAP_PREFIX "qgn_list_messagin_mail_deleted.png"
#define MODEST_FOLDER_ICON_JUNK			PIXMAP_PREFIX "qgn_list_messagin_mail_deleted.png"
#define MODEST_FOLDER_ICON_DRAFTS		PIXMAP_PREFIX "qgn_list_messagin_drafts.png"
#define MODEST_FOLDER_ICON_NORMAL		PIXMAP_PREFIX "qgn_list_gene_fldr_cls.png"
#define MODEST_FOLDER_ICON_LOCAL_FOLDERS	PIXMAP_PREFIX "qgn_list_gene_fldr_cls.png"
#define MODEST_FOLDER_ICON_MMC                  PIXMAP_PREFIX "qgn_list_gene_mmc.png"


/* toolbar */
#define  MODEST_TOOLBAR_ICON_MAIL_SEND		PIXMAP_PREFIX "qgn_list_messagin_sent.png"
#define  MODEST_TOOLBAR_ICON_NEW_MAIL		PIXMAP_PREFIX "mail-message-new.png"
/* #define  MODEST_TOOLBAR_ICON_SEND_RECEIVE	PIXMAP_PREFIX "gtk-refresh.png"  */
#define  MODEST_TOOLBAR_ICON_REPLY		PIXMAP_PREFIX "mail-reply-sender.png"
#define  MODEST_TOOLBAR_ICON_REPLY_ALL		PIXMAP_PREFIX "mail-reply-all.png"
#define  MODEST_TOOLBAR_ICON_FORWARD		PIXMAP_PREFIX "mail-forward.png"
#define  MODEST_TOOLBAR_ICON_DELETE		PIXMAP_PREFIX "mail-mark-junk.png" 
/* #define  MODEST_TOOLBAR_ICON_NEXT		PIXMAP_PREFIX "forward.png" */
/* #define  MODEST_TOOLBAR_ICON_PREV		PIXMAP_PREFIX "back.png" */
#define  MODEST_TOOLBAR_ICON_STOP		PIXMAP_PREFIX "stock-stop.png"
#define  MODEST_TOOLBAR_ICON_FORMAT_BULLETS     PIXMAP_PREFIX "qgn_list_gene_bullets"
#define  MODEST_TOOLBAR_ICON_BOLD               GTK_STOCK_BOLD
#define  MODEST_TOOLBAR_ICON_ITALIC             GTK_STOCK_ITALIC

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

#endif  /*__MODEST_ICON_NAMES_H__*/
