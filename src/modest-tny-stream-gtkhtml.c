/* modest-tny-stream-gtkhtml.c */

/* insert (c)/licensing information) */

#include "modest-tny-stream-gtkhtml.h"
#include <gtkhtml/gtkhtml-stream.h>
#include <gtkhtml/gtkhtml-search.h>

/* 'private'/'protected' functions */
static void  modest_tny_stream_gtkhtml_class_init   (ModestTnyStreamGtkhtmlClass *klass);
static void  modest_tny_stream_gtkhtml_init         (ModestTnyStreamGtkhtml *obj);
static void  modest_tny_stream_gtkhtml_finalize     (GObject *obj);

static void  modest_tny_stream_gtkhml_iface_init (gpointer g_iface, gpointer iface_data);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyStreamGtkhtmlPrivate ModestTnyStreamGtkhtmlPrivate;
struct _ModestTnyStreamGtkhtmlPrivate {
	GtkHTMLStream *stream;
};
#define MODEST_TNY_STREAM_GTKHTML_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                       MODEST_TYPE_TNY_STREAM_GTKHTML, \
                                                       ModestTnyStreamGtkhtmlPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_stream_gtkhtml_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyStreamGtkhtmlClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_stream_gtkhtml_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyStreamGtkhtml),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_stream_gtkhtml_init,
		};

		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) modest_tny_stream_gtkhml_iface_init,
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
                };

		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestTnyStreamGtkhtml",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_STREAM_IFACE,
					     &iface_info);

	}
	return my_type;
}

static void
modest_tny_stream_gtkhtml_class_init (ModestTnyStreamGtkhtmlClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_stream_gtkhtml_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyStreamGtkhtmlPrivate));
}

static void
modest_tny_stream_gtkhtml_init (ModestTnyStreamGtkhtml *obj)
{
	ModestTnyStreamGtkhtmlPrivate *priv;
	priv = MODEST_TNY_STREAM_GTKHTML_GET_PRIVATE(obj);

	priv->stream  = NULL;
}

static void
modest_tny_stream_gtkhtml_finalize (GObject *obj)
{
	ModestTnyStreamGtkhtmlPrivate *priv;

	priv = MODEST_TNY_STREAM_GTKHTML_GET_PRIVATE(obj);
	priv->stream = NULL;
}

GObject*
modest_tny_stream_gtkhtml_new (GtkHTMLStream *stream)
{
	GObject *obj;
	ModestTnyStreamGtkhtmlPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_STREAM_GTKHTML, NULL));
	priv = MODEST_TNY_STREAM_GTKHTML_GET_PRIVATE(obj);

	g_return_val_if_fail (stream, NULL);
	
	priv->stream = stream;

	return obj;
}


/* the rest are interface functions */


static ssize_t
gtkhtml_read (TnyStreamIface *self, char *buffer, size_t n)
{
	return -1; /* we cannot read */
}


static ssize_t
gtkhtml_write (TnyStreamIface *self, const char *buffer, size_t n)
{
	ModestTnyStreamGtkhtmlPrivate *priv;
	
	g_return_val_if_fail (self, 0);

	priv = MODEST_TNY_STREAM_GTKHTML_GET_PRIVATE(self);
	if (!priv->stream)
		g_warning ("cannot write to closed stream");
	else
		gtk_html_stream_write (priv->stream, buffer, n);
	
	return n; /* hmmm */
}

	
static gint
gtkhtml_flush (TnyStreamIface *self)
{
	return 0;
}
	

static gint
gtkhtml_close (TnyStreamIface *self)
{
	ModestTnyStreamGtkhtmlPrivate *priv;
	g_return_val_if_fail (self, 0);
	priv = MODEST_TNY_STREAM_GTKHTML_GET_PRIVATE(self);
	
	gtk_html_stream_close   (priv->stream, GTK_HTML_STREAM_OK);
	priv->stream = NULL;

	return 0;
}


static gboolean
gtkhtml_eos (TnyStreamIface *self)
{
	return TRUE;
}


	
static gint
gtkhtml_reset (TnyStreamIface *self)
{
	return 0;
}

	
static ssize_t
gtkhtml_write_to_stream (TnyStreamIface *self, TnyStreamIface *output)
{
	return 0;
}


static void
modest_tny_stream_gtkhml_iface_init (gpointer g_iface, gpointer iface_data)
{
        TnyStreamIfaceClass *klass;
	
	g_return_if_fail (g_iface);

	klass = (TnyStreamIfaceClass *)g_iface;
	
        klass->read_func  = gtkhtml_read;
        klass->write_func = gtkhtml_write;
        klass->flush_func = gtkhtml_flush;
        klass->close_func = gtkhtml_close;
	klass->eos_func   = gtkhtml_eos;
	klass->reset_func = gtkhtml_reset;
	klass->write_to_stream_func = gtkhtml_write_to_stream;
}
