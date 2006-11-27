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

#ifndef __MODEST_TOOLBAR_H__
#define __MODEST_TOOLBAR_H__

#include <gtk/gtk.h>
#include <modest-icon-factory.h>
#include <modest-icon-names.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TOOLBAR             (modest_toolbar_get_type())
#define MODEST_TOOLBAR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TOOLBAR,ModestToolbar))
#define MODEST_TOOLBAR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TOOLBAR,GtkToolbar))
#define MODEST_IS_TOOLBAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TOOLBAR))
#define MODEST_IS_TOOLBAR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TOOLBAR))
#define MODEST_TOOLBAR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TOOLBAR,ModestToolbarClass))

typedef struct _ModestToolbar      ModestToolbar;
typedef struct _ModestToolbarClass ModestToolbarClass;

/* typedef enum _ModestToolbarButton ModestToolbarButton; */
typedef enum _ModestToolbarButton {
	MODEST_TOOLBAR_BUTTON_MAIL_SEND,
	MODEST_TOOLBAR_BUTTON_NEW_MAIL,
	MODEST_TOOLBAR_BUTTON_REPLY,
	MODEST_TOOLBAR_BUTTON_REPLY_ALL,
	MODEST_TOOLBAR_BUTTON_FORWARD,
	MODEST_TOOLBAR_BUTTON_PRINT,
	MODEST_TOOLBAR_BUTTON_DELETE,
	MODEST_TOOLBAR_BUTTON_NEXT,
	MODEST_TOOLBAR_BUTTON_PREV,
	MODEST_TOOLBAR_BUTTON_STOP,
	MODEST_TOOLBAR_BUTTON_SEND_RECEIVE,

	MODEST_TOOLBAR_SEPARATOR,
	MODEST_TOOLBAR_BUTTON_NUM
} ModestToolbarButton;

struct _ModestToolbar {
	 GtkToolbar parent;
	/* insert public members, if any */
};

struct _ModestToolbarClass {
	GtkToolbarClass parent_class;
	
	void (* button_clicked) (ModestToolbar* obj, ModestToolbarButton button_id);
};

/* member functions */
GType        modest_toolbar_get_type    (void) G_GNUC_CONST;

ModestToolbar*   modest_toolbar_new     (const GSList *buttons);

G_END_DECLS

#endif /* __MODEST_TOOLBAR_H__ */

