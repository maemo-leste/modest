/* modest-gnome-bar.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_GNOME_INFO_BAR_H__
#define __MODEST_GNOME_INFO_BAR_H__

#include <gtk/gtkhbox.h>
#include "modest-progress-object.h"
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_GNOME_INFO_BAR             (modest_gnome_info_bar_get_type())
#define MODEST_GNOME_INFO_BAR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_GNOME_INFO_BAR,ModestGnomeInfoBar))
#define MODEST_GNOME_INFO_BAR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_GNOME_INFO_BAR,ModestProgressObject))
#define MODEST_IS_GNOME_INFO_BAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_GNOME_INFO_BAR))
#define MODEST_IS_GNOME_INFO_BAR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_GNOME_INFO_BAR))
#define MODEST_GNOME_INFO_BAR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_GNOME_INFO_BAR,ModestGnomeInfoBarClass))

typedef struct _ModestGnomeInfoBar      ModestGnomeInfoBar;
typedef struct _ModestGnomeInfoBarClass ModestGnomeInfoBarClass;

struct _ModestGnomeInfoBar {
	 GtkHBox parent;
};

struct _ModestGnomeInfoBarClass {
	GtkHBoxClass parent_class;
};

/* member functions */
GType         modest_gnome_info_bar_get_type       (void) G_GNUC_CONST;

/* typical parameter-less _new function */
GtkWidget*    modest_gnome_info_bar_new            (void);

/**
 * modest_gnome_info_bar_new:
 * @void: 
 * 
 * Sets a text in the status bar of the widget
 * 
 * Return value: 
 **/
void          modest_gnome_info_bar_set_message    (ModestGnomeInfoBar *self,
						    const gchar *message);


/**
 * modest_gnome_info_bar_set_progress:
 * @self: 
 * @message: 
 * @done: 
 * @total: 
 * 
 * Causes the progress bar of the widget to fill in the amount of work
 * done of a given total. If message is supplied then it'll be
 * superimposed on the progress bar
 **/
void          modest_gnome_info_bar_set_progress   (ModestGnomeInfoBar *self,
						    const gchar *message,
						    gint done,
						    gint total);

G_END_DECLS

#endif /* __MODEST_GNOME_INFO_BAR_H__ */

