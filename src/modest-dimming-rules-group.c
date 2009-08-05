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
#include "modest-platform.h"
#include "modest-ui-dimming-rules.h"

static void modest_dimming_rules_group_class_init (ModestDimmingRulesGroupClass *klass);
static void modest_dimming_rules_group_init       (ModestDimmingRulesGroup *obj);
static void modest_dimming_rules_group_finalize   (GObject *obj);
static void modest_dimming_rules_group_dispose    (GObject *obj);

#ifndef MODEST_TOOLKIT_GTK
static void _insensitive_press_callback (GtkWidget *widget, gpointer user_data);
#endif

static void on_window_destroy (gpointer data,
			       GObject *object);

static void _add_rule (ModestDimmingRulesGroup *self,
		       ModestDimmingRule *rule,
		       ModestWindow *window);

typedef struct _ModestDimmingRulesGroupPrivate ModestDimmingRulesGroupPrivate;
struct _ModestDimmingRulesGroupPrivate {	
	ModestWindow *window;
	gchar *name;
	gboolean notifications_enabled;
	GHashTable *rules_map;
	GSList *widget_rules;
};


#define MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_DIMMING_RULES_GROUP, \
                                                   ModestDimmingRulesGroupPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

static void _execute_dimming_rule (gpointer key, gpointer value, gpointer user_data);
static void _execute_widget_dimming_rule (gpointer data, gpointer user_data);

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
	gobject_class->dispose  = modest_dimming_rules_group_dispose;

	g_type_class_add_private (gobject_class, sizeof(ModestDimmingRulesGroupPrivate));
}

static void
modest_dimming_rules_group_init (ModestDimmingRulesGroup *obj)
{
	ModestDimmingRulesGroupPrivate *priv;

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(obj);

	priv->name = NULL;
	priv->window = NULL;
	priv->notifications_enabled = FALSE;
	priv->rules_map = g_hash_table_new_full ((GHashFunc) g_str_hash,
						 (GEqualFunc) g_str_equal,
						 (GDestroyNotify) g_free,
						 (GDestroyNotify) g_object_unref);
	priv->widget_rules = NULL;
}

static void
modest_dimming_rules_group_finalize (GObject *obj)
{
	ModestDimmingRulesGroupPrivate *priv;

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(obj);

	if (priv->window)
		g_object_weak_unref (G_OBJECT (priv->window), on_window_destroy, obj);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_dimming_rules_group_dispose (GObject *obj)
{
	ModestDimmingRulesGroupPrivate *priv;

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(obj);

	if (priv->rules_map != NULL) {
		g_hash_table_destroy (priv->rules_map);
		priv->rules_map = NULL;
	}

	if (priv->widget_rules != NULL) {
		g_slist_foreach (priv->widget_rules, (GFunc) g_object_unref, NULL);
		priv->widget_rules = NULL;
	}

	G_OBJECT_CLASS(parent_class)->dispose (obj);
}


ModestDimmingRulesGroup*
modest_dimming_rules_group_new(const gchar *group_name,
			       gboolean notifications_enabled)
{
	ModestDimmingRulesGroup *obj;
	ModestDimmingRulesGroupPrivate *priv;
		
	g_return_val_if_fail (group_name != NULL, NULL);

	obj = MODEST_DIMMING_RULES_GROUP(g_object_new(MODEST_TYPE_DIMMING_RULES_GROUP, NULL));

	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(obj);
	priv->name = g_strdup(group_name);
	priv->notifications_enabled = notifications_enabled;

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

static void
_add_rule (ModestDimmingRulesGroup *self,
	   ModestDimmingRule *rule,
	   ModestWindow *window)
{
	ModestDimmingRulesGroupPrivate *priv;
	GtkWidget *widget = NULL;
	const gchar *action_path = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULES_GROUP(self));
	g_return_if_fail (MODEST_IS_WINDOW(window));
	g_return_if_fail (MODEST_IS_DIMMING_RULE (rule));
	
	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);

	/* Set window to process dimming rules */
	priv->window = MODEST_WINDOW (window);

	widget = modest_dimming_rule_get_widget (rule);
#ifndef MODEST_TOOLKIT_GTK
	/* Connect insensitive-presss handler to show notifications */
	g_signal_connect (G_OBJECT (widget), "insensitive-press", 
			  G_CALLBACK (_insensitive_press_callback), 
			  rule);
#endif
	/* Register new dimming rule */		
	modest_dimming_rule_set_group (rule, self);
	action_path = modest_dimming_rule_get_action_path (rule);
	if (action_path)
		g_hash_table_insert (priv->rules_map, g_strdup(action_path), rule);
	else
		priv->widget_rules = g_slist_prepend (priv->widget_rules, rule);
}

