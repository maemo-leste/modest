/*
 * Copyright (C) 2008 Nokia Corporation, all rights reserved.
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

/**
 * SECTION:modest-number-editor
 * @short_description: A widget used to enter a number within a pre-defined range.
 *
 * ModestNumberEditor is used to enter a number from a specific range. 
 * There are two buttons to scroll the value in number field. 
 * Manual input is also possible.
 *
 * <example>
 * <title>ModestNumberEditor example</title>
 * <programlisting>
 * number_editor = modest_number_editor_new (-250, 500);
 * modest_number_editor_set_range (number_editor, 0, 100);
 * </programlisting>
 * </example>
 */

#undef                                          MODEST_DISABLE_DEPRECATED

#ifdef                                          HAVE_CONFIG_H
#include                                        <config.h>
#endif

#include                                        <string.h>
#include                                        <stdio.h>
#include                                        <stdlib.h>
#include                                        <libintl.h>
#include                                        <gdk/gdkkeysyms.h>

#include                                        "modest-number-editor.h"
#include                                        "modest-marshal.h"
#include                                        <hildon/hildon-banner.h>
#include                                        "modest-text-utils.h"

#define                                         _(String) dgettext("modest-libs", String)

typedef struct                                  _ModestNumberEditorPrivate ModestNumberEditorPrivate;

#define                                         MODEST_NUMBER_EDITOR_GET_PRIVATE(obj) \
                                                (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MODEST_TYPE_NUMBER_EDITOR, \
                                                ModestNumberEditorPrivate));

struct                                          _ModestNumberEditorPrivate
{
	gint start; /* Minimum */
	gint end;   /* Maximum */
	gint default_val;
	gboolean is_valid;

	/* Timer IDs */
	guint select_all_idle_id; /* Selection repaint hack
				     see modest_number_editor_select_all */
};


static void
modest_number_editor_class_init                 (ModestNumberEditorClass *editor_class);

static void
modest_number_editor_init                       (ModestNumberEditor *editor);

static gboolean
modest_number_editor_entry_focusout             (GtkWidget *widget, 
                                                 GdkEventFocus *event,
                                                 gpointer data);

static void
modest_number_editor_entry_changed              (GtkWidget *widget, 
                                                 gpointer data);

static void
modest_number_editor_finalize                   (GObject *self);

static gboolean
modest_number_editor_range_error                (ModestNumberEditor *editor,
                                                 ModestNumberEditorErrorType type);

static gboolean
modest_number_editor_select_all                 (ModestNumberEditor *editor);

static void
modest_number_editor_validate_value             (ModestNumberEditor *editor, 
                                                 gboolean allow_intermediate);
    
static void 
modest_number_editor_set_property               (GObject * object,
                                                 guint prop_id,
                                                 const GValue * value,
                                                 GParamSpec * pspec);

static void
modest_number_editor_get_property               (GObject *object,
                                                 guint prop_id,
                                                 GValue *value, 
                                                 GParamSpec * pspec);

enum
{
    RANGE_ERROR,
    VALID_CHANGED,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VALUE
};

static GtkContainerClass*                       parent_class;

static guint                                    ModestNumberEditor_signal[LAST_SIGNAL] = {0};

/**
 * modest_number_editor_get_type:
 *
 * Returns GType for ModestNumberEditor.
 *
 * Returns: ModestNumberEditor type
 */
GType G_GNUC_CONST
modest_number_editor_get_type                   (void)
{
    static GType editor_type = 0;

    if (!editor_type)
    {
        static const GTypeInfo editor_info =
        {
            sizeof (ModestNumberEditorClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) modest_number_editor_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof (ModestNumberEditor),
            0,  /* n_preallocs */
            (GInstanceInitFunc) modest_number_editor_init,
        };
        editor_type = g_type_register_static (HILDON_TYPE_ENTRY,
                "ModestNumberEditor",
                &editor_info, 0);
    }
    return editor_type;
}

