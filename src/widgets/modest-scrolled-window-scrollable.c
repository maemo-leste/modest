/* Copyright (c) 2009, Igalia
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

#include <glib/gi18n.h>
#include "modest-scrolled-window-scrollable.h"

static void modest_scrolled_window_scrollable_class_init (ModestScrolledWindowScrollableClass *klass);
static void modest_scrolled_window_scrollable_init (ModestScrolledWindowScrollable *self);
static void modest_scrollable_iface_init (gpointer g, gpointer iface_data);

/* GObject interface */
static void modest_scrolled_window_scrollable_finalize (GObject *obj);
static void modest_scrolled_window_scrollable_get_property (GObject *obj,
								 guint prop_id, 
								 GValue *value, 
								 GParamSpec *pspec);
static void modest_scrolled_window_scrollable_set_property (GObject *obj,
								 guint prop_id, 
								 const GValue *value, 
								 GParamSpec *pspec);

/* Modest scrollable interface */
static void modest_scrolled_window_scrollable_add_with_viewport (ModestScrollable *self, GtkWidget *widget);
static void modest_scrolled_window_scrollable_add_with_viewport_default (ModestScrollable *self, GtkWidget *widget);
static GtkAdjustment *modest_scrolled_window_scrollable_get_vadjustment (ModestScrollable *self);
static GtkAdjustment *modest_scrolled_window_scrollable_get_vadjustment_default (ModestScrollable *self);
static GtkAdjustment *modest_scrolled_window_scrollable_get_hadjustment (ModestScrollable *self);
static GtkAdjustment *modest_scrolled_window_scrollable_get_hadjustment_default (ModestScrollable *self);
static void modest_scrolled_window_scrollable_scroll_to (ModestScrollable *self, const gint x, const gint y);
static void modest_scrolled_window_scrollable_scroll_to_default (ModestScrollable *self, const gint x, const gint y);
static void modest_scrolled_window_scrollable_jump_to (ModestScrollable *self, const gint x, const gint y);
static void modest_scrolled_window_scrollable_jump_to_default (ModestScrollable *self, const gint x, const gint y);
static GtkPolicyType modest_scrolled_window_scrollable_get_vertical_policy (ModestScrollable *self);
static GtkPolicyType modest_scrolled_window_scrollable_get_vertical_policy_default (ModestScrollable *self);
static GtkPolicyType modest_scrolled_window_scrollable_get_horizontal_policy (ModestScrollable *self);
static GtkPolicyType modest_scrolled_window_scrollable_get_horizontal_policy_default (ModestScrollable *self);
static void modest_scrolled_window_scrollable_set_horizontal_policy (ModestScrollable *self, GtkPolicyType policy);
static void modest_scrolled_window_scrollable_set_horizontal_policy_default (ModestScrollable *self, GtkPolicyType policy);
static void modest_scrolled_window_scrollable_set_vertical_policy (ModestScrollable *self, GtkPolicyType policy);
static void modest_scrolled_window_scrollable_set_vertical_policy_default (ModestScrollable *self, GtkPolicyType policy);

/* list properties */
enum {
	PROP_0,
	PROP_HORIZONTAL_POLICY,
	PROP_VERTICAL_POLICY,
	PROP_MOVEMENT_MODE,
	PROP_HORIZONTAL_MAX_OVERSHOOT,
	PROP_VERTICAL_MAX_OVERSHOOT,
};

/* globals */
static GtkScrolledWindowClass *parent_class = NULL;

G_DEFINE_TYPE_EXTENDED    (ModestScrolledWindowScrollable,
			   modest_scrolled_window_scrollable,
			   GTK_TYPE_SCROLLED_WINDOW,
			   0,
			   {
			     G_IMPLEMENT_INTERFACE (MODEST_TYPE_SCROLLABLE, modest_scrollable_iface_init);
			   }
			   );

GtkWidget *
modest_scrolled_window_scrollable_new                            (void)
{
	GtkWidget *scrollable = g_object_new (MODEST_TYPE_SCROLLED_WINDOW_SCROLLABLE,
					      "hscrollbar-policy", GTK_POLICY_NEVER,
					      "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
					      NULL);

    return scrollable;
}

