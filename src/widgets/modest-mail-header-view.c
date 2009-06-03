/* Copyright (c) 2008, Nokia Corporation
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

#include <widgets/modest-mail-header-view.h>
#include <modest-marshal.h>

enum {
	RECPT_ACTIVATED_SIGNAL,
	SHOW_DETAILS_SIGNAL,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {0};

TnyHeaderFlags
modest_mail_header_view_get_priority (ModestMailHeaderView *self)
{
	return MODEST_MAIL_HEADER_VIEW_GET_IFACE (self)->get_priority (self);
}

void
modest_mail_header_view_set_priority (ModestMailHeaderView *self, TnyHeaderFlags flags)
{
	MODEST_MAIL_HEADER_VIEW_GET_IFACE (self)->set_priority (self, flags);
}

gboolean
modest_mail_header_view_get_loading (ModestMailHeaderView *self)
{
	return MODEST_MAIL_HEADER_VIEW_GET_IFACE (self)->get_loading (self);
}

void
modest_mail_header_view_set_loading (ModestMailHeaderView *self, gboolean is_loading)
{
	MODEST_MAIL_HEADER_VIEW_GET_IFACE (self)->set_loading (self, is_loading);
}

void
modest_mail_header_view_set_branding (ModestMailHeaderView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	MODEST_MAIL_HEADER_VIEW_GET_IFACE (self)->set_branding (self, brand_name, brand_icon);
}

const GtkWidget *
modest_mail_header_view_add_custom_header (ModestMailHeaderView *self,
					   const gchar *label,
					   GtkWidget *custom_widget,
					   gboolean with_expander,
					   gboolean start)
{
	return MODEST_MAIL_HEADER_VIEW_GET_IFACE (self)->add_custom_header (self, label, custom_widget, 
									    with_expander, start);
}

static void
modest_mail_header_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		
		signals[RECPT_ACTIVATED_SIGNAL] =
			g_signal_new ("recpt_activated",
				      MODEST_TYPE_MAIL_HEADER_VIEW,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestMailHeaderViewIface, recpt_activated),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__STRING,
				      G_TYPE_NONE, 1, G_TYPE_STRING);
		
		signals[SHOW_DETAILS_SIGNAL] =
			g_signal_new ("show_details",
				      MODEST_TYPE_MAIL_HEADER_VIEW,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestMailHeaderViewIface, show_details),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);
		
		initialized = TRUE;
	}
}

GType
modest_mail_header_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMailHeaderViewIface),
			modest_mail_header_view_base_init,   /* base init */
			NULL,		/* base finalize */
			NULL,           /* class_init */
			NULL,		/* class finalize */
			NULL,		/* class data */
			0,
			0,		/* n_preallocs */
			NULL,           /* instance_init */
			NULL
		};

 		my_type = g_type_register_static (G_TYPE_INTERFACE,
		                                  "ModestMailHeaderView",
		                                  &my_info, 0);

		g_type_interface_add_prerequisite (my_type,
						   TNY_TYPE_HEADER_VIEW);

	}
	return my_type;
}
