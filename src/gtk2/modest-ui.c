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

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include "../modest-ui.h"
#include "../modest-account-mgr.h"
#include "../modest-widget-factory.h"
#include "modest-main-window.h"


/* 'private'/'protected' functions */
static void   modest_ui_class_init     (ModestUIClass *klass);
static void   modest_ui_init           (ModestUI *obj);
static void   modest_ui_finalize       (GObject *obj);

gchar *on_password_requested (TnyAccountIface *, const gchar *, gboolean *);


typedef struct _ModestUIPrivate ModestUIPrivate;
struct _ModestUIPrivate {
	ModestConf            *conf;
        ModestAccountMgr      *account_mgr;
	ModestWidgetFactory   *widget_factory;	
        ModestTnyAccountStore *account_store;
};

#define MODEST_UI_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MODEST_TYPE_UI, \
                                       ModestUIPrivate))


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

/* globals */
static GObjectClass *parent_class = NULL;


GType
modest_ui_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestUIClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_ui_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestUI),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_ui_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestUI",
		                                  &my_info, 0);
	}
	return my_type;
}


static void
modest_ui_class_init (ModestUIClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_ui_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestUIPrivate));

}


static void
modest_ui_init (ModestUI *obj)
{
 	ModestUIPrivate *priv;

	priv = MODEST_UI_GET_PRIVATE(obj);

	priv->account_store  = NULL;
	priv->account_mgr    = NULL;
	priv->conf           = NULL;
	priv->widget_factory = NULL;
}


static void
modest_ui_finalize (GObject *obj)
{
	
	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);

	if (priv->account_store) {
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}
		
	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	if (priv->widget_factory) {
		g_object_unref (G_OBJECT(priv->widget_factory));
		priv->widget_factory = NULL;
	}

	if (priv->conf) {
		g_object_unref (G_OBJECT(priv->conf));
		priv->conf = NULL;
	}	
}


ModestUI*
modest_ui_new (ModestConf *modest_conf)
{
	GObject *obj;
	ModestUIPrivate *priv;

	g_return_val_if_fail (modest_conf, NULL);

	obj  = g_object_new(MODEST_TYPE_UI, NULL);
	priv = MODEST_UI_GET_PRIVATE(obj);

	g_object_ref (G_OBJECT(modest_conf));
	priv->conf = modest_conf;
	
	priv->account_mgr = MODEST_ACCOUNT_MGR(modest_account_mgr_new (priv->conf));
	if (!priv->account_mgr) {
		g_printerr ("modest: could not create ModestAccountMgr instance\n");
		g_object_unref (obj);
		return NULL;
        }

	priv->account_store = modest_tny_account_store_new (priv->account_mgr);
	if (!priv->account_store) {
		g_printerr ("modest: could not initialze ModestTnyAccountStore instance\n");
		return NULL;
        }

	priv->widget_factory = modest_widget_factory_new (
		priv->conf, priv->account_store, priv->account_mgr);
	if (!priv->account_store) {
		g_printerr ("modest: could not initialize widget factory\n");
		return NULL;
	}
		
	return MODEST_UI(obj);
}


GtkWidget*
modest_ui_main_window (ModestUI *modest_ui)
{
	GtkWidget* win;
	ModestUIPrivate *priv;

	g_return_val_if_fail (modest_ui, NULL);
	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	win = modest_main_window_new (priv->widget_factory, priv->conf);
	if (!win)
		g_printerr ("modest: could not create main window\n");

	return win;
}