static void
modest_scrolled_window_scrollable_class_init (ModestScrolledWindowScrollableClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class = (GObjectClass *) klass;
	gobject_class->finalize = modest_scrolled_window_scrollable_finalize;
	gobject_class->set_property = modest_scrolled_window_scrollable_set_property;
	gobject_class->get_property = modest_scrolled_window_scrollable_get_property;

	klass->add_with_viewport_func = modest_scrolled_window_scrollable_add_with_viewport_default;
	klass->get_vadjustment_func = modest_scrolled_window_scrollable_get_vadjustment_default;
	klass->get_hadjustment_func = modest_scrolled_window_scrollable_get_hadjustment_default;
	klass->scroll_to_func = modest_scrolled_window_scrollable_scroll_to_default;
	klass->jump_to_func = modest_scrolled_window_scrollable_jump_to_default;
	klass->get_vertical_policy_func = modest_scrolled_window_scrollable_get_vertical_policy_default;
	klass->get_horizontal_policy_func = modest_scrolled_window_scrollable_get_horizontal_policy_default;
	klass->set_vertical_policy_func = modest_scrolled_window_scrollable_set_vertical_policy_default;
	klass->set_horizontal_policy_func = modest_scrolled_window_scrollable_set_horizontal_policy_default;

	g_object_class_install_property (gobject_class,
					 PROP_HORIZONTAL_POLICY,
					 g_param_spec_enum ("horizontal_policy", 
							    _("Horizontal scrollbar policy"),
							    _("Visual policy of the horizontal scrollbar"),
							    GTK_TYPE_POLICY_TYPE,
							    GTK_POLICY_AUTOMATIC,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (gobject_class,
					 PROP_VERTICAL_POLICY,
					 g_param_spec_enum ("vertical_policy", 
							    _("Vertical scrollbar policy"),
							    _("Visual policy of the vertical scrollbar"),
							    GTK_TYPE_POLICY_TYPE,
							    GTK_POLICY_AUTOMATIC,
							    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (gobject_class,
					 PROP_MOVEMENT_MODE,
					 g_param_spec_flags ("movement_mode",
							     "Directions scroll is allowed",
							     "Movements allowed in the scrollable",
							     MODEST_TYPE_MOVEMENT_MODE,
							     MODEST_MOVEMENT_MODE_VERTICAL,
							     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT));

	g_object_class_install_property (gobject_class,
					 PROP_HORIZONTAL_MAX_OVERSHOOT,
					 g_param_spec_int ("horizontal-max-overshoot",
							   "Horizontal max overshoot",
							   "Horizontal maximum overshoot (0 disables)",
							   0, G_MAXINT, 150,
							   G_PARAM_READWRITE |G_PARAM_CONSTRUCT));

	g_object_class_install_property (gobject_class,
					 PROP_VERTICAL_MAX_OVERSHOOT,
					 g_param_spec_int ("vertical-max-overshoot",
							   "Vertical max overshoot",
							   "Vertical maximum overshoot (0 disables)",
							   0, G_MAXINT, 150,
							   G_PARAM_READWRITE |G_PARAM_CONSTRUCT));
}

static void
modest_scrolled_window_scrollable_init (ModestScrolledWindowScrollable *self)
{
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (self), GTK_SHADOW_NONE);
}

static void
modest_scrollable_iface_init (gpointer g, gpointer iface_data)
{
	ModestScrollableIface *iface = (ModestScrollableIface *) g;

	iface->jump_to = modest_scrolled_window_scrollable_jump_to;
	iface->scroll_to = modest_scrolled_window_scrollable_scroll_to;
	iface->add_with_viewport = modest_scrolled_window_scrollable_add_with_viewport;
	iface->get_vadjustment = modest_scrolled_window_scrollable_get_vadjustment;
	iface->get_hadjustment = modest_scrolled_window_scrollable_get_hadjustment;
	iface->get_vertical_policy = modest_scrolled_window_scrollable_get_vertical_policy;
	iface->set_vertical_policy = modest_scrolled_window_scrollable_set_vertical_policy;
	iface->get_horizontal_policy = modest_scrolled_window_scrollable_get_horizontal_policy;
	iface->set_horizontal_policy = modest_scrolled_window_scrollable_set_horizontal_policy;
}

static void 
modest_scrolled_window_scrollable_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}

static void
modest_scrolled_window_scrollable_get_property (GObject *obj,
						guint prop_id, 
						GValue *value, 
						GParamSpec *pspec)
{
	GtkPolicyType policy;

	switch (prop_id) {
	case PROP_VERTICAL_POLICY:
		gtk_scrolled_window_get_policy (GTK_SCROLLED_WINDOW (obj), NULL, &policy);
		g_value_set_enum (value, policy);
		break;
	case PROP_HORIZONTAL_POLICY:
		gtk_scrolled_window_get_policy (GTK_SCROLLED_WINDOW (obj), &policy, NULL);
		g_value_set_enum (value, policy);
		break;
	case PROP_MOVEMENT_MODE:
		g_value_set_enum (value, MODEST_MOVEMENT_MODE_BOTH);
		break;
	case PROP_HORIZONTAL_MAX_OVERSHOOT:
		g_value_set_int (value, 0);
		break;
	case PROP_VERTICAL_MAX_OVERSHOOT:
		g_value_set_int (value, 0);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
modest_scrolled_window_scrollable_set_property (GObject *obj,
						     guint prop_id, 
						     const GValue *value, 
						     GParamSpec *pspec)
{
	GtkPolicyType hpolicy, vpolicy;

	gtk_scrolled_window_get_policy (GTK_SCROLLED_WINDOW (obj), &hpolicy, &vpolicy);
	switch (prop_id) {
	case PROP_VERTICAL_POLICY:
		vpolicy = g_value_get_enum (value);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (obj), hpolicy, vpolicy);
		break;
	case PROP_HORIZONTAL_POLICY:
		hpolicy = g_value_get_enum (value);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (obj), hpolicy, vpolicy);
		break;
	case PROP_MOVEMENT_MODE:
	case PROP_HORIZONTAL_MAX_OVERSHOOT:
	case PROP_VERTICAL_MAX_OVERSHOOT:
		/* these three parameters are not supported in scrolled window */
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
modest_scrolled_window_scrollable_add_with_viewport (ModestScrollable *self,
							  GtkWidget *widget)
{
	MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->add_with_viewport_func (self, widget);
}

static void
modest_scrolled_window_scrollable_add_with_viewport_default (ModestScrollable *self,
								  GtkWidget *widget)
{
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (self), widget);
}

static GtkAdjustment *
modest_scrolled_window_scrollable_get_vadjustment (ModestScrollable *self)
{
	return MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->get_vadjustment_func (self);
}

static GtkAdjustment *
modest_scrolled_window_scrollable_get_vadjustment_default (ModestScrollable *self)
{
	return gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self));
}

static GtkAdjustment *
modest_scrolled_window_scrollable_get_hadjustment (ModestScrollable *self)
{
	return MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->get_hadjustment_func (self);
}

static GtkAdjustment *
modest_scrolled_window_scrollable_get_hadjustment_default (ModestScrollable *self)
{
	return gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (self));
}

