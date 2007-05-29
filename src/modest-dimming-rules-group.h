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

#ifndef __MODEST_DIMMING_RULES_GROUP_H__
#define __MODEST_DIMMING_RULES_GROUP_H__

#include <glib-object.h>
#include <widgets/modest-window.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_DIMMING_RULES_GROUP             (modest_dimming_rules_group_get_type())
#define MODEST_DIMMING_RULES_GROUP(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_DIMMING_RULES_GROUP,ModestDimmingRulesGroup))
#define MODEST_DIMMING_RULES_GROUP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_DIMMING_RULES_GROUP,GObject))
#define MODEST_IS_DIMMING_RULES_GROUP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_DIMMING_RULES_GROUP))
#define MODEST_IS_DIMMING_RULES_GROUP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_DIMMING_RULES_GROUP))
#define MODEST_DIMMING_RULES_GROUP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_DIMMING_RULES_GROUP,ModestDimmingRulesGroupClass))

typedef struct {
	const gchar     *action_path;
	GCallback  callback;
} ModestDimmingEntry;

typedef struct _ModestDimmingRulesGroup      ModestDimmingRulesGroup;
typedef struct _ModestDimmingRulesGroupClass ModestDimmingRulesGroupClass;

struct _ModestDimmingRulesGroup {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestDimmingRulesGroupClass {
	GObjectClass parent_class;

	/* Signals */
};


/* member functions */
GType        modest_dimming_rules_group_get_type    (void) G_GNUC_CONST;

/**
 * modest_dimming_rules_group_new:
 * @group_name: the name to identify new created group 
 * 
 * Creates a new instance of class #ModestDimmingRulesGroup. The @group_name
 * parameter identifies uniquely new group created, so it must not be NULL.
 *
 * Returns: a new #ModestDimmingRulesGroup instance, or NULL if parameters 
 * are invalid.
 **/
ModestDimmingRulesGroup*    modest_dimming_rules_group_new     (const gchar *group_name);



/**
 * modest_dimming_rules_group_add_rules:
 * @self: the #ModestDimmingRulesGroup object which stores dimming rules.
 * @modest_dimming_entries: a #ModestDimmingEntry array to define dimmed status handlers.
 * @n_elements: the number of elements of @modest_dimming_entries array.
 * @user_data: generic user data.
 * 
 * Add rules to @self dimming rules group object. 
 **/
void
modest_dimming_rules_group_add_rules (ModestDimmingRulesGroup *self,
				      const ModestDimmingEntry modest_dimming_entries[],
				      guint n_elements,
				      gpointer user_data);

/**
 * modest_dimming_rules_group_get_name:
 * @self: the #ModestDimmingRulesGroup object which stores dimming rules.
 * 
 * Gets the name, which uniquely identifies @self dimming rules group.
 *
 * Returns: a string with group name. 
 **/
gchar *
modest_dimming_rules_group_get_name (ModestDimmingRulesGroup *self);

G_END_DECLS

#endif /* __MODEST_DIMMING_RULES_GROUP_H__ */
