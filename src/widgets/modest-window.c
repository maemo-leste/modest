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

#include "modest-window.h"
#include "modest-window-priv.h"
#include "modest-ui-actions.h"
#include "modest-tny-platform-factory.h"
#include "modest-runtime.h"
#include "modest-window-mgr.h"
#include "modest-defs.h"
#include <string.h> /* for strcmp */
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xdmcp.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-defines.h>
#endif

/* 'private'/'protected' functions */
static void modest_window_class_init (ModestWindowClass *klass);
static void modest_window_init       (ModestWindow *obj);
static void modest_window_finalize   (GObject *obj);
static void modest_window_dispose    (GObject *obj);

static gdouble  modest_window_get_zoom_default           (ModestWindow *window);

static gboolean modest_window_zoom_plus_default          (ModestWindow *window);

static gboolean modest_window_zoom_minus_default         (ModestWindow *window);

static void     modest_window_disconnect_signals_default (ModestWindow *self);

static void     modest_window_show_progress_default      (ModestWindow *self,
							  gboolean show);

static void     modest_window_show_toolbar_default       (ModestWindow *window,
							  gboolean show_toolbar);

static void     modest_window_add_toolbar_default       (ModestWindow *window,
							 GtkToolbar *toolbar);

static void     modest_window_add_to_menu_default       (ModestWindow *self,
							 const gchar *label,
							 const gchar *accelerator,
							 ModestWindowMenuCallback callback,
							 ModestDimmingCallback dimming_callback);

static void     modest_window_set_title_default       (ModestWindow *window,
						       const gchar *title);

static void     modest_window_set_zoom_default           (ModestWindow *window,
							  gdouble zoom);

static gboolean on_key_pressed (GtkWidget *self, GdkEventKey *event, gpointer user_data);
static void _make_zoom_buttons_grabeable (GtkWidget* widget);
static gboolean _modest_window_map_event (GtkWidget *widget,
					  GdkEvent *event,
					  gpointer userdata);

/* list my signals  */
enum {
	LAST_SIGNAL
};

/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_window_get_type (void)
{
	static GType my_type = 0;
	static GType parent_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_window_init,
			NULL
		};
#ifndef MODEST_TOOLKIT_GTK
#ifdef MODEST_TOOLKIT_HILDON2
		parent_type = HILDON_TYPE_STACKABLE_WINDOW;
#else
		parent_type = HILDON_TYPE_WINDOW;
#endif
#else
		parent_type = GTK_TYPE_WINDOW;
#endif 
		my_type = g_type_register_static (parent_type,
		                                  "ModestWindow",
		                                  &my_info, 
						  G_TYPE_FLAG_ABSTRACT);
	}
	return my_type;
}

static void
modest_window_class_init (ModestWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->dispose  = modest_window_dispose;
	gobject_class->finalize = modest_window_finalize;

	klass->set_zoom_func = modest_window_set_zoom_default;
	klass->get_zoom_func = modest_window_get_zoom_default;
	klass->zoom_plus_func = modest_window_zoom_plus_default;
	klass->zoom_minus_func = modest_window_zoom_minus_default;
	klass->show_toolbar_func = modest_window_show_toolbar_default;
	klass->add_toolbar_func = modest_window_add_toolbar_default;
	klass->add_to_menu_func = modest_window_add_to_menu_default;
	klass->set_title_func = modest_window_set_title_default;
	klass->disconnect_signals_func = modest_window_disconnect_signals_default;
	klass->show_progress_func = modest_window_show_progress_default;

	g_type_class_add_private (gobject_class, sizeof(ModestWindowPrivate));
}

static void
modest_window_init (ModestWindow *obj)
{
	ModestWindowPrivate *priv;

	priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->ui_manager     = NULL;
	priv->ui_dimming_manager     = NULL;
	priv->toolbar        = NULL;
	priv->menubar        = NULL;

	priv->dimming_state = NULL;
	priv->ui_dimming_enabled = TRUE;
	priv->active_account = NULL;
	priv->active_mailbox = NULL;

	/* Connect signals */
	g_signal_connect (G_OBJECT (obj), 
			  "key-press-event", 
			  G_CALLBACK (on_key_pressed), NULL);
	g_signal_connect (G_OBJECT (obj), "map-event",
			  G_CALLBACK (_modest_window_map_event),
			  G_OBJECT (obj));
}