static void
modest_scrolled_window_scrollable_scroll_to (ModestScrollable *self, 
					     const gint x, const gint y)
{
	MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->scroll_to_func (self, x, y);
}

static void
modest_scrolled_window_scrollable_scroll_to_default (ModestScrollable *self,
						     const gint x, const gint y)
{
	GtkAdjustment *vadj, *hadj;
	hadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self));
	gtk_adjustment_set_value (hadj, (gdouble) y);
	vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self));
}

static void
modest_scrolled_window_scrollable_jump_to (ModestScrollable *self, 
						const gint x, const gint y)
{
	MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->jump_to_func (self, x, y);
}

static void
modest_scrolled_window_scrollable_jump_to_default (ModestScrollable *self,
							const gint x, const gint y)
{
	modest_scrollable_scroll_to (MODEST_SCROLLABLE (self), x, y);
}

static GtkPolicyType
modest_scrolled_window_scrollable_get_vertical_policy (ModestScrollable *self)
{
	return MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->get_vertical_policy_func (self);
}

static GtkPolicyType
modest_scrolled_window_scrollable_get_vertical_policy_default (ModestScrollable *self)
{
	GtkPolicyType policy;

	g_object_get (G_OBJECT (self), "vertical-policy", &policy, NULL);
	return policy;
}

static GtkPolicyType
modest_scrolled_window_scrollable_get_horizontal_policy (ModestScrollable *self)
{
	return MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->get_horizontal_policy_func (self);
}

static GtkPolicyType
modest_scrolled_window_scrollable_get_horizontal_policy_default (ModestScrollable *self)
{
	GtkPolicyType policy;

	g_object_get (G_OBJECT (self), "horizontal-policy", &policy, NULL);
	return policy;
}

static void
modest_scrolled_window_scrollable_set_horizontal_policy (ModestScrollable *self,
							      GtkPolicyType policy)
{
	MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->set_horizontal_policy_func (self, policy);
}

static void
modest_scrolled_window_scrollable_set_horizontal_policy_default (ModestScrollable *self,
								      GtkPolicyType policy)
{
	g_object_set (G_OBJECT (self), "horizontal-policy", policy, NULL);
}

static void
modest_scrolled_window_scrollable_set_vertical_policy (ModestScrollable *self,
							    GtkPolicyType policy)
{
	MODEST_SCROLLED_WINDOW_SCROLLABLE_GET_CLASS (self)->set_vertical_policy_func (self, policy);
}

static void
modest_scrolled_window_scrollable_set_vertical_policy_default (ModestScrollable *self,
								    GtkPolicyType policy)
{
	g_object_set (G_OBJECT (self), "vertical-policy", policy, NULL);
}
