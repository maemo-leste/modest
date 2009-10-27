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



static void
modest_scrollable_base_init (gpointer g_iface)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;

		g_object_interface_install_property (g_iface,
						     g_param_spec_enum ("vscrollbar_policy",
									"vscrollbar policy",
									"Visual policy of the vertical scrollbar",
									GTK_TYPE_POLICY_TYPE,
									GTK_POLICY_AUTOMATIC,
									G_PARAM_READWRITE |
									G_PARAM_CONSTRUCT));

		g_object_interface_install_property (g_iface,
						     g_param_spec_enum ("hscrollbar_policy",
									"hscrollbar policy",
									"Visual policy of the horizontal scrollbar",
									GTK_TYPE_POLICY_TYPE,
									GTK_POLICY_AUTOMATIC,
									G_PARAM_READWRITE |
									G_PARAM_CONSTRUCT));


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