static void
modest_window_finalize (GObject *obj)
{
	ModestWindowPrivate *priv;	

	priv = MODEST_WINDOW_GET_PRIVATE(obj);

	if (priv->dimming_state != NULL) {
		g_slice_free (DimmedState, priv->dimming_state);
		priv->dimming_state = NULL;
	}

	g_free (priv->active_account);
	g_free (priv->active_mailbox);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_window_dispose (GObject *obj)
{
	ModestWindowPrivate *priv;

	priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->ui_dimming_enabled = FALSE;

	if (priv->ui_manager) {
		g_object_unref (G_OBJECT(priv->ui_manager));
		priv->ui_manager = NULL;
	}
	if (priv->ui_dimming_manager) {
		g_object_run_dispose (G_OBJECT (priv->ui_dimming_manager));
		g_object_unref (G_OBJECT(priv->ui_dimming_manager));
		priv->ui_dimming_manager = NULL;
	}

	G_OBJECT_CLASS(parent_class)->dispose (obj);
}



const gchar*
modest_window_get_active_account (ModestWindow *self)
{
	g_return_val_if_fail (self, NULL);
	//g_warning ("%s: %s", __FUNCTION__, MODEST_WINDOW_GET_PRIVATE(self)->active_account);
	return MODEST_WINDOW_GET_PRIVATE(self)->active_account;
}

void
modest_window_set_active_account (ModestWindow *self, const gchar *active_account)
{
	ModestWindowPrivate *priv;

	g_return_if_fail (self);	
	priv = MODEST_WINDOW_GET_PRIVATE(self);

	//g_warning ("%s: %s", __FUNCTION__, active_account);
	
	/* only 'real' account should be set here; for example the email signature
	 * depends on the current account, so if you reply to a message in your
	 * archive, it should take the signature from the real active account,
	 * not the non-existing one from your mmc-pseudo-account
	 */
	if (active_account && ((strcmp (active_account, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) == 0) ||
			       (strcmp (active_account, MODEST_MMC_ACCOUNT_ID) == 0))) {
			g_warning ("%s: %s is not a valid active account",
				   __FUNCTION__, active_account);
			return;
	}
	
	if (active_account == priv->active_account)
		return;
	else {
		g_free (priv->active_account);
		priv->active_account = NULL;
		if (active_account)
			priv->active_account = g_strdup (active_account);
	}
}

const gchar*
modest_window_get_active_mailbox (ModestWindow *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WINDOW_GET_PRIVATE(self)->active_mailbox;
}

void
modest_window_set_active_mailbox (ModestWindow *self, const gchar *active_mailbox)
{
	ModestWindowPrivate *priv;

	g_return_if_fail (self);	
	priv = MODEST_WINDOW_GET_PRIVATE(self);

	if (active_mailbox == priv->active_mailbox)
		return;
	else {
		g_free (priv->active_mailbox);
		priv->active_mailbox = NULL;
		if (active_mailbox)
			priv->active_mailbox = g_strdup (active_mailbox);
	}
}

void
modest_window_check_dimming_rules (ModestWindow *self)
{
	ModestWindowPrivate *priv;	

	g_return_if_fail (MODEST_IS_WINDOW (self));
	priv = MODEST_WINDOW_GET_PRIVATE(self);

	if (priv->ui_dimming_enabled)
		modest_ui_dimming_manager_process_dimming_rules (priv->ui_dimming_manager);
}

void
modest_window_check_dimming_rules_group (ModestWindow *self,
					 const gchar *group_name)
{
	ModestWindowPrivate *priv;	

	g_return_if_fail (MODEST_IS_WINDOW (self));
	priv = MODEST_WINDOW_GET_PRIVATE(self);

	if (priv->ui_dimming_enabled)
		modest_ui_dimming_manager_process_dimming_rules_group (priv->ui_dimming_manager, group_name);
}

void
modest_window_set_dimming_state (ModestWindow *window,
				 DimmedState *state)
{
	ModestWindowPrivate *priv;	

	g_return_if_fail (MODEST_IS_WINDOW (window));
	priv = MODEST_WINDOW_GET_PRIVATE(window);

	/* Free previous */
	if (priv->dimming_state != NULL)
		g_slice_free (DimmedState, priv->dimming_state);

	/* Set new state */
	priv->dimming_state = state;
}

const DimmedState *
modest_window_get_dimming_state (ModestWindow *window)
{
	ModestWindowPrivate *priv;	

	g_return_val_if_fail (MODEST_IS_WINDOW (window), NULL);
	priv = MODEST_WINDOW_GET_PRIVATE(window);

	return priv->dimming_state;
}

void
modest_window_disable_dimming (ModestWindow *self)
{
	ModestWindowPrivate *priv;	

	g_return_if_fail (MODEST_IS_WINDOW (self));
	priv = MODEST_WINDOW_GET_PRIVATE(self);

	priv->ui_dimming_enabled = FALSE;
}

void
modest_window_enable_dimming (ModestWindow *self)
{
	ModestWindowPrivate *priv;	

	g_return_if_fail (MODEST_IS_WINDOW (self));
	priv = MODEST_WINDOW_GET_PRIVATE(self);

	priv->ui_dimming_enabled = TRUE;
}

GtkAction *
modest_window_get_action (ModestWindow *window, 
			  const gchar *action_path) 
{
	GtkAction *action = NULL;
	ModestWindowPrivate *priv;	

	g_return_val_if_fail (MODEST_IS_WINDOW (window), NULL);
	priv = MODEST_WINDOW_GET_PRIVATE(window);

        action = gtk_ui_manager_get_action (priv->ui_manager, action_path);	

	return action;
}

GtkWidget *
modest_window_get_action_widget (ModestWindow *window, 
				 const gchar *action_path) 
{
	GtkWidget *widget = NULL;
	ModestWindowPrivate *priv;	

	g_return_val_if_fail (MODEST_IS_WINDOW (window), NULL);
	priv = MODEST_WINDOW_GET_PRIVATE(window);

        widget = gtk_ui_manager_get_widget (priv->ui_manager, action_path);	

	return widget;
}

void
modest_window_set_zoom (ModestWindow *window,
			gdouble zoom)
{
	MODEST_WINDOW_GET_CLASS (window)->set_zoom_func (window, zoom);
	return;
}

gdouble
modest_window_get_zoom (ModestWindow *window)
{
	return MODEST_WINDOW_GET_CLASS (window)->get_zoom_func (window);
}

gboolean
modest_window_zoom_plus (ModestWindow *window)
{
	return MODEST_WINDOW_GET_CLASS (window)->zoom_plus_func (window);
}

gboolean
modest_window_zoom_minus (ModestWindow *window)
{
	return MODEST_WINDOW_GET_CLASS (window)->zoom_minus_func (window);
}

void 
modest_window_show_toolbar (ModestWindow *window,
			    gboolean show_toolbar)
{
	MODEST_WINDOW_GET_CLASS (window)->show_toolbar_func (window,
							     show_toolbar);
}

void 
modest_window_add_toolbar (ModestWindow *window,
			   GtkToolbar *toolbar)
{
	MODEST_WINDOW_GET_CLASS (window)->add_toolbar_func (window,
							    toolbar);
}

void 
modest_window_add_to_menu (ModestWindow *window,
			   const gchar *label,
			   const gchar *accelerator,
			   ModestWindowMenuCallback callback,
			   ModestDimmingCallback dimming_callback)
{
	MODEST_WINDOW_GET_CLASS (window)->add_to_menu_func (window,
							    label,
							    accelerator,
							    callback,
							    dimming_callback);
}

void 
modest_window_set_title (ModestWindow *window,
			 const gchar *title)
{
	MODEST_WINDOW_GET_CLASS (window)->set_title_func (window,
							  title);
}

void 
modest_window_disconnect_signals (ModestWindow *window)
{
	MODEST_WINDOW_GET_CLASS (window)->disconnect_signals_func (window);
}

void 
modest_window_show_progress (ModestWindow *window, gboolean show)
{
	MODEST_WINDOW_GET_CLASS (window)->show_progress_func (window, show);
}


/* Default implementations */

static void
modest_window_set_zoom_default (ModestWindow *window,
				gdouble zoom)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);

}

