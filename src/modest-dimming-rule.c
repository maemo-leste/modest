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

#include "modest-dimming-rule.h"
#include "modest-ui-dimming-manager.h"

static void modest_dimming_rule_class_init (ModestDimmingRuleClass *klass);
static void modest_dimming_rule_init       (ModestDimmingRule *obj);
static void modest_dimming_rule_finalize   (GObject *obj);
static void modest_dimming_rule_dispose    (GObject *obj);

typedef struct _ModestDimmingRulePrivate ModestDimmingRulePrivate;
struct _ModestDimmingRulePrivate {
	ModestDimmingRulesGroup *group;
	ModestWindow *win;
	ModestDimmingCallback dimming_rule;
	gchar *action_path;
	GtkWidget *widget;
	gchar *notification;
};

#define MODEST_DIMMING_RULE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_DIMMING_RULE, \
                                                   ModestDimmingRulePrivate))

/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_dimming_rule_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestDimmingRuleClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_dimming_rule_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestDimmingRule),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_dimming_rule_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestDimmingRule",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_dimming_rule_class_init (ModestDimmingRuleClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_dimming_rule_finalize;
	gobject_class->dispose  = modest_dimming_rule_dispose;

	g_type_class_add_private (gobject_class, sizeof(ModestDimmingRulePrivate));
}

static void
modest_dimming_rule_init (ModestDimmingRule *obj)
{
	ModestDimmingRulePrivate *priv;

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(obj);
	priv->group= NULL;
	priv->win = NULL;
	priv->dimming_rule = NULL;
	priv->action_path = NULL;
	priv->widget = NULL;
	priv->notification = NULL;
}

static void
modest_dimming_rule_finalize (GObject *obj)
{
	ModestDimmingRulePrivate *priv;

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(obj);

	if (priv->group != NULL)
		g_object_unref(priv->group);
	if (priv->action_path != NULL)
		g_free(priv->action_path);
	if (priv->notification != NULL)
		g_free(priv->notification);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_dimming_rule_dispose (GObject *obj)
{
	ModestDimmingRulePrivate *priv;

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(obj);

	if (priv->group != NULL) {
		g_object_unref (priv->group);
		priv->group = NULL;
	}

	G_OBJECT_CLASS(parent_class)->dispose (obj);
}


ModestDimmingRule*
modest_dimming_rule_new(ModestWindow *win,
			ModestDimmingCallback dimming_rule,
			const gchar *action_path)
{
	ModestDimmingRule *obj;
	ModestDimmingRulePrivate *priv;
		
	g_return_val_if_fail (MODEST_IS_WINDOW (win), NULL);
	g_return_val_if_fail (dimming_rule != NULL, NULL);
	g_return_val_if_fail (action_path != NULL, NULL);
			      
	obj = MODEST_DIMMING_RULE(g_object_new(MODEST_TYPE_DIMMING_RULE, NULL));

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(obj);
	priv->win = win;
	priv->dimming_rule = dimming_rule;
	priv->action_path = g_strdup(action_path);

	return obj;
}

ModestDimmingRule*
modest_dimming_rule_new_from_widget (ModestWindow *win,
				     ModestDimmingCallback dimming_rule,
				     GtkWidget *widget)
{
	ModestDimmingRule *obj;
	ModestDimmingRulePrivate *priv;
		
	g_return_val_if_fail (MODEST_IS_WINDOW (win), NULL);
	g_return_val_if_fail (dimming_rule != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
			      
	obj = MODEST_DIMMING_RULE(g_object_new(MODEST_TYPE_DIMMING_RULE, NULL));

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(obj);
	priv->win = win;
	priv->dimming_rule = dimming_rule;
	priv->widget = widget;

	return obj;
}


void
modest_dimming_rule_process (ModestDimmingRule *self)
{
	
	ModestDimmingRulePrivate *priv = NULL;
	GtkAction *action = NULL;
	gboolean dimmed = FALSE;

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(self);
	g_return_if_fail (priv->win != NULL);
	g_return_if_fail (priv->dimming_rule != NULL);
	g_return_if_fail ((priv->action_path != NULL)|| GTK_IS_WIDGET (priv->widget));

	/* process dimming rule */
	dimmed = priv->dimming_rule (priv->win, self);

	/* Update dimming status */
	if (priv->action_path != NULL) {
		action = modest_window_get_action (priv->win, priv->action_path);	
		if (action != NULL)
			gtk_action_set_sensitive (action, !dimmed);
		else
			g_printerr ("modest: action path '%s' has not associatd action\n", priv->action_path);
	} else if (priv->widget != NULL) {
		ModestUIDimmingMode mode;

		mode = modest_ui_dimming_manager_get_widget_dimming_mode (priv->widget);
		switch (mode) {
		case MODEST_UI_DIMMING_MODE_HIDE:
			if (dimmed) {
				gtk_widget_hide (GTK_WIDGET (priv->widget));
			} else {
				gtk_widget_show (GTK_WIDGET (priv->widget));
			}
			break;
		case MODEST_UI_DIMMING_MODE_DIM:
		default:
			gtk_widget_set_sensitive (priv->widget, !dimmed);
			break;
		}
	}
}

void
modest_dimming_rule_set_group (ModestDimmingRule *rule,
			       ModestDimmingRulesGroup *group)
{
	ModestDimmingRulePrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULE (rule));
	g_return_if_fail (MODEST_IS_DIMMING_RULES_GROUP (group));
	priv = MODEST_DIMMING_RULE_GET_PRIVATE(rule);

	if (priv->group == group) 
		return;
	if (priv->group != NULL) 
		g_object_unref (priv->group);
	priv->group = g_object_ref (group);			
}	

ModestDimmingRulesGroup *
modest_dimming_rule_get_group (ModestDimmingRule *rule)
{
	ModestDimmingRulePrivate *priv = NULL;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (rule), NULL);
	priv = MODEST_DIMMING_RULE_GET_PRIVATE(rule);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULES_GROUP (priv->group), NULL);
	
	return g_object_ref(priv->group);
}

void
modest_dimming_rule_set_notification (ModestDimmingRule *rule,
				      const gchar *notification)
{
	ModestDimmingRulePrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULE (rule));
	priv = MODEST_DIMMING_RULE_GET_PRIVATE(rule);
	
	/* Free previous notification */
	if (priv->notification != NULL) {
		g_free(priv->notification);
		priv->notification = NULL;
	}

	/* Set new notification message */
	if (notification != NULL)
		priv->notification = g_strdup(notification);
}

gchar *
modest_dimming_rule_get_notification (ModestDimmingRule *rule)
{
	ModestDimmingRulePrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (rule), NULL);
	priv = MODEST_DIMMING_RULE_GET_PRIVATE(rule);
	
	return g_strdup(priv->notification);
}

GtkWidget *
modest_dimming_rule_get_widget (ModestDimmingRule *rule)
{
	ModestDimmingRulePrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (rule), NULL);
	priv = MODEST_DIMMING_RULE_GET_PRIVATE(rule);

	if (priv->action_path != NULL)
		return modest_window_get_action_widget (MODEST_WINDOW (priv->win), priv->action_path);
	else
		return priv->widget;
}

const gchar *
modest_dimming_rule_get_action_path (ModestDimmingRule *rule)
{
	ModestDimmingRulePrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (rule), NULL);
	priv = MODEST_DIMMING_RULE_GET_PRIVATE(rule);

	return priv->action_path;
}
