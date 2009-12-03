/* Copyright (c) 2008, Nokia Corporation
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

#include <modest-shell-banner.h>
#include <modest-gtk-window-mgr.h>
#include <modest-runtime.h>
#include <glib-object.h>

#define _DEFAULT_TIMEOUT 5000

/* 'private'/'protected' functions */
static void modest_shell_banner_class_init  (gpointer klass, gpointer class_data);
static void modest_shell_banner_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_shell_banner_dispose     (GObject *obj);
static void modest_shell_banner_finalize     (GObject *obj);
static gboolean _on_timeout (ModestShellBanner *self);

typedef struct _ModestShellBannerPrivate ModestShellBannerPrivate;
struct _ModestShellBannerPrivate {
	GtkWidget *label;
	guint timeout_id;
};
#define MODEST_SHELL_BANNER_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									 MODEST_TYPE_SHELL_BANNER, \
									 ModestShellBannerPrivate))

/* globals */
static GtkFrameClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

/************************************************************************/

GType
modest_shell_banner_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestShellBannerClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_shell_banner_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestShellBanner),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_shell_banner_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_SHELL_BANNER,
		                                  "ModestShellBanner",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_shell_banner_class_init (gpointer klass, gpointer class_data)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->dispose  = modest_shell_banner_dispose;
	gobject_class->finalize  = modest_shell_banner_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestShellBannerPrivate));
	

}

static void
modest_shell_banner_dispose (GObject *obj)
{
	ModestShellBannerPrivate *priv;
	priv = MODEST_SHELL_BANNER_GET_PRIVATE (obj);
	if (priv->timeout_id) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}
	G_OBJECT_CLASS(parent_class)->dispose (obj);
}

static void
modest_shell_banner_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_shell_banner_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestShellBannerPrivate *priv;
	GtkWidget *hbox;

	ModestShellBanner *self = (ModestShellBanner *) instance;
	priv = MODEST_SHELL_BANNER_GET_PRIVATE (self);

	gtk_frame_set_shadow_type (GTK_FRAME (self), GTK_SHADOW_OUT);
	priv->label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0.5, 0.5);
	gtk_widget_show (priv->label);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), priv->label, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	gtk_container_add (GTK_CONTAINER (self), hbox);
}

GtkWidget *
modest_shell_banner_new (GtkWidget *parent)
{
	return modest_shell_banner_new_with_timeout (parent, _DEFAULT_TIMEOUT);
}

GtkWidget *
modest_shell_banner_new_with_timeout (GtkWidget *parent, gint timeout)
{
	GtkWidget *self;
	GtkWidget *toplevel;
	ModestShellBannerPrivate *priv;

	self = g_object_new (MODEST_TYPE_SHELL_BANNER, NULL);
	priv = MODEST_SHELL_BANNER_GET_PRIVATE (self);

	if (parent)
		toplevel = gtk_widget_get_toplevel (parent);
	else
		toplevel = NULL;

	if (toplevel == NULL) {
		GtkWidget *shell;
		shell = modest_gtk_window_mgr_get_shell (MODEST_GTK_WINDOW_MGR (modest_runtime_get_window_mgr ()));
		modest_shell_add_banner (MODEST_SHELL (shell), MODEST_SHELL_BANNER (self));
	} else if (MODEST_IS_SHELL (toplevel)) {
		modest_shell_add_banner (MODEST_SHELL (toplevel), MODEST_SHELL_BANNER (self));
	} else if (GTK_IS_DIALOG (toplevel)) {
		gtk_container_add (GTK_CONTAINER (GTK_DIALOG (toplevel)->vbox), self);
		gtk_container_child_set (GTK_CONTAINER (GTK_DIALOG (toplevel)->vbox), self,
					 "position", 0, NULL);
	}
	
	gtk_widget_show (self);

	if (timeout != 0)
		priv->timeout_id = g_timeout_add (timeout, (GSourceFunc) _on_timeout, self);
	else
		priv->timeout_id = 0;

	return self;
}

void
modest_shell_banner_set_icon (ModestShellBanner *self, const gchar *icon_name)
{
	return;
}

void
modest_shell_banner_set_text (ModestShellBanner *self, const gchar *label)
{
	ModestShellBannerPrivate *priv;

	priv = MODEST_SHELL_BANNER_GET_PRIVATE (self);

	gtk_label_set_text (GTK_LABEL (priv->label), label);
}

void
modest_shell_banner_set_animation (ModestShellBanner *self, const gchar *animation_name)
{
	return;
}

static gboolean
_on_timeout (ModestShellBanner *self)
{
	ModestShellBannerPrivate *priv;

	priv = MODEST_SHELL_BANNER_GET_PRIVATE (self);
	if (priv->timeout_id) {
		priv->timeout_id = 0;
	}
	gtk_widget_destroy (GTK_WIDGET (self));
	return FALSE;
}
