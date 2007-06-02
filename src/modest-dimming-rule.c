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

static void modest_dimming_rule_class_init (ModestDimmingRuleClass *klass);
static void modest_dimming_rule_init       (ModestDimmingRule *obj);
static void modest_dimming_rule_finalize   (GObject *obj);

typedef struct _ModestDimmingRulePrivate ModestDimmingRulePrivate;
struct _ModestDimmingRulePrivate {
	ModestWindow *win;
	ModestDimmingCallback dimming_rule;
	gchar *action_path;
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

	g_type_class_add_private (gobject_class, sizeof(ModestDimmingRulePrivate));
}

static void
modest_dimming_rule_init (ModestDimmingRule *obj)
{
	ModestDimmingRulePrivate *priv;

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(obj);
	priv->win = NULL;
	priv->dimming_rule = NULL;
	priv->action_path = NULL;
	priv->notification = NULL;
}

static void
modest_dimming_rule_finalize (GObject *obj)
{
	ModestDimmingRulePrivate *priv;

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(obj);

	if (priv->action_path != NULL)
		g_free(priv->action_path);
	if (priv->notification != NULL)
		g_free(priv->notification);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
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


void
modest_dimming_rule_process (ModestDimmingRule *self)
{
	
	ModestDimmingRulePrivate *priv = NULL;
	GtkAction *action = NULL;
	gboolean dimmed = FALSE;

	priv = MODEST_DIMMING_RULE_GET_PRIVATE(self);
	g_return_if_fail (priv->win != NULL);
	g_return_if_fail (priv->dimming_rule != NULL);
	g_return_if_fail (priv->action_path != NULL);

	/* process dimming rule */
	dimmed = priv->dimming_rule (priv->win, self);

	/* Update dimming status */
        action = modest_window_get_action (priv->win, priv->action_path);	
	g_return_if_fail (action != NULL);
	gtk_action_set_sensitive (action, !dimmed);
}

void
modest_dimming_rule_set_notification (ModestDimmingRule *rule,
				      const gchar *notification)
{
	ModestDimmingRulePrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_DIMMING_RULE (rule));
	priv = MODEST_DIMMING_RULE_GET_PRIVATE(rule);
	
	/* Free previous notification */
	if (priv->notification != NULL)
		g_free(priv->notification);

	/* Set new notification message */
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
