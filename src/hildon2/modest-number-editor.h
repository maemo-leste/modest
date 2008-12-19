/*
 * This file is a part of modest
 *
 * Copyright (C) 2005, 2006, 2008 Nokia Corporation, all rights reserved.
 *
 */

#ifndef                                         __MODEST_NUMBER_EDITOR_H__
#define                                         __MODEST_NUMBER_EDITOR_H__

#include                                        <gtk/gtk.h>
#include                                        <hildon/hildon-entry.h>

G_BEGIN_DECLS

#define                                         MODEST_TYPE_NUMBER_EDITOR \
                                                (modest_number_editor_get_type())

#define                                         MODEST_NUMBER_EDITOR(obj) \
                                                (GTK_CHECK_CAST (obj, MODEST_TYPE_NUMBER_EDITOR, ModestNumberEditor))

#define                                         MODEST_NUMBER_EDITOR_CLASS(klass) \
                                                (GTK_CHECK_CLASS_CAST ((klass), MODEST_TYPE_NUMBER_EDITOR, \
                                                ModestNumberEditorClass))

#define                                         MODEST_IS_NUMBER_EDITOR(obj) \
                                                (GTK_CHECK_TYPE (obj, MODEST_TYPE_NUMBER_EDITOR))

#define                                         MODEST_IS_NUMBER_EDITOR_CLASS(klass) \
                                                (GTK_CHECK_CLASS_TYPE ((klass), MODEST_TYPE_NUMBER_EDITOR))

#define                                         MODEST_NUMBER_EDITOR_GET_CLASS(obj) \
                                                ((ModestNumberEditorClass *) G_OBJECT_GET_CLASS(obj))

typedef struct                                  _ModestNumberEditor ModestNumberEditor;

typedef struct                                  _ModestNumberEditorClass ModestNumberEditorClass;

struct                                          _ModestNumberEditor 
{
    HildonEntry parent;
};

typedef enum
{
    MODEST_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED,
    MODEST_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED,
    MODEST_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE
}                                               ModestNumberEditorErrorType;

struct                                          _ModestNumberEditorClass 
{
    GtkEntryClass parent_class;
  
    gboolean  (*range_error)  (ModestNumberEditor *editor, ModestNumberEditorErrorType type); 
};

GType G_GNUC_CONST
modest_number_editor_get_type                   (void);

GtkWidget*  
modest_number_editor_new                        (gint min, gint max);

void
modest_number_editor_set_range                  (ModestNumberEditor *editor, 
                                                 gint min,
                                                 gint max);

gint
modest_number_editor_get_value                  (ModestNumberEditor *editor);

void
modest_number_editor_set_value                  (ModestNumberEditor *editor, 
                                                 gint value);


GType modest_number_editor_error_type_get_type (void);
#define MODEST_TYPE_NUMBER_EDITOR_ERROR_TYPE (modest_number_editor_error_type_get_type())

G_END_DECLS

#endif                                          /* __MODEST_NUMBER_EDITOR_H__ */
