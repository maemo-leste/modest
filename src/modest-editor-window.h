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
};

/* member functions */
GType        modest_editor_window_get_type    (void) G_GNUC_CONST;


/**
 * modest_editor_window_new:
 * @ui: a ModestUI instance
 *
 * Creates a new editor window instance which is a subclass of GtkWindow and
 * lives in the ModestUI context *ui
 * It uses an interface function modest_ui_new_editor_window() to create the
 * the actual window contents (see its documentation for the interface specification)
 *
 * Returns: a GtkWindow* to show and populate
 */
GtkWidget *modest_editor_window_new         (ModestUI *ui);

/**
 * modest_editor_window_get_data:
 * @edit_win: a ModestEditorWindow instance
 *
 * Retrieves the generic data pointer from the ModestEditorWindow (which
 * the UI interface code can use to store arbitrary (state-)data in the
 * ModestEditorWindow widget.
 *
 * Returns: a gpointer to the ModestEditorWindow's data store
 */
gpointer modest_editor_window_get_data(ModestEditorWindow *edit_win);

/**
 * modest_editor_window_set_modified:
 * @edit_win: a ModestEditorWindow instance
 * @modified: the modified flag for this instance
 *
 * Set/reset the modified flag for the instance. This flag can be used to store
 * information whether the contents of the editor window was modified by the
 * user or not. If it was modified (see also modest_editor_window_get_modified())
 * a dialog can be presented to ask the user for confirmation.
 *
 * Returns: gboolean the new state of the modified flag
 */
gboolean modest_editor_window_set_modified(ModestEditorWindow *edit_win, gboolean modified);

/**
 * modest_editor_window_get_modified:
 * @edit_win: a ModestEditorWindow instance
 *
 * Gets the state of the modified-flag of this instance. This flag can be used to store
 * information whether the contents of the editor window was modified by the
 * user or not. If it was modified (see also modest_editor_window_get_modified())
 * a dialog can be presented to ask the user for confirmation.
 *
 * Returns: gboolean the new state of the modified flag
 */
gboolean modest_editor_window_get_modified(ModestEditorWindow *edit_win);

/**
 * modest_editor_window_set_to_header:
 * @edit_win: a ModestEditorWindow instance
 * @to: The "To:" header string for this editor instance
 *
 * Sets the "To:" header to the string *to
 *
 * Returns: TRUE on success, FALSE otherwise
 */
gboolean modest_editor_window_set_to_header(ModestEditorWindow *edit_win, const gchar *to);

/**
 * modest_editor_window_set_cc_header:
 * @edit_win: a ModestEditorWindow instance
 * @cc: The "CC:" header string for this editor instance
 *
 * Sets the "CC:" header to the string *cc
 *
 * Returns: TRUE on success, FALSE otherwise
 */
gboolean modest_editor_window_set_cc_header(ModestEditorWindow *edit_win, const gchar *cc);

/**
 * modest_editor_window_set_bcc_header:
 * @edit_win: a ModestEditorWindow instance
 * @bcc: The "BCC:" header string for this editor instance
 *
 * Sets the "BCC:" header to the string *bcc
 *
 * Returns: TRUE on success, FALSE otherwise
 */
gboolean modest_editor_window_set_bcc_header(ModestEditorWindow *edit_win, const gchar *bcc);

/**
 * modest_editor_window_set_subject_header:
 * @edit_win: a ModestEditorWindow instance
 * @subject: The "Subject:" header string for this editor instance
 *
 * Sets the "Subject:" header to the string *subject
 *
 * Returns: TRUE on success, FALSE otherwise
 */
gboolean modest_editor_window_set_subject_header(ModestEditorWindow *edit_win, const gchar *subject);

/**
 * modest_editor_window_set_body:
 * @edit_win: a ModestEditorWindow instance
 * @body: The message body string for this editor instance
 *
 * Sets the message body to the string *body
 *
 * Returns: TRUE on success, FALSE otherwise
 */
gboolean modest_editor_window_set_body(ModestEditorWindow *edit_win, const gchar *body);

/**
 * modest_editor_window_attach_file:
 * @edit_win: a ModestEditorWindow instance
 * @filename: The name of the file to attach
 *
 * Attaches the file "filename" to the message contents
 *
 * Returns: TRUE on success, FALSE otherwise
 */
gboolean modest_editor_window_attach_file(ModestEditorWindow *edit_win, const gchar *filename);

/**
 * modest_editor_window_set_attachments:
 * @edit_win: a ModestEditorWindow instance
 * @attachments: a list of attachments
 *
 * Sets the list of attachments to *attachments
 *
 * Returns: The new GList* of attachments.
 */
GList * modest_editor_window_set_attachments(ModestEditorWindow *edit_win, GList* attachments);

/**
 * modest_editor_window_get_attachments:
 * @edit_win: a ModestEditorWindow instance
 *
 * Gets the GList* of attachments of this instance
 *
 * Returns: The GList* of attachments of this instance
 */
GList * modest_editor_window_get_attachments(ModestEditorWindow *edit_win);

G_END_DECLS

#endif /* __MODEST_EDITOR_WINDOW_H__ */
