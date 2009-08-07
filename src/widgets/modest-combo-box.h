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

#ifndef __MODEST_COMBO_BOX_H__
#define __MODEST_COMBO_BOX_H__

#include <gtk/gtk.h>
#include <modest-pair.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_COMBO_BOX             (modest_combo_box_get_type())
#define MODEST_COMBO_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_COMBO_BOX,ModestComboBox))
#define MODEST_COMBO_BOX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_COMBO_BOX,GObject))
#define MODEST_IS_COMBO_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_COMBO_BOX))
#define MODEST_IS_COMBO_BOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_COMBO_BOX))
#define MODEST_COMBO_BOX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_COMBO_BOX,ModestComboBoxClass))

typedef struct _ModestComboBox      ModestComboBox;
typedef struct _ModestComboBoxClass ModestComboBoxClass;

struct _ModestComboBox {
	 GtkComboBox parent;
	/* insert public members, if any */
};

struct _ModestComboBoxClass {
	GtkComboBoxClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestComboBox* obj); */
};


/* member functions */
GType        modest_combo_box_get_type    (void) G_GNUC_CONST;

/**
 * modest_combo_box_new
 * @pairs: a #ModestPairList; each element should be a ptr to a #ModestPair,
 * with the first item in the pair being an opaque ID, and the second item being 
 * a gchar* string for display. 
 * If the ID is a string or other non fundamental type, you must make sure that 
 * the instance remains allocated for the lifetime of this combo box.
 * The simplest way to achieve this is to ensure that the ModestPairList exists 
 * for as long as the combo box.
 * @cmp_id_func: a GEqualFunc to compare the ids (= the first elements of the pairs)
 * For example, if the ids are strings, you can use g_str_equal.
 *
 * (If you provide NULL for this parameter, the id ptr address will be compared)
 * 
 * create a new modest combo box
 * 
 * Returns: a new GtkComboBox instance, or NULL in case of failure
 */
GtkWidget*   modest_combo_box_new         (ModestPairList* pairs,
					   GEqualFunc id_equal_func);

/**
 * modest_combo_box_set_pair_list:
 * @combo: a #ModestComboBox
 * @pairs: a #ModestPairList
 *
 * sets the model of the combobox with a new pair list.
 */
void         modest_combo_box_set_pair_list (ModestComboBox *combo, 
					     ModestPairList *pairs);
/**
 * modest_combo_box_get_active_id
 * @self: a valid ModestComboBox instance 
 * 
 * get the id for the currently active item, or NULL if there's nothing chosen
 * (ie., the id is the first element of the pair)
 * 
 * Returns: the id or NULL if there's nothing chosen.
 */
gpointer   modest_combo_box_get_active_id  (ModestComboBox *self);

/**
 * modest_combo_box_set_active_id
 * @self: a valid ModestComboBox instance
 * @id: the id to make active
 * 
 * set the active item
 * 
 */
void       modest_combo_box_set_active_id (ModestComboBox *self, gpointer id);

/**
 * modest_combo_box_get_active_display_name
 * @self: a valid ModestComboBox instance 
 * 
 * get the display name for the currently active lemma, or NULL if
 * there's nothing chosen
 * 
 * Returns: the display name or NULL if there's nothing chosen.
 */
const gchar* modest_combo_box_get_active_display_name  (ModestComboBox *self);

G_END_DECLS

#endif /* __MODEST_COMBO_BOX_H__ */

