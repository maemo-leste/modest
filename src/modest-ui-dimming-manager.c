/* Copyright (c) 2006, Nokia Corporation
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

#include "modest-debug.h"
#include "modest-ui-dimming-manager.h"
#include "modest-dimming-rules-group-priv.h"

static void modest_ui_dimming_manager_class_init (ModestUIDimmingManagerClass *klass);
static void modest_ui_dimming_manager_init       (ModestUIDimmingManager *obj);
static void modest_ui_dimming_manager_finalize   (GObject *obj);

static void _process_all_rules (gpointer key, gpointer value, gpointer user_data);


typedef struct _ModestUIDimmingManagerPrivate ModestUIDimmingManagerPrivate;
struct _ModestUIDimmingManagerPrivate {
	GHashTable *groups_map;
	GHashTable *delayed_calls;
};

#define MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_UI_DIMMING_MANAGER, \
                                                   ModestUIDimmingManagerPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_ui_dimming_manager_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestUIDimmingManagerClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_ui_dimming_manager_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestUIDimmingManager),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_ui_dimming_manager_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestUIDimmingManager",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_ui_dimming_manager_class_init (ModestUIDimmingManagerClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_ui_dimming_manager_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestUIDimmingManagerPrivate));
}

static void
modest_ui_dimming_manager_init (ModestUIDimmingManager *obj)
{
	ModestUIDimmingManagerPrivate *priv;

	priv = MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(obj);

	priv->groups_map = g_hash_table_new_full ((GHashFunc) g_str_hash,
						  (GEqualFunc) g_str_equal,
						  (GDestroyNotify) g_free,
						  (GDestroyNotify) g_object_unref);
	priv->delayed_calls = g_hash_table_new_full (g_str_hash,
						     g_str_equal,
						     g_free,
						     NULL);
}

static void
modest_ui_dimming_manager_finalize (GObject *obj)
{
	ModestUIDimmingManagerPrivate *priv;

	priv = MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(obj);

	if (priv->groups_map != NULL)
		g_hash_table_unref (priv->groups_map);

	if (priv->delayed_calls != NULL)
		g_hash_table_unref (priv->delayed_calls);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestUIDimmingManager*
modest_ui_dimming_manager_new()
{
	ModestUIDimmingManager *obj;
		
	obj = MODEST_UI_DIMMING_MANAGER(g_object_new(MODEST_TYPE_UI_DIMMING_MANAGER, NULL));


	return obj;
}

void
modest_ui_dimming_manager_insert_rules_group (ModestUIDimmingManager *self,
					      ModestDimmingRulesGroup *group)
{
	ModestUIDimmingManagerPrivate *priv;
	gchar *group_name = NULL;
	gboolean unique = FALSE;
	
	priv = MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(self);
	
	/* Get group name */
	group_name = modest_dimming_rules_group_get_name (group);
	
	/* Check group name is unique */
	unique = g_hash_table_lookup (priv->groups_map, group_name) == NULL;
	if (!unique) {
		g_free(group_name);
		g_return_if_fail (unique);
	}
	
	
	/* Insert new dimming rules group */
	g_hash_table_insert (priv->groups_map, group_name, g_object_ref(group));
}

void
modest_ui_dimming_manager_process_dimming_rules (ModestUIDimmingManager *self)
{
	ModestUIDimmingManagerPrivate *priv;
	
	priv = MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(self);

	/* Peforms a full dimming tules checking */
	g_hash_table_foreach (priv->groups_map, _process_all_rules, NULL);
}

typedef struct
{
	ModestDimmingRulesGroup *group;
	ModestUIDimmingManager *manager;
	gchar *name;
} DelayedDimmingRules;

static gboolean
process_dimming_rules_delayed (gpointer data)
{
	DelayedDimmingRules *helper = (DelayedDimmingRules *) data;
	gpointer timeout_handler;
	ModestUIDimmingManagerPrivate *priv;

	/* We remove the timeout here because the execute action could
	   take too much time, and so this will be called again */
	priv = MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(helper->manager);
	timeout_handler = g_hash_table_lookup (priv->delayed_calls, helper->name);

	MODEST_DEBUG_BLOCK(g_print ("---------------------HIT %d\n", GPOINTER_TO_INT (timeout_handler)););

	if (GPOINTER_TO_INT (timeout_handler) > 0) {
		g_source_remove (GPOINTER_TO_INT (timeout_handler));
	}

	modest_dimming_rules_group_execute (helper->group);

	return FALSE;
}

static void
process_dimming_rules_delayed_destroyer (gpointer data)
{
	DelayedDimmingRules *helper = (DelayedDimmingRules *) data;
	ModestUIDimmingManagerPrivate *priv;

	priv = MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(helper->manager);
	g_hash_table_remove (priv->delayed_calls, helper->name);
	g_object_unref (helper->manager);
	g_free (helper->name);
	g_slice_free (DelayedDimmingRules, helper);
}

void
modest_ui_dimming_manager_process_dimming_rules_group (ModestUIDimmingManager *self,
						       const gchar *group_name)
{
	ModestDimmingRulesGroup *group = NULL;
	ModestUIDimmingManagerPrivate *priv;
	guint *handler, new_handler;
	DelayedDimmingRules *helper;
	
	g_return_if_fail (group_name != NULL);

	priv = MODEST_UI_DIMMING_MANAGER_GET_PRIVATE(self);

	/* Search group by name */
	group = MODEST_DIMMING_RULES_GROUP(g_hash_table_lookup (priv->groups_map, group_name));
	g_return_if_fail (group != NULL);

	/* If there was another pending dimming operation check then ignore this */
	handler = g_hash_table_lookup (priv->delayed_calls, group_name);
	if (!handler) {
		/* Create the helper and start the timeout */
		helper = g_slice_new (DelayedDimmingRules);
		helper->group = group;
		helper->manager = g_object_ref (self);
		helper->name = g_strdup (group_name);
		new_handler = g_timeout_add_full (G_PRIORITY_DEFAULT, 500, process_dimming_rules_delayed, 
						  helper, process_dimming_rules_delayed_destroyer);
		g_hash_table_insert (priv->delayed_calls, g_strdup (group_name), GINT_TO_POINTER (new_handler));
		MODEST_DEBUG_BLOCK(g_print ("---------------------Adding %d\n", new_handler););
	} else {
		MODEST_DEBUG_BLOCK(g_print ("---------------------Ignoring\n"););
	}
}


static void
_process_all_rules (gpointer key, gpointer value, gpointer user_data)
{
	g_return_if_fail (MODEST_IS_DIMMING_RULES_GROUP (value));
	
	modest_dimming_rules_group_execute (MODEST_DIMMING_RULES_GROUP (value));
}

