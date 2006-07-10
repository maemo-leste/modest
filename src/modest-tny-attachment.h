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


/* modest-tny-attachment.h */

#ifndef __MODEST_TNY_ATTACHMENT_H__
#define __MODEST_TNY_ATTACHMENT_H__

#include <glib-object.h>
#include <tny-stream-iface.h>
#include <tny-msg-iface.h>
#include <tny-msg-mime-part-iface.h>
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_ATTACHMENT             (modest_tny_attachment_get_type())
#define MODEST_TNY_ATTACHMENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_ATTACHMENT,ModestTnyAttachment))
#define MODEST_TNY_ATTACHMENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_ATTACHMENT,GObject))
#define MODEST_IS_TNY_ATTACHMENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_ATTACHMENT))
#define MODEST_IS_TNY_ATTACHMENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_ATTACHMENT))
#define MODEST_TNY_ATTACHMENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_ATTACHMENT,ModestTnyAttachmentClass))

typedef struct _ModestTnyAttachment      ModestTnyAttachment;
typedef struct _ModestTnyAttachmentClass ModestTnyAttachmentClass;

struct _ModestTnyAttachment {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestTnyAttachmentClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestTnyAttachment* obj); */
};

/* member functions */
GType        modest_tny_attachment_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget*, */
/*    otherwise probably a GObject*. */
ModestTnyAttachment*    modest_tny_attachment_new         (void);

/* fill in other public functions, eg.: */
/* 	void       modest_tny_attachment_do_something (ModestTnyAttachment *self, const gchar* param); */
/* 	gboolean   modest_tny_attachment_has_foo      (ModestTnyAttachment *self, gint value); */

void modest_tny_attachment_set_name (ModestTnyAttachment *self, const gchar * thing);
const gchar *modest_tny_attachment_get_name (ModestTnyAttachment *self);

void modest_tny_attachment_set_filename (ModestTnyAttachment *self, const gchar * thing);
const gchar *modest_tny_attachment_get_filename (ModestTnyAttachment *self);

void modest_tny_attachment_set_mime_type (ModestTnyAttachment *self, const gchar * thing);
const gchar *modest_tny_attachment_get_mime_type (ModestTnyAttachment *self);

void modest_tny_attachment_guess_mime_type (ModestTnyAttachment *self);

TnyStreamIface * modest_tny_attachment_get_stream (ModestTnyAttachment *self);

void modest_tny_attachment_free_list(GList *list);

GList *modest_tny_attachment_new_list_from_msg(const TnyMsgIface *msg, gboolean with_body);

G_END_DECLS

#endif /* __MODEST_TNY_ATTACHMENT_H__ */

