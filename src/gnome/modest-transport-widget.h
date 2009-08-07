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

#ifndef __MODEST_TRANSPORT_WIDGET_H__
#define __MODEST_TRANSPORT_WIDGET_H__

G_BEGIN_DECLS

#include <gtk/gtk.h>
#include "modest-protocol-registry.h"

/* convenience macros */
#define MODEST_TYPE_TRANSPORT_WIDGET             (modest_transport_widget_get_type())
#define MODEST_TRANSPORT_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TRANSPORT_WIDGET,ModestTransportWidget))
#define MODEST_TRANSPORT_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TRANSPORT_WIDGET,GtkContainer))
#define MODEST_IS_TRANSPORT_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TRANSPORT_WIDGET))
#define MODEST_IS_TRANSPORT_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TRANSPORT_WIDGET))
#define MODEST_TRANSPORT_WIDGET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TRANSPORT_WIDGET,ModestTransportWidgetClass))

typedef struct _ModestTransportWidget      ModestTransportWidget;
typedef struct _ModestTransportWidgetClass ModestTransportWidgetClass;

struct _ModestTransportWidget {
	 GtkVBox parent;
	/* insert public members, if any */
};

struct _ModestTransportWidgetClass {
	GtkVBoxClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestTransportWidget* obj); */
};

/* member functions */
GType        modest_transport_widget_get_type    (void) G_GNUC_CONST;

GtkWidget*   modest_transport_widget_new         (ModestProtocolType proto);

gboolean        modest_transport_widget_get_remember_password (ModestTransportWidget *self);
gboolean        modest_transport_widget_get_requires_auth     (ModestTransportWidget *self);
const gchar*    modest_transport_widget_get_username          (ModestTransportWidget *self);
const gchar*    modest_transport_widget_get_servername        (ModestTransportWidget *self);
ModestProtocolType  modest_transport_widget_get_proto             (ModestTransportWidget *self);

G_END_DECLS

#endif /* __MODEST_TRANSPORT_WIDGET_H__ */