static void
modest_number_editor_class_init                 (ModestNumberEditorClass *editor_class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (editor_class);

    g_type_class_add_private (editor_class,
            sizeof (ModestNumberEditorPrivate));

    parent_class = g_type_class_peek_parent (editor_class);

    editor_class->range_error = modest_number_editor_range_error;

    gobject_class->finalize                 = modest_number_editor_finalize;
    gobject_class->set_property             = modest_number_editor_set_property;
    gobject_class->get_property             = modest_number_editor_get_property;

    /**
     * ModestNumberEditor:value:
     *
     * The current value of the number editor.
     */
    g_object_class_install_property (gobject_class, PROP_VALUE,
            g_param_spec_int ("value",
                "Value",
                "The current value of number editor",
                G_MININT,
                G_MAXINT,
                0, G_PARAM_READWRITE));

    ModestNumberEditor_signal[RANGE_ERROR] =
        g_signal_new ("range_error", MODEST_TYPE_NUMBER_EDITOR,
                G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET
                (ModestNumberEditorClass, range_error),
                g_signal_accumulator_true_handled, NULL,
                modest_marshal_BOOLEAN__ENUM,
                G_TYPE_BOOLEAN, 1, MODEST_TYPE_NUMBER_EDITOR_ERROR_TYPE);

    ModestNumberEditor_signal[VALID_CHANGED] =
        g_signal_new ("valid_changed", MODEST_TYPE_NUMBER_EDITOR,
                G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET
                (ModestNumberEditorClass, valid_changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
modest_number_editor_finalize                   (GObject *self)
{
    ModestNumberEditorPrivate *priv;

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (self);
    g_assert (priv);

    if (priv->select_all_idle_id)
        g_source_remove (priv->select_all_idle_id);

    /* Call parent class finalize, if have one */
    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize(self);
}

static void
modest_number_editor_init                       (ModestNumberEditor *editor)
{
    ModestNumberEditorPrivate *priv;

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
    g_assert (priv);

    priv->select_all_idle_id = 0;
    priv->is_valid = TRUE;
}

/* Format given number to editor field, no checks performed, all signals
   are sent normally. */
static void
modest_number_editor_real_set_value             (ModestNumberEditor *editor, 
                                                 gint value)
{
    /* FIXME: That looks REALLY bad */
    gchar buffer[32];

    /* Update text in entry to new value */
    g_snprintf (buffer, sizeof (buffer), "%d", value);
    gtk_entry_set_text (GTK_ENTRY (editor), buffer);
}

static void
add_select_all_idle                             (ModestNumberEditor *editor)
{
    ModestNumberEditorPrivate *priv;

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);

    if (! priv->select_all_idle_id)
    {
        priv->select_all_idle_id =
            g_idle_add((GSourceFunc) modest_number_editor_select_all, editor);
    }    
}

static void
modest_number_editor_validate_value             (ModestNumberEditor *editor, 
                                                 gboolean allow_intermediate)
{
	ModestNumberEditorPrivate *priv;
	gint error_code, fixup_value;
	const gchar *text;
	long value;
	gchar *tail;
	gboolean r;
	gboolean is_valid = TRUE;

	g_assert (MODEST_IS_NUMBER_EDITOR(editor));
	
	priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
	g_assert (priv);

	text = gtk_entry_get_text (GTK_ENTRY (editor));
	error_code = -1;
	fixup_value = priv->default_val;

	if (text && text[0]) {
		/* Try to convert entry text to number */
		value = strtol (text, &tail, 10);

		/* Check if conversion succeeded */
		if (tail[0] == 0) {
			/* Check if value is in allowed range. This is tricky in those
			   cases when user is editing a value. 
			   For example: Range = [100, 500] and user have just inputted "4".
			   This should not lead into error message. Otherwise value is
			   resetted back to "100" and next "4" press will reset it back
			   and so on. */

			if (allow_intermediate) {
				/* We now have the following error cases:
				 * If inputted value as above maximum and
				 maximum is either positive or then maximum
				 negative and value is positive.
				 * If inputted value is below minimum and minimum
				 is negative or minumum positive and value
				 negative or zero.
				 In all other cases situation can be fixed just by
				 adding new numbers to the string.
				*/
				if (value > priv->end && (priv->end >= 0 || (priv->end < 0 && value >= 0))) {
					error_code = MODEST_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED;
					fixup_value = priv->end;
					is_valid = FALSE;
				} else if (value < priv->start && (priv->start < 0 || (priv->start >= 0 && value <= 0))) {
					error_code = MODEST_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED;
					fixup_value = priv->start;
					is_valid = FALSE;
				}
			} else {
				if (value > priv->end) {
					error_code = MODEST_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED;
					fixup_value = priv->end;
					is_valid = FALSE;
				} else if (value < priv->start) {
					error_code = MODEST_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED;
					fixup_value = priv->start;
					is_valid = FALSE;
				}
			}
			/* The only valid case when conversion can fail is when we
			   have plain '-', intermediate forms are allowed AND
			   minimum bound is negative */
		} else {
			is_valid = FALSE;
			if (! allow_intermediate || strcmp (text, "-") != 0 || priv->start >= 0)
				error_code = MODEST_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE;
		}
	} else {
		is_valid = FALSE;
		if (! allow_intermediate)
			error_code = MODEST_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE;
	}

	if (error_code != -1) {
		/* If entry is empty and intermediate forms are nor allowed, 
		   emit error signal */
		/* Change to default value */
		modest_number_editor_set_value (editor, fixup_value);
		g_signal_emit (editor, ModestNumberEditor_signal[RANGE_ERROR], 0, error_code, &r);
		add_select_all_idle (editor);
		is_valid = modest_number_editor_is_valid (editor);
	}

	if (priv->is_valid != is_valid) {
		priv->is_valid = is_valid;
		g_signal_emit (editor, ModestNumberEditor_signal[VALID_CHANGED], 0, is_valid);
	}
}

static void 
modest_number_editor_entry_changed              (GtkWidget *widget, 
                                                 gpointer data)
{
    g_assert (MODEST_IS_NUMBER_EDITOR (data));
    modest_number_editor_validate_value (MODEST_NUMBER_EDITOR (data), TRUE);
    g_object_notify (G_OBJECT (data), "value");
}

static gboolean
modest_number_editor_entry_focusout             (GtkWidget *widget, 
                                                 GdkEventFocus *event,
                                                 gpointer data)
{
	GtkWidget *window;
	
	g_assert (MODEST_IS_NUMBER_EDITOR(data));

	window = gtk_widget_get_toplevel (widget);
	if (window && gtk_window_has_toplevel_focus (GTK_WINDOW (window)))
		modest_number_editor_validate_value (MODEST_NUMBER_EDITOR(data), FALSE);

	return FALSE;
}

static gboolean
modest_number_editor_range_error                (ModestNumberEditor *editor,
                                                 ModestNumberEditorErrorType type)
{

    gint min, max;
    gchar *err_msg = NULL;
    ModestNumberEditorPrivate *priv;

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
    g_assert (priv);

    min = priv->start;
    max = priv->end;

    /* Construct error message */
    switch (type)
    {
        case MODEST_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED:
            err_msg = g_strdup_printf (_HL("ckct_ib_maximum_value"), max, max);
            break;

        case MODEST_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED:
            err_msg = g_strdup_printf (_HL("ckct_ib_minimum_value"), min, min);
            break;

        case MODEST_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE:
            err_msg =
                g_strdup_printf (_HL("ckct_ib_set_a_value_within_range"), min, max);
            break;
    }

    /* Infoprint error */
    if (err_msg)
    {
        hildon_banner_show_information (GTK_WIDGET (GTK_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET(editor),
                                        GTK_TYPE_WINDOW))), NULL, err_msg);
        g_free(err_msg);
    }

    return TRUE;
}

