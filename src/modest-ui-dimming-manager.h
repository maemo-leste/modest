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


#ifndef __MODEST_UI_DIMMING_MANAGER_H__
#define __MODEST_UI_DIMMING_MANAGER_H__

#include <glib-object.h>
#include "modest-dimming-rules-group.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_UI_DIMMING_MANAGER             (modest_ui_dimming_manager_get_type())
#define MODEST_UI_DIMMING_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_UI_DIMMING_MANAGER,ModestUIDimmingManager))
#define MODEST_UI_DIMMING_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_UI_DIMMING_MANAGER,GObject))
#define MODEST_IS_UI_DIMMING_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_UI_DIMMING_MANAGER))
#define MODEST_IS_UI_DIMMING_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_UI_DIMMING_MANAGER))
#define MODEST_UI_DIMMING_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_UI_DIMMING_MANAGER,ModestUIDimmingManagerClass))

typedef struct _ModestUIDimmingManager      ModestUIDimmingManager;
typedef struct _ModestUIDimmingManagerClass ModestUIDimmingManagerClass;

struct _ModestUIDimmingManager {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestUIDimmingManagerClass {
	GObjectClass parent_class;
	
	/* Signals */
};

typedef enum {
	MODEST_UI_DIMMING_MODE_DIM = 0,
	MODEST_UI_DIMMING_MODE_HIDE,
} ModestUIDimmingMode;

ModestUIDimmingManager* modest_ui_dimming_manager_new(void);


void
modest_ui_dimming_manager_insert_rules_group (ModestUIDimmingManager *self,
						 ModestDimmingRulesGroup *group);

void
modest_ui_dimming_manager_process_dimming_rules (ModestUIDimmingManager *self);


void
modest_ui_dimming_manager_process_dimming_rules_group (ModestUIDimmingManager *self,
						       const gchar *group_name);

void
modest_ui_dimming_manager_set_widget_dimming_mode (GtkWidget *widget,
						   ModestUIDimmingMode mode);
						   
ModestUIDimmingMode
modest_ui_dimming_manager_get_widget_dimming_mode (GtkWidget *widget);
						   


G_END_DECLS

#endif 
