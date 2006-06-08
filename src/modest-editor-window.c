/* modest-editor-window.c */

/* insert (c)/licensing information) */

#include "modest-ui.h"
#include "modest-editor-window.h"

/* 'private'/'protected' functions */
static void                      modest_editor_window_class_init    (ModestEditorWindowClass *klass);
static void                      modest_editor_window_init          (ModestEditorWindow *obj);
static void                      modest_editor_window_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestEditorWindowPrivate ModestEditorWindowPrivate;
struct _ModestEditorWindowPrivate {
	gpointer user_data;
	gboolean modified;
};
#define MODEST_EDITOR_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                  MODEST_TYPE_EDITOR_WINDOW, \
                                                  ModestEditorWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_editor_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestEditorWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_editor_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestEditorWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_editor_window_init,
		};
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
		                                  "ModestEditorWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_editor_window_class_init (ModestEditorWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_editor_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestEditorWindowPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_editor_window_init (ModestEditorWindow *obj)
{
	ModestEditorWindowPrivate *priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(obj);

	priv->user_data = NULL;
	priv->modified = FALSE;
}

static void
modest_editor_window_finalize (GObject *obj)
{
	ModestEditorWindowPrivate *priv;

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(obj);

	if (priv->user_data)
		g_free(priv->user_data);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_editor_window_new (ModestUI *ui)
{
	GObject *self;
	ModestEditorWindowPrivate *priv;
	GtkWidget *w;
	gpointer data;

	self = G_OBJECT(g_object_new(MODEST_TYPE_EDITOR_WINDOW, NULL));
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(self);

	/* for now create a local test-window */

	data = NULL;
	w = GTK_WIDGET(modest_ui_new_editor_window(ui, &data));
	// g_message("new data = %p", data);
	if (!w)
		return NULL;
	if (!data)
		g_message("editor window user data is emtpy");

	gtk_container_add(GTK_CONTAINER(self), w);
	priv->user_data = data;
	// g_message("new priv->data = %p", priv->user_data);
	
	return GTK_WIDGET(self);
}

/*
 * return user defined data from a ModestEditorWindow instance
 * like e.g. a refernce to a GladeXML*
 */
gpointer modest_editor_window_get_data(ModestEditorWindow *edit_win)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win) {
		return NULL;
	}
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	// g_message("get priv->data = %p", priv->user_data);

	return priv->user_data;
}

gboolean modest_editor_window_set_modified(ModestEditorWindow *edit_win, gboolean modified)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win) {
		return FALSE;
	}
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	priv->modified = modified;

	return priv->modified;
}

gboolean modest_editor_window_get_modified(ModestEditorWindow *edit_win)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win) {
		return FALSE;
	}
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	// g_message("get priv->data = %p", priv->user_data);

	return priv->modified;
}	

gboolean modest_editor_window_set_to_header(ModestEditorWindow *edit_win, const gchar *to)
{
	ModestEditorWindowPrivate *priv;

	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_to_header(to, priv->user_data);
}


gboolean modest_editor_window_set_cc_header(ModestEditorWindow *edit_win, const gchar *cc)
{
	ModestEditorWindowPrivate *priv;

	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_cc_header(cc, priv->user_data);
}

gboolean modest_editor_window_set_bcc_header(ModestEditorWindow *edit_win, const gchar *bcc)
{
	ModestEditorWindowPrivate *priv;

	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_bcc_header(bcc, priv->user_data);
}

gboolean modest_editor_window_set_subject_header(ModestEditorWindow *edit_win, const gchar *subject)
{
	ModestEditorWindowPrivate *priv;

	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_subject_header(subject, priv->user_data);
}

gboolean modest_editor_window_set_body(ModestEditorWindow *edit_win, const gchar *body)
{
	ModestEditorWindowPrivate *priv;

	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_body(body, priv->user_data);
}
