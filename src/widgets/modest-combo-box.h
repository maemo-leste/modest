/* modest-combo-box.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_COMBO_BOX_H__
#define __MODEST_COMBO_BOX_H__

#include <gtk/gtk.h>
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_COMBO_BOX             (modest_combo_box_get_type())
#define MODEST_COMBO_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_COMBO_BOX,ModestComboBox))
#define MODEST_COMBO_BOX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_COMBO_BOX,GtkComboBox))
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


struct _ModestComboBoxLemma {
	const gchar *display_name;
	gpointer id;
};
typedef struct _ModestComboBoxLemma ModestComboBoxLemma;

/**
 * modest_combo_box_get_type
 *
 * Returns: the id of the ModestComboBox type
 */
GType        modest_combo_box_get_type    (void) G_GNUC_CONST;

/**
 * modest_combo_box_new
 * @lemmas: a ptr to a NULL terminated list of ModestComboBox lemmas,
 * each corresponding to a display_name, and the corresponding value
 * create a new modest combo box,
 *
 * create a new modest combo box
 * 
 * Returns: a new ModestComboBox instance, or NULL in case of failure
 */
GtkWidget*   modest_combo_box_new         (ModestComboBoxLemma *lemmas);


/**
 * modest_combo_box_get_active_id
 * @self: a valid ModestComboBox instance 
 * 
 * get the id for the currently active lemma, or NULL if there's nothing chosen
 * 
 * Returns: the id or NULL if there's nothing chosen.
 */
gpointer   modest_combo_box_get_active_id       (ModestComboBox *self);



G_END_DECLS

#endif /* __MODEST_COMBO_BOX_H__ */

