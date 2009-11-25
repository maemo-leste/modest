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

#include <config.h>

#include <modest-scrollable.h>

/**
 * modest_scrollable_add_with_viewport:
 * @scrollable: a #ModestScrollable instance
 * @widget: a #GtkWidget
 *
 * adds a widget to the @scrollable, wrapping it into a viewport.
 * This is useful if @widget does not implement scroll_adjustments
 * interface.
 */
void            
modest_scrollable_add_with_viewport (ModestScrollable *scrollable,
				     GtkWidget *widget)
{
	if (MODEST_SCROLLABLE_GET_IFACE (scrollable)->add_with_viewport) {
		MODEST_SCROLLABLE_GET_IFACE (scrollable)->add_with_viewport (scrollable, widget);
	}
}

/**
 * modest_scrollable_get_vadjustment:
 * @self: a #ModestScrollable
 *
 * obtains the vertical adjustment for @self
 *
 * Returns: a #GtkAdjustment
 */
GtkAdjustment *
modest_scrollable_get_vadjustment (ModestScrollable *self)
{
	if (MODEST_SCROLLABLE_GET_IFACE (self)->get_vadjustment) {
		return MODEST_SCROLLABLE_GET_IFACE (self)->get_vadjustment (self);
	} else {
		return NULL;
	}
}

/**
 * modest_scrollable_get_hadjustment:
 * @self: a #ModestScrollable
 *
 * obtains the horizontal adjustment for @self
 *
 * Returns: a #GtkAdjustment
 */
GtkAdjustment *
modest_scrollable_get_hadjustment (ModestScrollable *self)
{
	if (MODEST_SCROLLABLE_GET_IFACE (self)->get_hadjustment) {
		return MODEST_SCROLLABLE_GET_IFACE (self)->get_hadjustment (self);
	} else {
		return NULL;
	}
}

/**
 * modest_scrollable_scroll_to:
 * @scrollable: a #ModestScrollable instance
 * @x: the X coordinate to scroll to
 * @y: the Y coordinate to scroll to
 *
 * scrolls softly visible area of @scrollable, to show the coordinates
 * (@x, @y).
 */
void            
modest_scrollable_scroll_to (ModestScrollable *scrollable,
			     const gint x, const gint y)
{
	if (MODEST_SCROLLABLE_GET_IFACE (scrollable)->scroll_to) {
		MODEST_SCROLLABLE_GET_IFACE (scrollable)->scroll_to (scrollable, x, y);
	}
}

/**
 * modest_scrollable_jump_to:
 * @scrollable: a #ModestScrollable instance
 * @x: the X coordinate to jump to
 * @y: the Y coordinate to jump to
 *
 * scrolls visible area of @scrollable, to show the coordinates
 * (@x, @y). Movement is immediate.
 */
void            
modest_scrollable_jump_to (ModestScrollable *scrollable,
			     const gint x, const gint y)
{
	if (MODEST_SCROLLABLE_GET_IFACE (scrollable)->jump_to) {
		MODEST_SCROLLABLE_GET_IFACE (scrollable)->jump_to (scrollable, x, y);
	}
}

/**
 * modest_scrollable_get_vertical_policy:
 * @scrollable: a #ModestScrollable instance
 *
 * returns the vertical scroll policy
 *
 * Returns: a #GtkPolicyType
 */
GtkPolicyType            
modest_scrollable_get_vertical_policy (ModestScrollable *scrollable)
{
	if (MODEST_SCROLLABLE_GET_IFACE (scrollable)->get_vertical_policy) {
		return MODEST_SCROLLABLE_GET_IFACE (scrollable)->get_vertical_policy (scrollable);
	} else {
		return GTK_POLICY_NEVER;
	}
}

/**
 * modest_scrollable_get_horizontal_policy:
 * @scrollable: a #ModestScrollable instance
 *
 * returns the horizontal scroll policy
 *
 * Returns: a #GtkPolicyType
 */
GtkPolicyType            
modest_scrollable_get_horizontal_policy (ModestScrollable *scrollable)
{
	if (MODEST_SCROLLABLE_GET_IFACE (scrollable)->get_horizontal_policy) {
		return MODEST_SCROLLABLE_GET_IFACE (scrollable)->get_horizontal_policy (scrollable);
	} else {
		return GTK_POLICY_NEVER;
	}
}

/**
 * modest_scrollable_set_vertical_policy:
 * @scrollable: a #ModestScrollable instance
 * @policy: a #GtkPolicyType
 *
 * sets the vertical scroll policy
 */