/**
 * modest_number_editor_new:
 * @min: minimum accepted value
 * @max: maximum accepted value
 * 
 * Creates new number editor
 *
 * Returns: a new #ModestNumberEditor widget
 */
GtkWidget*
modest_number_editor_new                        (gint min, 
                                                 gint max)
{
    ModestNumberEditor *editor = g_object_new (MODEST_TYPE_NUMBER_EDITOR, NULL);

    /* Connect child widget signals */
    g_signal_connect (GTK_OBJECT (editor), "changed",
            G_CALLBACK (modest_number_editor_entry_changed),
            editor);

    g_signal_connect (GTK_OBJECT (editor), "focus-out-event",
            G_CALLBACK (modest_number_editor_entry_focusout),
            editor);

    /* Numeric input mode */
    hildon_gtk_entry_set_input_mode (GTK_ENTRY (editor), 
				     HILDON_GTK_INPUT_MODE_NUMERIC);
    hildon_gtk_widget_set_theme_size ((GtkWidget *) editor, 
				      HILDON_SIZE_FINGER_HEIGHT);

    /* Set user inputted range to editor */
    modest_number_editor_set_range (editor, min, max);

    return GTK_WIDGET (editor);
}

/**
 * modest_number_editor_set_range:
 * @editor: a #ModestNumberEditor widget
 * @min: minimum accepted value
 * @max: maximum accepted value
 *
 * Sets accepted number range for editor
 */
