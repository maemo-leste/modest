/* modest-account-presets.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_ACCOUNT_PRESETS_H__
#define __MODEST_ACCOUNT_PRESETS_H__

#include <glib-object.h>
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_PRESETS             (modest_account_presets_get_type())
#define MODEST_ACCOUNT_PRESETS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_PRESETS,ModestAccountPresets))
#define MODEST_ACCOUNT_PRESETS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_PRESETS,GObject))
#define MODEST_IS_ACCOUNT_PRESETS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_PRESETS))
#define MODEST_IS_ACCOUNT_PRESETS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_PRESETS))
#define MODEST_ACCOUNT_PRESETS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_PRESETS,ModestAccountPresetsClass))

typedef struct _ModestAccountPresets      ModestAccountPresets;
typedef struct _ModestAccountPresetsClass ModestAccountPresetsClass;

typedef struct _ModestPreset      ModestPreset;

struct _ModestAccountPresets {
	GObject parent;
	/* public members */
	gint count;	/* number of available presets */
};

struct _ModestAccountPresetsClass {
	GObjectClass parent_class;
	GList * (* get_list) (ModestAccountPresets *self);
	GList * (* get_names) (ModestAccountPresets *self);
	ModestPreset * (* get_by_name) (ModestAccountPresets *self, const gchar *name);
	gboolean (* load_file) (ModestAccountPresets *self, const gchar *filename);
};

/* data type to hold an account preset dataset */
struct _ModestPreset {
	gchar *name;
	gchar *transport_server;
	gchar *storage_server;
	gint transport_port;
	gint storage_port;
	/* add security stuff */
	gchar *note;
};

/* member functions */
GType        modest_account_presets_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
GObject*    modest_account_presets_new         (void);

/* public methods */
GList *modest_account_presets_get_list (ModestAccountPresets *self);
GList *modest_account_presets_get_names (ModestAccountPresets *self);
ModestPreset *modest_account_presets_get_by_name (ModestAccountPresets *self, const gchar *name);
gboolean modest_account_presets_load_file (ModestAccountPresets *self, const gchar *filename);


G_END_DECLS

#endif /* __MODEST_ACCOUNT_PRESETS_H__ */