void
modest_scrollable_set_vertical_policy (ModestScrollable *scrollable,
				       GtkPolicyType policy)
{
	if (MODEST_SCROLLABLE_GET_IFACE (scrollable)->set_vertical_policy) {
		MODEST_SCROLLABLE_GET_IFACE (scrollable)->set_vertical_policy (scrollable, policy);
	}
}

/**
 * modest_scrollable_set_horizontal_policy:
 * @scrollable: a #ModestScrollable instance
 * @policy: a #GtkPolicyType
 *
 * sets the horizontal scroll policy
 */
void
modest_scrollable_set_horizontal_policy (ModestScrollable *scrollable,
					 GtkPolicyType policy)
{
	if (MODEST_SCROLLABLE_GET_IFACE (scrollable)->set_horizontal_policy) {
		MODEST_SCROLLABLE_GET_IFACE (scrollable)->set_horizontal_policy (scrollable, policy);
	}
}

static void
modest_scrollable_base_init (gpointer g_iface)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;

		g_object_interface_install_property (g_iface,
						     g_param_spec_enum ("vertical_policy",
									"Vertical scroll policy",
									"Visual policy of the vertical scroll",
									GTK_TYPE_POLICY_TYPE,
									GTK_POLICY_AUTOMATIC,
									G_PARAM_READWRITE |
									G_PARAM_CONSTRUCT));

		g_object_interface_install_property (g_iface,
						     g_param_spec_enum ("horizontal_policy",
									"Horizontal scroll policy",
									"Visual policy of the horizontal scroll",
									GTK_TYPE_POLICY_TYPE,
									GTK_POLICY_AUTOMATIC,
									G_PARAM_READWRITE |
									G_PARAM_CONSTRUCT));

		g_object_interface_install_property (g_iface,
						     g_param_spec_flags ("movement_mode",
									 "Directions scroll is allowed",
									 "Movements allowed in the scrollable",
									 MODEST_TYPE_MOVEMENT_MODE,
									 MODEST_MOVEMENT_MODE_VERTICAL,
									 G_PARAM_READWRITE |
									 G_PARAM_CONSTRUCT));

		g_object_interface_install_property (g_iface,
						     g_param_spec_int ("horizontal-max-overshoot",
								       "Horizontal max overshoot",
								       "Horizontal maximum overshoot (0 disables)",
								       0, G_MAXINT, 150,
								       G_PARAM_READWRITE |G_PARAM_CONSTRUCT));

		g_object_interface_install_property (g_iface,
						     g_param_spec_int ("vertical-max-overshoot",
								       "Vertical max overshoot",
								       "Vertical maximum overshoot (0 disables)",
								       0, G_MAXINT, 150,
								       G_PARAM_READWRITE |G_PARAM_CONSTRUCT));


	}
}

GType
modest_scrollable_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0)) 
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestScrollableIface),
		  modest_scrollable_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL,   /* instance_init */
		  NULL
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
			"ModestScrollable", &info, 0);

	}

	return type;
}

void
modest_scrollable_scroll (ModestScrollable *scrollable, 
			  gint horizontal, gint vertical)
{
	g_return_if_fail (MODEST_IS_SCROLLABLE (scrollable));
	gint h_pos = -1;
	gint v_pos = -1;

	g_assert (scrollable);
	/* at atleast one of values have to be valid */
	g_return_if_fail (h_pos == -1 && v_pos == -1);

	if (horizontal != 0) {
		GtkAdjustment *h_adj;

		h_adj = modest_scrollable_get_hadjustment (scrollable);
		g_return_if_fail (h_adj);

		h_pos = h_adj->value + h_adj->step_increment * horizontal;
		if (horizontal > 0) {
			h_pos += h_adj->page_size;
		}
	}

	if (vertical != 0) {
		GtkAdjustment *v_adj;

		v_adj = modest_scrollable_get_vadjustment (scrollable);
		g_return_if_fail (v_adj);

		v_pos = v_adj->value + v_adj->step_increment * vertical;
		if (vertical > 0) {
			v_pos += v_adj->page_size;
		}
	}

	modest_scrollable_scroll_to (scrollable, h_pos, v_pos);
}

GType
modest_movement_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
	    { MODEST_MOVEMENT_MODE_HORIZONTAL, "HILDON_MOVEMENT_MODE_HORIZONTAL", "horizontal" },
	    { MODEST_MOVEMENT_MODE_VERTICAL, "MODEST_MOVEMENT_MODE_VERTICAL", "vertical" },
	    { MODEST_MOVEMENT_MODE_BOTH, "MODEST_MOVEMENT_MODE_BOTH", "both" },
	    { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("ModestMovementMode", values);
  }
  return etype;
}