void
modest_number_editor_set_range                  (ModestNumberEditor *editor, 
                                                 gint min, 
                                                 gint max)
{
    ModestNumberEditorPrivate *priv;
    gchar buffer_min[32], buffer_max[32];
    gint a, b;

    g_return_if_fail (MODEST_IS_NUMBER_EDITOR (editor));

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
    g_assert (priv);

    /* Set preferences */
    priv->start = MIN (min, max);
    priv->end = MAX (min, max);

    /* Find maximum allowed length of value */
    g_snprintf (buffer_min, sizeof (buffer_min), "%d", min);
    g_snprintf (buffer_max, sizeof (buffer_max), "%d", max);
    a = strlen (buffer_min);
    b = strlen (buffer_max);

    /* Set maximum size of entry */
    gtk_entry_set_width_chars (GTK_ENTRY (editor), MAX (a, b));
    modest_number_editor_set_value (editor, priv->start);
}

/**
 * modest_number_editor_is_valid:
 * @editor: pointer to #ModestNumberEditor
 *
 * Returns: if @editor contents are valid
 */
gboolean
modest_number_editor_is_valid                  (ModestNumberEditor *editor)
{
    ModestNumberEditorPrivate *priv;

    g_return_val_if_fail (MODEST_IS_NUMBER_EDITOR (editor), FALSE);

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
    g_assert (priv);

    return priv->is_valid;

}

/**
 * modest_number_editor_get_value:
 * @editor: pointer to #ModestNumberEditor
 *
 * Returns: current NumberEditor value
 */
gint
modest_number_editor_get_value                  (ModestNumberEditor *editor)
{
    ModestNumberEditorPrivate *priv;

    g_return_val_if_fail (MODEST_IS_NUMBER_EDITOR (editor), 0);

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
    g_assert (priv);

    return atoi (gtk_entry_get_text (GTK_ENTRY (editor)));
}

/**
 * modest_number_editor_set_value:
 * @editor: pointer to #ModestNumberEditor
 * @value: numeric value for number editor
 *
 * Sets numeric value for number editor
 */
void
modest_number_editor_set_value                  (ModestNumberEditor *editor, 
                                                 gint value)
{
    ModestNumberEditorPrivate *priv;

    g_return_if_fail (MODEST_IS_NUMBER_EDITOR (editor));

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
    g_assert (priv);

    g_return_if_fail (value <= priv->end);
    g_return_if_fail (value >= priv->start);

    priv->default_val = value;
    modest_number_editor_real_set_value (editor, value);
    g_object_notify (G_OBJECT(editor), "value");
}

/* When calling gtk_entry_set_text, the entry widget does things that can
 * cause the whole widget to redraw. This redrawing is delayed and if any
 * selections are made right after calling the gtk_entry_set_text the
 * setting of the selection might seem to have no effect.
 *
 * If the selection is delayed with a lower priority than the redrawing,
 * the selection should stick. Calling this function with g_idle_add should
 * do it.
 */
static gboolean
modest_number_editor_select_all                 (ModestNumberEditor *editor)
{   
    ModestNumberEditorPrivate *priv;

    g_return_val_if_fail (MODEST_IS_NUMBER_EDITOR (editor), FALSE);

    priv = MODEST_NUMBER_EDITOR_GET_PRIVATE (editor);
    g_assert (priv);

    GDK_THREADS_ENTER ();
    gtk_editable_select_region (GTK_EDITABLE (editor), 0, -1);
    priv->select_all_idle_id = 0;
    GDK_THREADS_LEAVE ();
    return FALSE;
} 

static void
modest_number_editor_set_property               (GObject *object,
                                                 guint prop_id,
                                                 const GValue *value, 
                                                 GParamSpec *pspec)
{
    ModestNumberEditor *editor;

    editor = MODEST_NUMBER_EDITOR (object);

    switch (prop_id) {

        case PROP_VALUE:
            modest_number_editor_set_value (editor, g_value_get_int (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
modest_number_editor_get_property               (GObject *object,
                                                 guint prop_id, 
                                                 GValue *value, 
                                                 GParamSpec *pspec)
{
    ModestNumberEditor *editor;

    editor = MODEST_NUMBER_EDITOR (object);

    switch (prop_id) {

        case PROP_VALUE:
            g_value_set_int(value, modest_number_editor_get_value (editor));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


/* enumerations from "modest-number-editor.h" */
GType
modest_number_editor_error_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { MODEST_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED, "MODEST_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED", "maximum-value-exceed" },
      { MODEST_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED, "MODEST_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED", "minimum-value-exceed" },
      { MODEST_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE, "MODEST_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE", "erroneous-value" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("ModestNumberEditorErrorType", values);
  }
  return etype;
}

