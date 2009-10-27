/* Copyright (c) 2009, Nokia Corporation
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

#include <hildon/hildon.h>
#include <modest-hildon2-msg-view-window.h>

/* 'private'/'protected' functions */
static void modest_hildon2_msg_view_window_class_init  (ModestHildon2MsgViewWindowClass *klass);
static void modest_hildon2_msg_view_window_instance_init (ModestHildon2MsgViewWindow *obj);
static void modest_hildon2_msg_view_window_finalize    (GObject *obj);

/* globals */
static GtkWindowClass *parent_class = NULL;

/************************************************************************/

GType
modest_hildon2_msg_view_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestHildon2MsgViewWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_hildon2_msg_view_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestHildon2MsgViewWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_hildon2_msg_view_window_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_MSG_VIEW_WINDOW,
		                                  "ModestHildon2MsgViewWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_hildon2_msg_view_window_class_init (ModestHildon2MsgViewWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_hildon2_msg_view_window_finalize;

}

static void
modest_hildon2_msg_view_window_instance_init (ModestHildon2MsgViewWindow *obj)
{
	hildon_program_add_window (hildon_program_get_instance(),
				   HILDON_WINDOW(obj));
}

static void
modest_hildon2_msg_view_window_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestWindow *
modest_hildon2_msg_view_window_new (void)
{
	return modest_hildon2_msg_view_window_new ();
}

