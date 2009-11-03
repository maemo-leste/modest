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

#ifndef MODEST_ISEARCH_TOOLBAR_H
#define MODEST_ISEARCH_TOOLBAR_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MODEST_TYPE_ISEARCH_TOOLBAR            (modest_isearch_toolbar_get_type ())
#define MODEST_ISEARCH_TOOLBAR(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_ISEARCH_TOOLBAR, ModestISearchToolbar))
#define MODEST_IS_ISEARCH_TOOLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_ISEARCH_TOOLBAR))
#define MODEST_ISEARCH_TOOLBAR_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MODEST_TYPE_ISEARCH_TOOLBAR, ModestISearchToolbarIface))

typedef struct _ModestISearchToolbar ModestISearchToolbar;
typedef struct _ModestISearchToolbarIface ModestISearchToolbarIface;

struct _ModestISearchToolbarIface
{
	GTypeInterface parent;

	/*signals*/
	void (*isearch_close) (ModestISearchToolbar *self);
	void (*isearch_search) (ModestISearchToolbar *self);
	
	/* virtuals */
	void (*highlight_entry) (ModestISearchToolbar *self, gboolean get_focus);
	void (*set_label) (ModestISearchToolbar *self, const gchar *label);
	const gchar * (*get_search) (ModestISearchToolbar *self);

};

GType modest_isearch_toolbar_get_type (void);

void modest_isearch_toolbar_highlight_entry (ModestISearchToolbar *self, gboolean get_focus);
void modest_isearch_toolbar_set_label (ModestISearchToolbar *self, const gchar *label);
const gchar *modest_isearch_toolbar_get_search (ModestISearchToolbar *self);

G_END_DECLS

#endif
