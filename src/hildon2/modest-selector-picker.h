/* Copyright (c) 2006, 2008 Nokia Corporation
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

#ifndef __MODEST_SELECTOR_PICKER_H__
#define __MODEST_SELECTOR_PICKER_H__

#include <hildon/hildon-picker-button.h>
#include <modest-pair.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_SELECTOR_PICKER             (modest_selector_picker_get_type())
#define MODEST_SELECTOR_PICKER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_SELECTOR_PICKER,ModestSelectorPicker))
#define MODEST_SELECTOR_PICKER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_SELECTOR_PICKER,GObject))
#define MODEST_IS_SELECTOR_PICKER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_SELECTOR_PICKER))
#define MODEST_IS_SELECTOR_PICKER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_SELECTOR_PICKER))
#define MODEST_SELECTOR_PICKER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_SELECTOR_PICKER,ModestSelectorPickerClass))

typedef struct _ModestSelectorPicker      ModestSelectorPicker;
typedef struct _ModestSelectorPickerClass ModestSelectorPickerClass;

struct _ModestSelectorPicker {
	 HildonPickerButton parent;
	/* insert public members, if any */
};

struct _ModestSelectorPickerClass {
	HildonPickerButtonClass parent_class;
};


/* member functions */
GType        modest_selector_picker_get_type    (void) G_GNUC_CONST;

/**
 * modest_selector_picker_new
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
 * create a new modest selector picker.
 * 
 * Returns: a new ModestSelectorPicker instance, or NULL in case of failure
 */
GtkWidget*   modest_selector_picker_new         (HildonSizeType size,
						 HildonButtonArrangement arrangement,
						 ModestPairList* pairs,
						 GEqualFunc id_equal_func,
						 gboolean center_aligned);

/**
 * modest_selector_picker_set_pair_list:
 * @combo: a #ModestSelectorPicker
 * @pairs: a #ModestPairList
 *
 * sets the model of the picker with a new pair list.
 */
void         modest_selector_picker_set_pair_list (ModestSelectorPicker *combo, 
						   ModestPairList *pairs);
/**
 * modest_selector_picker_get_active_id
 * @self: a valid ModestSelectorPicker instance 
 * 
 * get the id for the currently active item, or NULL if there's nothing chosen
 * (ie., the id is the first element of the pair)
 * 
 * Returns: the id or NULL if there's nothing chosen.
 */
gpointer   modest_selector_picker_get_active_id  (ModestSelectorPicker *self);

/**
 * modest_selector_picker_set_active_id
 * @self: a valid ModestSelectorPicker instance
 * @id: the id to make active
 * 
 * set the active item
 * 
 */
void       modest_selector_picker_set_active_id (ModestSelectorPicker *self, gpointer id);

/**
 * modest_selector_picker_get_active_display_name
 * @self: a valid ModestSelectorPicker instance 
 * 
 * get the display name for the currently active lemma, or NULL if
 * there's nothing chosen
 * 
 * Returns: the display name or NULL if there's nothing chosen.
 */
const gchar* modest_selector_picker_get_active_display_name  (ModestSelectorPicker *self);

/**
 * modest_selector_picker_set_value_max_chars:
 * @self: a #ModestSelectorPicker
 * @value_max_chars: maximum number of chars displayed in picker button, or -1 if not limited
 *
 * Set the maximum number of chars accepted in the value part of the selector picker
 */
void       modest_selector_picker_set_value_max_chars (ModestSelectorPicker *self, gint value_max_width_chars);

/**
 * modest_selector_picker_get_value_max_chars:
 * @self: a #ModestSelectorPicker
 *
 * Get the maximum number of chars accepted in the value part of the selector picker
 */
gint       modest_selector_picker_get_value_max_chars (ModestSelectorPicker *self);

G_END_DECLS

#endif /* __MODEST_SELECTOR_PICKER_H__ */

