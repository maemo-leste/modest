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

#ifndef __MODEST_DIMMING_RULE_H__
#define __MODEST_DIMMING_RULE_H__

#include <glib-object.h>
#include "widgets/modest-window.h"
#include "modest-dimming-rules-group.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_DIMMING_RULE             (modest_dimming_rule_get_type())
#define MODEST_DIMMING_RULE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_DIMMING_RULE,ModestDimmingRule))
#define MODEST_DIMMING_RULE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_DIMMING_RULE,GObject))
#define MODEST_IS_DIMMING_RULE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_DIMMING_RULE))
#define MODEST_IS_DIMMING_RULE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_DIMMING_RULE))
#define MODEST_DIMMING_RULE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_DIMMING_RULE,ModestDimmingRuleClass))

typedef struct _ModestDimmingRule      ModestDimmingRule;
typedef struct _ModestDimmingRuleClass ModestDimmingRuleClass;


struct _ModestDimmingRule {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestDimmingRuleClass {
	GObjectClass parent_class;

	/* Signals */
};

/* member functions */
GType        modest_dimming_rule_get_type    (void) G_GNUC_CONST;

/**
 * modest_dimming_rule_new:
 * @win: the #ModestWindow object which executes dimming rule.
 * @dimming_rule: a #ModestDimmingCallback function to check dimmed status.
 * @action_path: full path of action registered on #GtkUIManager UIManager.
 * 
 * Creates a new instance of class #ModestDimmingRule using parameters to
 * fill private data, required to process dimming rules. All parameters
 * are required and NULL will be returned if one of these parameters is
 * invalid or NULL.
 * 
 * Returns: a new instance of #ModestDimmingRule class, or NULL, if parameters
 * are invalid.
 **/
ModestDimmingRule*    modest_dimming_rule_new     (ModestWindow *win,
						   ModestDimmingCallback dimming_rule,
						   const gchar *action_path);


/**
 * modest_dimming_rule_new:
 * @win: the #ModestWindow object which executes dimming rule.
 * @dimming_rule: a #ModestDimmingCallback function to check dimmed status.
 * @widget: the widget the rule will apply to
 * 
 * Creates a new instance of class #ModestDimmingRule using parameters to
 * fill private data, required to process dimming rules. All parameters
 * are required and NULL will be returned if one of these parameters is
 * invalid or NULL.
 * 
 * Returns: a new instance of #ModestDimmingRule class, or NULL, if parameters
 * are invalid.
 **/
ModestDimmingRule*
modest_dimming_rule_new_from_widget (ModestWindow *win,
				     ModestDimmingCallback dimming_rule,
				     GtkWidget *widget);
/**
 * modest_dimming_rule_process:
 * @rule: a #ModestDimmingRule object to process.
 * 
 * Process dimming rule, executing private callback defined at 
 * instantiation time. This callback may updates notification provate field
 * of @rule in order to show information banners when 'insensitive-press'
 * events occurs. 
 *
 **/
void modest_dimming_rule_process (ModestDimmingRule *self);

/**
 * modest_dimming_rule_set_group:
 * @rule: a #ModestDimmingRule object to process.
 * @group: a #ModestDimmingRulesGroup object to associate.
 * 
 * Creates a new reference of @group, associated to this "rule.
 */
void modest_dimming_rule_set_group (ModestDimmingRule *rule,
				    ModestDimmingRulesGroup *group);

/**
 * modest_dimming_rule_set_group:
 * @rule: a #ModestDimmingRule object to process.
 * 
 * Gets a new reference of associated group of this @rule.
 *
 * @Returns: a new object reference of #ModestDimmingRulesGroup, or 
 * NULL if invalid @rule.
 */
ModestDimmingRulesGroup *
modest_dimming_rule_get_group (ModestDimmingRule *rule);

/**
 * modest_dimming_rule_get_widget:
 * @rule: a #ModestDimmingRule
 *
 * Widget the dimming rule is referenced to
 *
 * Returns: a #GtkWidget or %NULL if the dimming rule has no widget attached
 */
const gchar*
modest_dimming_rule_get_action_path (ModestDimmingRule *rule);

/**
 * modest_dimming_rule_get_widget:
 * @rule: a #ModestDimmingRule
 *
 * In case the dimming rule references directly a widget, it
 * returns the widget.
 *
 * Returns: a #GtkWidget or %NULL if the dimming rule has no direct widget attached
 */
GtkWidget *
modest_dimming_rule_get_widget (ModestDimmingRule *rule);

void modest_dimming_rule_set_notification (ModestDimmingRule *rule,
					   const gchar *notification);

gchar *modest_dimming_rule_get_notification (ModestDimmingRule *rule);
					   

G_END_DECLS

#endif /* __MODEST_DIMMING_RULE_H__ */
