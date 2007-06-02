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

#include "modest-dimming-rules-group.h"
#include "modest-dimming-rules-group-priv.h"
#include "modest-dimming-rule.h"
#include "modest-hildon-includes.h"

static void modest_dimming_rules_group_class_init (ModestDimmingRulesGroupClass *klass);
static void modest_dimming_rules_group_init       (ModestDimmingRulesGroup *obj);
static void modest_dimming_rules_group_finalize   (GObject *obj);

static void _insensitive_press_callback (GtkWidget *widget, gpointer user_data);


typedef struct _ModestDimmingRulesGroupPrivate ModestDimmingRulesGroupPrivate;
struct _ModestDimmingRulesGroupPrivate {	
	gchar *name;
	GHashTable *rules_map;
};

#define MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_DIMMING_RULES_GROUP, \
                                                   ModestDimmingRulesGroupPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

static void _execute_dimming_rule (gpointer key, gpointer value, gpointer user_data);
static void _insensitive_press_callback (GtkWidget *widget, gpointer user_data);



GType
modest_dimming_rules_group_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestDimmingRulesGroupClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_dimming_rules_group_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestDimmingRulesGroup),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_dimming_rules_group_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestDimmingRulesGroup",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_dimming_rules_group_class_init (ModestDimmingRulesGroupClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_dimming_rules_group_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestDimmingRulesGroupPrivate));
}

static void
modest_dimming_rules_group_init (ModestDimmingRulesGroup *obj)
{
	ModestDimmingRulesGroupPrivate *priv;

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(obj);

	priv->name = NULL;
	priv->rules_map = g_hash_table_new_full ((GHashFunc) g_str_hash,
						 (GEqualFunc) g_str_equal,
						 (GDestroyNotify) g_free,
						 (GDestroyNotify) g_object_unref);
}

static void
modest_dimming_rules_group_finalize (GObject *obj)
{
	ModestDimmingRulesGroupPrivate *priv;

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(obj);

	if (priv->name != NULL)
		g_free(priv->name);

	if (priv->rules_map != NULL)
		g_hash_table_destroy (priv->rules_map);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestDimmingRulesGroup*
modest_dimming_rules_group_new(const gchar *group_name)
{
	ModestDimmingRulesGroup *obj;
	ModestDimmingRulesGroupPrivate *priv;
		
	g_return_val_if_fail (group_name != NULL, NULL);

	obj = MODEST_DIMMING_RULES_GROUP(g_object_new(MODEST_TYPE_DIMMING_RULES_GROUP, NULL));

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(obj);
	priv->name = g_strdup(group_name);

	return obj;
}


gchar *
modest_dimming_rules_group_get_name (ModestDimmingRulesGroup *self) 
{
	ModestDimmingRulesGroupPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULES_GROUP(self), NULL);

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);
	
	return g_strdup (priv->name);
}

void
modest_dimming_rules_group_add_rules (ModestDimmingRulesGroup *self,
				      const ModestDimmingEntry modest_dimming_entries[],
				      guint n_elements,
				      gpointer user_data)
{
	ModestDimmingRulesGroupPrivate *priv;
	ModestDimmingRule *dim_rule = NULL;
	ModestDimmingEntry entry;
	GtkWidget *widget = NULL;
	gboolean unique = FALSE;
	guint i;

	g_return_if_fail (MODEST_IS_DIMMING_RULES_GROUP(self));
	g_return_if_fail (modest_dimming_entries != NULL);
	g_return_if_fail (n_elements > 0);
	g_return_if_fail (MODEST_IS_WINDOW(user_data));
	
	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);

	for (i=0; i < n_elements; i++) {
		entry = modest_dimming_entries[i]; 
		
		/* Check dimming_rule and action path are NOT NULL */
		if (entry.action_path == NULL) continue;
		if (entry.callback == NULL) continue;
		
		/* Check action path is unique */
		unique = g_hash_table_lookup (priv->rules_map, entry.action_path) == NULL;
		if (!unique) continue;

		/* Check action path is valid */
		widget = modest_window_get_action_widget (MODEST_WINDOW (user_data), entry.action_path);
		if (widget == NULL) continue;

		/* Create a new dimming rule */
		dim_rule = modest_dimming_rule_new (MODEST_WINDOW(user_data),
						    (ModestDimmingCallback) entry.callback,
						    entry.action_path);
		
		/* Connect insensitive-presss handler to show notifications */
		g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (_insensitive_press_callback), dim_rule);

		/* Register new dimming rule */		
		g_hash_table_insert (priv->rules_map, g_strdup(entry.action_path), dim_rule);
	}
}

void 
modest_dimming_rules_group_execute (ModestDimmingRulesGroup *self) 
{
	ModestDimmingRulesGroupPrivate *priv;

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);

	g_hash_table_foreach (priv->rules_map, _execute_dimming_rule, NULL);
}


static void
_execute_dimming_rule (gpointer key, gpointer value, gpointer user_data)
{
	g_return_if_fail (MODEST_IS_DIMMING_RULE (value));

	/* Process diomming rule */
	modest_dimming_rule_process (MODEST_DIMMING_RULE(value));
}

static void
_insensitive_press_callback (GtkWidget *widget, gpointer user_data)
{
	ModestDimmingRule *rule;
	gchar *notification = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULE (user_data));
	rule = MODEST_DIMMING_RULE (user_data);

	/* Get specific notification */
	notification = modest_dimming_rule_get_notification (rule);
	if (notification == NULL) return;

	/* Show notification banner */
	hildon_banner_show_information (NULL, NULL, notification);	

	/* Free */
	g_free(notification);
}
