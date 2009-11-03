/* Copyright (c) 2008 Nokia Corporation
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

#ifndef __MODEST_HILDON2_WINDOW_H__
#define __MODEST_HILDON2_WINDOW_H__

#include <widgets/modest-window.h>
#include <widgets/modest-account-view.h>
#include <modest-dimming-rule.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_HILDON2_WINDOW             (modest_hildon2_window_get_type())
#define MODEST_HILDON2_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_HILDON2_WINDOW,ModestHildon2Window))
#define MODEST_HILDON2_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_HILDON2_WINDOW,ModestWindow))

#define MODEST_IS_HILDON2_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_HILDON2_WINDOW))
#define MODEST_IS_HILDON2_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_HILDON2_WINDOW))
#define MODEST_HILDON2_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_HILDON2_WINDOW,ModestHildon2WindowClass))

typedef struct _ModestHildon2Window      ModestHildon2Window;
typedef struct _ModestHildon2WindowClass ModestHildon2WindowClass;

struct _ModestHildon2Window {
	ModestWindow parent;
};

struct _ModestHildon2WindowClass {
	ModestWindowClass parent_class;

	void (*pack_toolbar_func) (ModestHildon2Window *self, GtkPackType pack_type, GtkWidget *toolbar);
	void (*edit_mode_changed) (ModestHildon2Window *self, gint edit_mode, gboolean enabled);
};

typedef gboolean (*ModestHildon2EditModeCallback) (ModestHildon2Window *self);

#define EDIT_MODE_CALLBACK(x) ((ModestHildon2EditModeCallback) (x))

/* edit mode id for no edit mode */
#define MODEST_HILDON2_WINDOW_EDIT_MODE_NONE -1

/**
 * modest_hildon2_window_get_type:
 * 
 * get the GType for the ModestHildon2Window class
 *
 * Returns: a GType for ModestHildon2Window
 */
GType modest_hildon2_window_get_type (void) G_GNUC_CONST;


/**
 * modest_hildon2_window_pack_toolbar:
 * @self: a #ModestHildon2Window
 * @pack_type: a #GtkPackType
 * @toolbar: a toolbar widget
 *
 * packs a toolbar (widget @toolbar) in @self with @pack_type
 */
void modest_hildon2_window_pack_toolbar (ModestHildon2Window *self,
					 GtkPackType pack_type,
					 GtkWidget *toolbar);

void modest_hildon2_window_register_edit_mode (ModestHildon2Window *self,
					       gint edit_mode_id,
					       const gchar *description,
					       const gchar *button,
					       GtkTreeView *tree_view,
					       GtkSelectionMode mode,
					       ModestHildon2EditModeCallback action);

void modest_hildon2_window_set_edit_mode (ModestHildon2Window *self,
					  gint edit_mode_id);
void modest_hildon2_window_unset_edit_mode (ModestHildon2Window *self);

G_END_DECLS

#endif
