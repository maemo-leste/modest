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

#include <modest-plugin.h>
#include <modest-runtime.h>

G_DEFINE_TYPE (ModestPlugin, modest_plugin, G_TYPE_OBJECT)

static void   modest_plugin_class_init (ModestPluginClass *klass);
static void   modest_plugin_finalize   (GObject *obj);

#define MODEST_PLUGIN_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
								       MODEST_TYPE_PLUGIN, \
								       ModestPluginPrivate))

static GObjectClass *parent_class = NULL;

/* globals */

static void
modest_plugin_class_init (ModestPluginClass *klass)
{
	GObjectClass *object_class;
	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = modest_plugin_finalize;
}

static void
modest_plugin_init (ModestPlugin *obj)
{
	
}

static void   
modest_plugin_finalize   (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

ModestPlugin*
modest_plugin_new (void)
{
	ModestPlugin *plugin;

	plugin = g_object_new (MODEST_TYPE_PLUGIN, NULL);

	g_type_module_use (G_TYPE_MODULE (plugin));
	g_type_module_unuse (G_TYPE_MODULE (plugin));

	return plugin;
}

ModestAccountMgr *
modest_plugin_get_account_mgr (void)
{
	/* This is for avoiding including modest runtime itself */
	return modest_runtime_get_account_mgr ();
}

ModestProtocolRegistry *
modest_plugin_get_protocol_registry (void)
{
	/* This is for avoiding including modest runtime itself */
	return modest_runtime_get_protocol_registry ();
}

ModestTnyAccountStore *
modest_plugin_get_account_store (void)
{
	/* This is for avoiding including modest runtime itself */
	return modest_runtime_get_account_store ();
}

ModestMailOperationQueue *
modest_plugin_get_mail_operation_queue (void)
{
	/* This is for avoiding including modest runtime itself */
	return modest_runtime_get_mail_operation_queue ();
}

const gchar *
modest_plugin_get_api_version (ModestPlugin *plugin)
{
	ModestPluginClass *plugin_class;

	plugin_class = MODEST_PLUGIN_GET_CLASS (plugin);
	if (plugin_class->get_version)
		return plugin_class->get_version ();
	else
		return NULL;
}
