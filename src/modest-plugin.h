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
#ifndef __MODEST_PLUGIN_H__
#define __MODEST_PLUGIN_H__

#include <glib-object.h>
#include <glib/gi18n.h>
#include <modest-account-mgr.h>
#include <modest-protocol-registry.h>
#include <modest-tny-account-store.h>
#include <modest-mail-operation-queue.h>

#define MODEST_API_VERSION_STR2_HELPER(x) #x
#define MODEST_API_VERSION_STR_HELPER(x) MODEST_API_VERSION_STR2_HELPER(x)
#define MODEST_API_VERSION_STR MODEST_API_VERSION_STR_HELPER(MODEST_API_VERSION)

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_PLUGIN             (modest_plugin_get_type())
#define MODEST_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_PLUGIN,ModestPlugin))
#define MODEST_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_PLUGIN,ModestPluginClass))
#define MODEST_IS_PLUGIN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_PLUGIN))
#define MODEST_IS_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_PLUGIN))
#define MODEST_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_PLUGIN,ModestPluginClass))

typedef struct _ModestPlugin      ModestPlugin;
typedef struct _ModestPluginClass ModestPluginClass;

struct _ModestPlugin {
	GObject parent;
};

struct _ModestPluginClass {
	GObjectClass parent_class;
	const gchar * (*get_version) (void);
};

/**
 * modest_plugin_get_type:
 *
 * Returns: GType of the account store
 */
GType  modest_plugin_get_type   (void) G_GNUC_CONST;

#define MODEST_PLUGIN_REGISTER_TYPE(PluginName, plugin_name)		        \
										\
static GType plugin_name##_type = 0;						\
										\
GType										\
plugin_name##_get_type (void)							\
{										\
	return plugin_name##_type;						\
}										\
										\
static void     plugin_name##_init              (PluginName        *self);	\
static void     plugin_name##_class_init        (PluginName##Class *klass);	\
static gpointer plugin_name##_parent_class = NULL;				\
static const gchar *plugin_name##_internal_get_version (void)      		\
{                                                                               \
        return MODEST_API_VERSION_STR;					        \
}                                                                               \
static void     plugin_name##_class_intern_init (gpointer klass)		\
{										\
	plugin_name##_parent_class = g_type_class_peek_parent (klass);		\
	plugin_name##_class_init ((PluginName##Class *) klass);			\
	((ModestPluginClass *)klass)->get_version = plugin_name##_internal_get_version; \
}										\
										\
G_MODULE_EXPORT GType								\
register_modest_plugin (GTypeModule *module)					\
{										\
	static const GTypeInfo our_info =					\
	{									\
		sizeof (PluginName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) plugin_name##_class_intern_init,		\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (PluginName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) plugin_name##_init				\
	};									\
										\
										\
	plugin_name##_type = g_type_module_register_type (module,		\
					    MODEST_TYPE_PLUGIN,			\
					    #PluginName,			\
					    &our_info,				\
					    0);					\
										\
	return plugin_name##_type;						\
}

/* Global methods providing access to singletons without using modest runtime */
ModestAccountMgr *modest_plugin_get_account_mgr (void);
ModestProtocolRegistry *modest_plugin_get_protocol_registry (void);
ModestTnyAccountStore *modest_plugin_get_account_store (void);
ModestMailOperationQueue *modest_plugin_get_mail_operation_queue (void);
const gchar *modest_plugin_get_api_version (ModestPlugin *plugin);
G_END_DECLS

#endif /* __MODEST_PLUGIN_H__ */
