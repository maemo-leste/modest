/* Copyright (c) 2007, Nokia Corporation
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

#include <modest-zoomable.h>

/**
 * modest_zoomable_set_zoom:
 * @zoomable: a #ModestZoomable instance
 * @value: a #gdouble (1.0 is no zoom).
 *
 * sets the current zoom leven of @zoomable to @value
 */
void            
modest_zoomable_set_zoom (ModestZoomable *zoomable,
			  gdouble value)
{
	MODEST_ZOOMABLE_GET_IFACE (zoomable)->set_zoom_func (zoomable, value);
}

/**
 * modest_zoomable_get_zoom:
 * @zoomable: a #ModestZoomable instance
 *
 * obtains the current zoom level of @zoomable.
 *
 * Returns: a #gdouble (1.0 is no zoom).
 */
gdouble         
modest_zoomable_get_zoom (ModestZoomable *zoomable)
{
	return MODEST_ZOOMABLE_GET_IFACE (zoomable)->get_zoom_func (zoomable);
}

/**
 * modest_zoomable_zoom_plus:
 * @zoomable: a #ModestZoomable instance
 *
 * increases the zoom level, if it can. If not
 * it returns %FALSE
 *
 * Returns: %TRUE if zoom was increased, %FALSE otherwise
 */
gboolean
modest_zoomable_zoom_plus   (ModestZoomable *zoomable)
{
	return MODEST_ZOOMABLE_GET_IFACE (zoomable)->zoom_plus_func (zoomable);
}

/**
 * modest_zoomable_zoom_minus:
 * @zoomable: a #ModestZoomable instance
 *
 * decreases the zoom level, if it can. If not
 * it returns %FALSE
 *
 * Returns: %TRUE if zoom was decreased, %FALSE otherwise
 */
gboolean        
modest_zoomable_zoom_minus  (ModestZoomable *zoomable)
{
	return MODEST_ZOOMABLE_GET_IFACE (zoomable)->zoom_minus_func (zoomable);
}

static void
modest_zoomable_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

GType
modest_zoomable_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0)) 
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestZoomableIface),
		  modest_zoomable_base_init,   /* base_init */
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
			"ModestZoomable", &info, 0);

	}

	return type;
}