static gdouble
modest_window_get_zoom_default (ModestWindow *window)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
	return 1.0;
}

static gboolean
modest_window_zoom_plus_default (ModestWindow *window)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
	return FALSE;
}

static gboolean
modest_window_zoom_minus_default (ModestWindow *window)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
	return FALSE;
}

static void 
modest_window_show_toolbar_default (ModestWindow *window,
				    gboolean show_toolbar)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
}

static void 
modest_window_add_toolbar_default (ModestWindow *window,
				   GtkToolbar *toolbar)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
}

static void 
modest_window_add_to_menu_default (ModestWindow *window,
				   const gchar *label,
				   const gchar *accelerator,
				   ModestWindowMenuCallback callback,
				   ModestDimmingCallback dimming_callback)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
}

static void 
modest_window_set_title_default (ModestWindow *window,
				 const gchar *title)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
}

static void 
modest_window_disconnect_signals_default (ModestWindow *self)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
}

static void 
modest_window_show_progress_default (ModestWindow *self, gboolean show)
{
	g_warning ("modest: You should implement %s", __FUNCTION__);
}

void
modest_window_save_state (ModestWindow *window)
{
	ModestWindowClass *klass = MODEST_WINDOW_GET_CLASS (window);
	if (klass->save_state_func)
		klass->save_state_func (window);
}

