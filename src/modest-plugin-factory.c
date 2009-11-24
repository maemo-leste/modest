/* Copyright (c) 2008, Nokia Corporation
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

#include "modest-plugin-factory.h"
#include "modest-protocol-registry.h"
#include "modest-plugin.h"
#include "modest-module.h"
#include <gmodule.h>
/* include other impl specific header files */

#define PLUGIN_EXT	".modest-mail-plugin"

/* 'private'/'protected' functions */
static void modest_plugin_factory_class_init (ModestPluginFactoryClass *klass);
static void modest_plugin_factory_init       (ModestPluginFactory *obj);
static void modest_plugin_factory_finalize   (GObject *obj);

static ModestPlugin* modest_plugin_factory_load     (const gchar *file);

typedef GType (*ModestModuleRegisterFunc) (GTypeModule *);

typedef struct _ModestPluginFactoryPrivate ModestPluginFactoryPrivate;
struct _ModestPluginFactoryPrivate {
	GSList *plugins;
};

#define MODEST_PLUGIN_FACTORY_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                        MODEST_TYPE_PLUGIN_FACTORY, \
                                                        ModestPluginFactoryPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_plugin_factory_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestPluginFactoryClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_plugin_factory_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestPluginFactory),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_plugin_factory_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestPluginFactory",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_plugin_factory_class_init (ModestPluginFactoryClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_plugin_factory_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestPluginFactoryPrivate));
}

static void
modest_plugin_factory_init (ModestPluginFactory *obj)
{
	ModestPluginFactoryPrivate *priv = MODEST_PLUGIN_FACTORY_GET_PRIVATE(obj);

	if (!g_module_supported ()) {
		g_warning ("unable to initialize the plugin factory");
		return;
	}

	priv->plugins = NULL;
	g_module_open (NULL, 0);
}

static void
modest_plugin_factory_finalize (GObject *obj)
{
	ModestPluginFactoryPrivate *priv;
	GSList *iter;

	priv = MODEST_PLUGIN_FACTORY_GET_PRIVATE (obj);

	/* Free the plugin list */
	for (iter = priv->plugins; iter; iter = g_slist_next (iter))
		g_object_unref ((GObject *) iter->data);

	g_slist_free (priv->plugins);
	priv->plugins = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestPluginFactory*
modest_plugin_factory_new (void)
{
	return MODEST_PLUGIN_FACTORY (g_object_new (MODEST_TYPE_PLUGIN_FACTORY, NULL));
}

void
modest_plugin_factory_load_all (ModestPluginFactory *self)
{
	ModestPluginFactoryPrivate *priv;
	GError *error = NULL;
	GDir *d;
	const gchar *dirent;
	ModestProtocolRegistry *pr;

	pr = modest_plugin_get_protocol_registry ();

	priv = MODEST_PLUGIN_FACTORY_GET_PRIVATE(self);

	if (!g_file_test (MODEST_MAILPLUGINDIR, G_FILE_TEST_IS_DIR)) {
		return;
	}

	d = g_dir_open (MODEST_MAILPLUGINDIR, 0, &error);

	if (!d)	{
		g_warning ("%s", error->message);
		g_error_free (error);

		return;
	}

	while ((dirent = g_dir_read_name (d))) {
		if (g_str_has_suffix (dirent, PLUGIN_EXT)) {
			gchar *plugin_file;
			ModestPlugin *plugin = NULL;

			plugin_file = g_build_filename (MODEST_MAILPLUGINDIR, dirent, NULL);
			plugin = modest_plugin_factory_load (plugin_file);
			g_free (plugin_file);

			if (plugin)
				priv->plugins = g_slist_prepend (priv->plugins, plugin);
		}
	}
	priv->plugins = g_slist_reverse (priv->plugins);

	g_dir_close (d);
}

static ModestPlugin*
modest_plugin_factory_load (const gchar *file)
{
	ModestPlugin *plugin = NULL;
	GKeyFile *plugin_file = NULL;
	gchar *plugin_name, *dir, *path;
	GTypeModule *type_module;

	g_return_val_if_fail (file != NULL, NULL);

	plugin_file = g_key_file_new ();

	if (!g_key_file_load_from_file (plugin_file, file, G_KEY_FILE_NONE, NULL)) {
		g_warning ("Bad plugin file: %s", file);
		goto error;
	}

	/* Get Location */
	plugin_name = g_key_file_get_string (plugin_file,
					     "Modest Mail Plugin",
					     "Module",
					     NULL);

	if ((plugin_name == NULL) || (*plugin_name == '\0')) {
		g_warning ("Could not find 'Module' in %s", file);
		goto error;
	}

	g_key_file_free (plugin_file);

	/* Build path to plugin */
	dir = g_path_get_dirname (file);
	path = g_module_build_path (dir, plugin_name);
	g_free (dir);

	/* plugin = g_module_open (path, G_MODULE_BIND_LAZY); */
	type_module = G_TYPE_MODULE (modest_module_new (path));
	if (type_module) {
		g_type_module_use (type_module);
		plugin = MODEST_PLUGIN (modest_module_new_object (MODEST_MODULE (type_module)));
		if (plugin)
			g_debug ("Plugin %s API version %s", plugin_name, modest_plugin_get_api_version (plugin));
		g_type_module_unuse (type_module);
	}
	g_free (path);
	g_free (plugin_name);

	return plugin;
error:
	g_key_file_free (plugin_file);

	return NULL;
}
