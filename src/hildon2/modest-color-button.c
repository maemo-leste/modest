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

#include <config.h>
#include <modest-color-button.h>

#ifdef MODEST_USE_CALENDAR_WIDGETS
#include <calendar-ui-widgets.h>
#endif

/* 'private'/'protected' functions */
static void modest_color_button_class_init  (ModestColorButtonClass *klass);

#ifdef MODEST_USE_CALENDAR_WIDGETS
static void modest_color_button_clicked     (GtkButton *button);
#endif

/* globals */
static GtkWindowClass *parent_class = NULL;

/************************************************************************/

GType
modest_color_button_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestColorButtonClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_color_button_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestColorButton),
			1,		/* n_preallocs */
			NULL,
			NULL
		};
		my_type = g_type_register_static (HILDON_TYPE_COLOR_BUTTON,
		                                  "ModestColorButton",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_color_button_class_init (ModestColorButtonClass *klass)
{
#ifdef MODEST_USE_CALENDAR_WIDGETS
	GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

	button_class->clicked  = modest_color_button_clicked;
#endif

	parent_class = g_type_class_peek_parent (klass);
}

GtkWidget *
modest_color_button_new ()
{
	const gchar *theme;
	GtkWidget *button = (GtkWidget *) g_object_new (MODEST_TYPE_COLOR_BUTTON, NULL);

	/* For theming purpouses */
	theme = "widget_class \"*.GtkToolItem.ModestColorButton\" style \"osso-toolbutton\"";
	gtk_rc_parse_string (theme);

	return button;
}


#ifdef MODEST_USE_CALENDAR_WIDGETS
static void
modest_color_button_clicked (GtkButton *button)
{
	/* Show ColorPicker dialog */
	PipCalendarColor color = pip_color_picker_select_color(PipTextColorRed, PipColorPickerText);

	/* Check if some color is selected rather than dialog is dismissed */
	if (color != PipCalendarColorInvalid) {
		GdkColor *gdk_color = (GdkColor *) pip_calendar_color_get_gdkcolor(color);

		if (gdk_color)
			hildon_color_button_set_color ((HildonColorButton *) button, gdk_color);
	}
}
#endif
