/* modest-account-presets.c */

/* insert (c)/licensing information) */

#include "modest-account-presets.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                        modest_account_presets_class_init    (ModestAccountPresetsClass *klass);
static void                        modest_account_presets_init          (ModestAccountPresets *obj);
static void                        modest_account_presets_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountPresetsPrivate ModestAccountPresetsPrivate;
struct _ModestAccountPresetsPrivate {
	/* my private members go here, eg. */
	GKeyFile *preset_file;
	GList *preset_list;
};
#define MODEST_ACCOUNT_PRESETS_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_ACCOUNT_PRESETS, \
                                                    ModestAccountPresetsPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_account_presets_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountPresetsClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_account_presets_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountPresets),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_account_presets_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestAccountPresets",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_presets_class_init (ModestAccountPresetsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_presets_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountPresetsPrivate));

	klass->get_list = modest_account_presets_get_list;
	klass->get_names = modest_account_presets_get_names;
	klass->get_by_name = modest_account_presets_get_by_name;
	klass->load_file = modest_account_presets_load_file;
	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_account_presets_init (ModestAccountPresets *obj)
{
 	ModestAccountPresetsPrivate *priv = MODEST_ACCOUNT_PRESETS_GET_PRIVATE(obj);

 	priv->preset_file = g_key_file_new ();
	priv->preset_list = NULL;
	obj->count = 0;	
}

static void
modest_account_presets_finalize (GObject *obj)
{
 	ModestAccountPresetsPrivate *priv = MODEST_ACCOUNT_PRESETS_GET_PRIVATE(obj);

 	g_object_unref (priv->preset_file);
}

GObject*
modest_account_presets_new (void)
{
	return G_OBJECT(g_object_new(MODEST_TYPE_ACCOUNT_PRESETS, NULL));
}

/* method implementations */

GList *
modest_account_presets_get_list (ModestAccountPresets *self)
{
	
}

GList *
modest_account_presets_get_names (ModestAccountPresets *self)
{
}

ModestPreset *
modest_account_presets_get_by_name (ModestAccountPresets *self, const gchar *name)
{
	
}

gboolean 
modest_account_presets_load_file (ModestAccountPresets *self, const gchar *filename)
{
	
	return TRUE;
}