static gboolean
on_key_pressed (GtkWidget *self,
		GdkEventKey *event,
		gpointer user_data)
{
	ModestWindowMgr *mgr = NULL;

	mgr = modest_runtime_get_window_mgr ();

	switch (event->keyval) {
 	case GDK_F6: 
		modest_ui_actions_on_change_fullscreen (NULL, MODEST_WINDOW(self));
		return TRUE;
#ifdef MODEST_TOOLKIT_HILDON2
	case HILDON_HARDKEY_INCREASE: 
		modest_ui_actions_on_zoom_plus (NULL, MODEST_WINDOW(self));
		return TRUE;
	case HILDON_HARDKEY_DECREASE: 
		modest_ui_actions_on_zoom_minus	(NULL, MODEST_WINDOW(self));
		return TRUE;
#else
	case GDK_F7: 
		modest_ui_actions_on_zoom_plus (NULL, MODEST_WINDOW(self));
		return TRUE;
	case GDK_F8: 
		modest_ui_actions_on_zoom_minus	(NULL, MODEST_WINDOW(self));
		return TRUE;
#endif
 	case GDK_Escape: 
		if (modest_window_mgr_get_fullscreen_mode (mgr))
			modest_ui_actions_on_change_fullscreen (NULL, MODEST_WINDOW(self));
		else if (MODEST_IS_MSG_VIEW_WINDOW (self))
			modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (self));
		break;
	}
	
	return FALSE;
}

static gboolean 
_modest_window_map_event (GtkWidget *widget,
			  GdkEvent *event,
			  gpointer userdata)
{
	_make_zoom_buttons_grabeable (GTK_WIDGET (widget));
	return FALSE;
}

static void
_make_zoom_buttons_grabeable (GtkWidget* widget)
{
    GdkDisplay *display;
    Atom atom;
    unsigned long val = 1;

    display = gdk_drawable_get_display (widget->window);
    atom = gdk_x11_get_xatom_by_name_for_display (display, "_HILDON_ZOOM_KEY_ATOM");
    XChangeProperty (GDK_DISPLAY_XDISPLAY (display),
                     GDK_WINDOW_XID (widget->window), atom,
                     XA_INTEGER, 32, PropModeReplace,
                     (unsigned char *) &val, 1);
}
