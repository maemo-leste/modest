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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <gmodule.h>
#include "modest-module.h"

G_DEFINE_TYPE (ModestModule, modest_module, G_TYPE_TYPE_MODULE);


static gboolean modest_module_load (GTypeModule *gmodule);
static void modest_module_unload (GTypeModule *gmodule);
static void modest_module_init (ModestModule *module);
static void modest_module_finalize (GObject *object);
static void modest_module_class_init (ModestModuleClass *class);

typedef GType (*ModestModuleRegisterFunc) (GTypeModule *);

typedef struct _ModestModulePrivate ModestModulePrivate;
struct _ModestModulePrivate {
	GModule *g_module;
	gchar   *path;
	GType    type;
};

#define MODEST_MODULE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
								       MODEST_TYPE_MODULE, \
								       ModestModulePrivate))
  
static gboolean
modest_module_load (GTypeModule *g_type_module)
{
	ModestModule *module = MODEST_MODULE (g_type_module);
	ModestModulePrivate *priv = MODEST_MODULE_GET_PRIVATE (module);
	ModestModuleRegisterFunc register_func;

	/* We don't do lazy linking to fail just if we cannot link all the symbols available */
	priv->g_module = g_module_open (priv->path, 0);

	if (priv->g_module == NULL) {
		g_warning ("%s", g_module_error());
		return FALSE;
	}

	/* Get the register function */
	if (!g_module_symbol (priv->g_module, "register_modest_plugin", (void *) &register_func)) {
		g_warning ("%s", g_module_error());
		g_module_close (priv->g_module);

		return FALSE;
	}

	if (register_func == NULL) {
		g_warning ("register_modest_plugin shouldn't be NULL");
		g_module_close (priv->g_module);

		return FALSE;
	}

	/* call the register function to initialize the module */
	priv->type = register_func (g_type_module);
	if (priv->type == 0) {
		g_warning ("%s is not a modest plugin", priv->path);
		return FALSE;
	}

	return TRUE;
}

static void
modest_module_unload (GTypeModule *gmodule)
{
	ModestModule *module = MODEST_MODULE (gmodule);
	ModestModulePrivate *priv = MODEST_MODULE_GET_PRIVATE (module);

	g_module_close (priv->g_module);

	priv->g_module = NULL;
	priv->type = 0;
}

GObject *
modest_module_new_object (ModestModule *module)
{
	ModestModulePrivate *priv = MODEST_MODULE_GET_PRIVATE (module);

	if (priv->type == 0) {
		return NULL;
	}

	return g_object_new (priv->type, NULL);
}

static void
modest_module_init (ModestModule *module)
{
}

static void
modest_module_finalize (GObject *object)
{
	ModestModule *module = MODEST_MODULE (object);
	ModestModulePrivate *priv = MODEST_MODULE_GET_PRIVATE (module);

	g_free (priv->path);

	G_OBJECT_CLASS (modest_module_parent_class)->finalize (object);
}

static void
modest_module_class_init (ModestModuleClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

	gobject_class->finalize = modest_module_finalize;

	module_class->load = modest_module_load;
	module_class->unload = modest_module_unload;

	g_type_class_add_private (gobject_class, sizeof(ModestModulePrivate));
}

ModestModule *
modest_module_new (const gchar *path)
{
	ModestModule *module;
	ModestModulePrivate *priv;

	if (path == NULL || path[0] == '\0') {
		return NULL;
	}

	module = g_object_new (MODEST_TYPE_MODULE, NULL);
	priv = MODEST_MODULE_GET_PRIVATE (module);

	g_type_module_set_name (G_TYPE_MODULE (module), path);

	priv->path = g_strdup (path);

	return module;
}
