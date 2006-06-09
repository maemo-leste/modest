/* modest-editor-window.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_EDITOR_WINDOW_H__
#define __MODEST_EDITOR_WINDOW_H__

#include <glib-object.h>
#include <gtk/gtkwindow.h>

#include "modest-ui.h"


G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_EDITOR_WINDOW             (modest_editor_window_get_type())
#define MODEST_EDITOR_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_EDITOR_WINDOW,ModestEditorWindow))
#define MODEST_EDITOR_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_EDITOR_WINDOW,GtkWindow))
#define MODEST_IS_EDITOR_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_EDITOR_WINDOW))
#define MODEST_IS_EDITOR_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_EDITOR_WINDOW))
#define MODEST_EDITOR_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_EDITOR_WINDOW,ModestEditorWindowClass))

typedef struct _ModestEditorWindow      ModestEditorWindow;
typedef struct _ModestEditorWindowClass ModestEditorWindowClass;

struct _ModestEditorWindow {
	 GtkWindow parent;
	/* insert public members, if any */
};

struct _ModestEditorWindowClass {
	GtkWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestEditorWindow* obj); */
};

/* member functions */
GType        modest_editor_window_get_type    (void) G_GNUC_CONST;


GtkWidget *modest_editor_window_new         (ModestUI *ui);

gpointer modest_editor_window_get_data(ModestEditorWindow *edit_win);

gboolean modest_editor_window_set_modified(ModestEditorWindow *edit_win, gboolean modified);

gboolean modest_editor_window_get_modified(ModestEditorWindow *edit_win);

/* fill in other public functions, eg.: */
/* 	void       modest_editor_window_do_something (ModestEditorWindow *self, const gchar* param); */
/* 	gboolean   modest_editor_window_has_foo      (ModestEditorWindow *self, gint value); */

gboolean modest_editor_window_set_to_header(ModestEditorWindow *edit_win, const gchar *to);

gboolean modest_editor_window_set_cc_header(ModestEditorWindow *edit_win, const gchar *cc);

gboolean modest_editor_window_set_bcc_header(ModestEditorWindow *edit_win, const gchar *bcc);

gboolean modest_editor_window_set_subject_header(ModestEditorWindow *edit_win, const gchar *subject);

gboolean modest_editor_window_set_body(ModestEditorWindow *edit_win, const gchar *body);

gboolean modest_editor_window_attach_file(ModestEditorWindow *edit_win, const gchar *filename);

GList * modest_editor_window_set_attachments(ModestEditorWindow *edit_win, GList* attachments);

GList * modest_editor_window_get_attachments(ModestEditorWindow *edit_win);

G_END_DECLS

#endif /* __MODEST_EDITOR_WINDOW_H__ */
