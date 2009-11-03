/* Copyright (c) 2009, Igalia Corporation
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

#include <modest-isearch-toolbar.h>


/******************* METHODS */
void
modest_isearch_toolbar_highlight_entry (ModestISearchToolbar *self, gboolean get_focus)
{
	MODEST_ISEARCH_TOOLBAR_GET_IFACE (self)->highlight_entry (self, get_focus);
}

void
modest_isearch_toolbar_set_label (ModestISearchToolbar *self, const gchar *label)
{
	MODEST_ISEARCH_TOOLBAR_GET_IFACE (self)->set_label (self, label);
}

const gchar *
modest_isearch_toolbar_get_search (ModestISearchToolbar *self)
{
	return MODEST_ISEARCH_TOOLBAR_GET_IFACE (self)->get_search (self);
}

static void
modest_isearch_toolbar_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {

		/* init signals here */
		g_signal_new ("isearch-close",
			      MODEST_TYPE_ISEARCH_TOOLBAR,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ModestISearchToolbarIface, isearch_close),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
		g_signal_new ("isearch-search",
			      MODEST_TYPE_ISEARCH_TOOLBAR,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ModestISearchToolbarIface, isearch_search),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
		initialized = TRUE;
	}
}

GType
modest_isearch_toolbar_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0)) 
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestISearchToolbarIface),
		  modest_isearch_toolbar_base_init,   /* base_init */
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
			"ModestISearchToolbar", &info, 0);

	}

	return type;
}
