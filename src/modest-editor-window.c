/* modest-editor-window.c */

/* insert (c)/licensing information) */

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
}

static void
modest_editor_window_finalize (GObject *obj)
{
	ModestEditorWindowPrivate *priv;

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(obj);

	/* FIXME: free GladeXML */
}

GtkWindow*
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
	if (!w)
		return NULL;

	gtk_container_add(GTK_CONTAINER(self), w);
	priv->user_data = data;
	
	return GTK_WINDOW(self);
}


gpointer modest_editor_window_get_data(ModestEditorWindow *edit_win)
{
	GObject *self;
	ModestEditorWindowPrivate *priv;

	
	if (!edit_win)
		return NULL;
	self = G_OBJECT(g_object_new(MODEST_TYPE_EDITOR_WINDOW, NULL));
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(self);

	return priv->user_data;
}


gboolean modest_editor_window_set_to_header(ModestEditorWindow *edit_win, gchar *to)
{
	return modest_ui_editor_window_set_to_header(edit_win, to);
}
