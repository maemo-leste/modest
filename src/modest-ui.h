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

#ifndef __MODEST_UI_H__
#define __MODEST_UI_H__

#include <glib-object.h>
#include <gtk/gtkcontainer.h>
#include "modest-conf.h"
#include "modest-tny-msg-view.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_UI             (modest_ui_get_type())
#define MODEST_UI(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_UI,ModestUI))
#define MODEST_UI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_UI,GObject))
#define MODEST_IS_UI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_UI))
#define MODEST_IS_UI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_UI))
#define MODEST_UI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_UI,ModestUIClass))

typedef struct _ModestUI      ModestUI;
typedef struct _ModestUIClass ModestUIClass;

struct _ModestUI {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestUIClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestUI* obj); */
};


/**
 * modest_ui_get_type:
 * 
 * get the GType for ModestUI
 *  
 * Returns: the GType
 */
GType        modest_ui_get_type        (void) G_GNUC_CONST;



/**
 * modest_ui_new:
 * @modest_conf: a ModestConf instance 
 *  
 * Returns: a new ModestUI, or NULL in case of error
 */
ModestUI*     modest_ui_new             (ModestConf *modest_conf);


/**
 * modest_ui_show_main_window:
 * @ui: a ModestUI instance 
 * 
 * show the application's main window
 * 
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean     modest_ui_show_main_window (ModestUI *ui);


/**
 * modest_ui_new_edit_window:
 * @ui: a ModestUI instance 
 * @to: people to send this to, ';' separated
 * @cc: people to send carbon-copies (cc), ';' separated
 * @bcc people to send blind-carbon-copies (bcc),';' separated
 * @subject: the subject of the message
 * @body: the body text of the message
 * @att: a list with the filepaths for attachments
 *  
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean     modest_ui_new_edit_window (ModestUI *ui,
					 const gchar* to,
					 const gchar* cc,
					 const gchar* bcc,
					 const gchar* subject,
					 const gchar* body,
					 const GSList* att);


GtkContainer *modest_ui_new_editor_window (ModestUI *modest_ui, gpointer *user_data);

gboolean modest_ui_editor_window_set_to_header(const gchar *to, gpointer window_data);
gboolean modest_ui_editor_window_set_cc_header(const gchar *cc, gpointer window_data);
gboolean modest_ui_editor_window_set_bcc_header(const gchar *bcc, gpointer window_data);
gboolean modest_ui_editor_window_set_subject_header(const gchar *subject, gpointer window_data);
gboolean modest_ui_editor_window_set_body(const gchar *body, gpointer window_data);
gboolean modest_ui_editor_window_update_attachments(gpointer window_data);

GtkContainer *modest_ui_new_viewer_window (ModestUI *modest_ui, GtkWidget *msg_view,
					   TnyMsgIface *msg, gpointer *user_data);

/*
 * the new API below
 */
GtkWidget *modest_ui_main_window (ModestUI *modest_ui);

G_END_DECLS
#endif /* __MODEST_UI_H__ */
