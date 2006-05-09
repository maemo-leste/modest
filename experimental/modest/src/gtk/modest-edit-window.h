/* modest-edit-window.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_EDIT_WINDOW_H__
#define __MODEST_EDIT_WINDOW_H__

#include <gtk/gtkwindow.h>
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_EDIT_WINDOW             (modest_edit_window_get_type())
#define MODEST_EDIT_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_EDIT_WINDOW,ModestEditWindow))
#define MODEST_EDIT_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_EDIT_WINDOW,GtkWindow))
#define MODEST_IS_EDIT_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_EDIT_WINDOW))
#define MODEST_IS_EDIT_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_EDIT_WINDOW))
#define MODEST_EDIT_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_EDIT_WINDOW,ModestEditWindowClass))

typedef struct _ModestEditWindow      ModestEditWindow;
typedef struct _ModestEditWindowClass ModestEditWindowClass;

struct _ModestEditWindow {
	 GtkWindow parent;	
};

struct _ModestEditWindowClass {
	GtkWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestEditWindow* obj); */
};

/* member functions */
GType        modest_edit_window_get_type    (void) G_GNUC_CONST;


GtkWidget*   modest_edit_window_new         (const gchar *to,
					     const gchar *cc,
					     const gchar *bcc,
					     const gchar *subject,
					     const gchar *body,
					     const GSList *attachments);
/* fill in other public functions, eg.: */
/* 	void       modest_edit_window_do_something (ModestEditWindow *self, const gchar* param); */
/* 	gboolean   modest_edit_window_has_foo      (ModestEditWindow *self, gint value); */


G_END_DECLS

#endif /* __MODEST_EDIT_WINDOW_H__ */