void
modest_dimming_rules_group_add_widget_rule (ModestDimmingRulesGroup *self,
					    GtkWidget *widget,
					    GCallback callback,
					    ModestWindow *window)
{
	ModestDimmingRulesGroupPrivate *priv;
	ModestDimmingRule *dim_rule = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULES_GROUP(self));
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (MODEST_IS_WINDOW(window));
	
	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);

	/* Set window to process dimming rules */
	priv->window = MODEST_WINDOW (window);

	dim_rule = modest_dimming_rule_new_from_widget (priv->window,
							(ModestDimmingCallback) callback,
							widget);

	_add_rule (self, dim_rule, window);
}

void
modest_dimming_rules_group_add_rules (ModestDimmingRulesGroup *self,
				      const ModestDimmingEntry modest_dimming_entries[],
				      guint n_elements,
				      ModestWindow *window)
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
	g_return_if_fail (MODEST_IS_WINDOW(window));
	
	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);

	/* Set window to process dimming rules */
	priv->window = MODEST_WINDOW (window);
	g_object_weak_ref (G_OBJECT (window), on_window_destroy, self);

	/* Add dimming rules */
	for (i=0; i < n_elements; i++) {
		entry = modest_dimming_entries[i]; 
		
		/* Check dimming_rule and action path are NOT NULL */
		if (entry.action_path == NULL) continue;
		if (entry.callback == NULL) continue;
		
		/* Check action path is unique */
		unique = g_hash_table_lookup (priv->rules_map, entry.action_path) == NULL;
		if (!unique) continue;

		/* Check action path is valid */
		widget = modest_window_get_action_widget (MODEST_WINDOW (window), entry.action_path);
		if (widget == NULL) continue;

		/* Create a new dimming rule */
		dim_rule = modest_dimming_rule_new (priv->window,
						    (ModestDimmingCallback) entry.callback,
						    entry.action_path);

		_add_rule (self, dim_rule, window);
	}
}

gboolean 
modest_dimming_rules_group_notifications_enabled (ModestDimmingRulesGroup *self)
{
	ModestDimmingRulesGroupPrivate *priv;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULES_GROUP(self), FALSE);
	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);
	
	return priv->notifications_enabled;
} 

void 
modest_dimming_rules_group_execute (ModestDimmingRulesGroup *self) 
{
	ModestDimmingRulesGroupPrivate *priv;
	DimmedState *state = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULES_GROUP(self));
	priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE(self);

	if (!priv->window)
		return;

	/* Init dimming rules init data */
	state = modest_ui_dimming_rules_define_dimming_state (priv->window);	
	modest_window_set_dimming_state (priv->window, state);

	/* execute group dimming rules */
	g_hash_table_foreach (priv->rules_map, _execute_dimming_rule, NULL);
	g_slist_foreach (priv->widget_rules, (GFunc) _execute_widget_dimming_rule, NULL);

	/* Free dimming ruls init data */
	modest_window_set_dimming_state (priv->window, NULL);
}


static void
_execute_dimming_rule (gpointer key, gpointer value, gpointer user_data)
{
	g_return_if_fail (MODEST_IS_DIMMING_RULE (value));

	/* Process dimming rule */
	modest_dimming_rule_process (MODEST_DIMMING_RULE(value));
}

static void
_execute_widget_dimming_rule (gpointer data, gpointer user_data)
{
	g_return_if_fail (MODEST_IS_DIMMING_RULE (data));

	/* Process dimming rule */
	modest_dimming_rule_process (MODEST_DIMMING_RULE(data));
}

#ifndef MODEST_TOOLKIT_GTK
static void
_insensitive_press_callback (GtkWidget *widget, gpointer user_data)
{
	ModestDimmingRulesGroup *group = NULL;
	ModestDimmingRule *rule = NULL;
	gchar *notification = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULE (user_data));
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check if this group has notification system enabled */
	group = modest_dimming_rule_get_group (rule);
	if (!modest_dimming_rules_group_notifications_enabled (group))
		goto frees;

	/* Get specific notification */
	notification = modest_dimming_rule_get_notification (rule);
	if (notification == NULL)
		goto frees;

	/* Notifications for dimmed items are not shown in Hildon2 */
#ifndef MODEST_TOOLKIT_HILDON2
	/* Show notification banner */
	modest_platform_information_banner (NULL, NULL, notification);
#endif

	/* Free */
 frees:
	if (group != NULL)
		g_object_unref(group);
	if (notification != NULL)
		g_free(notification);
}
#endif

static void
on_window_destroy (gpointer data,
		   GObject *object)
{
	ModestDimmingRulesGroup *self = MODEST_DIMMING_RULES_GROUP (data);
	ModestDimmingRulesGroupPrivate *priv = MODEST_DIMMING_RULES_GROUP_GET_PRIVATE (self);

	priv->window = NULL;
}
