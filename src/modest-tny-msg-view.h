/* modest-tny-msg-view.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_MSG_VIEW_H__
#define __MODEST_TNY_MSG_VIEW_H__


#include <gtk/gtk.h>
#include <tny-stream-iface.h>
#include <tny-msg-iface.h>
#include <tny-msg-mime-part-iface.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_MSG_VIEW             (modest_tny_msg_view_get_type())
#define MODEST_TNY_MSG_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_MSG_VIEW,ModestTnyMsgView))
#define MODEST_TNY_MSG_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_MSG_VIEW,ModestTnyMsgViewClass))
#define MODEST_IS_TNY_MSG_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_MSG_VIEW))
#define MODEST_IS_TNY_MSG_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_MSG_VIEW))
#define MODEST_TNY_MSG_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_MSG_VIEW,ModestTnyMsgViewClass))

typedef struct _ModestTnyMsgView      ModestTnyMsgView;
typedef struct _ModestTnyMsgViewClass ModestTnyMsgViewClass;

struct _ModestTnyMsgView {
	GtkScrolledWindow parent;
};

struct _ModestTnyMsgViewClass {
	GtkScrolledWindowClass parent_class;
};


/**
 * modest_tny_msg_view_get_type
 *
 * get the GType for the this class
 *
 * Returns: the GType for this class
 */
GType        modest_tny_msg_view_get_type    (void) G_GNUC_CONST;


/**
 * modest_tny_msg_view_new 
 * @tny_msg: a TnyMsgIface instance, or NULL
 *
 * create a new ModestTnyMsgView widget (a GtkScrolledWindow subclass),
 * and display the @tny_msg e-mail message in it. If @tny_msg is NULL,
 * then a blank page will be displayed
 *  
 * Returns: a new ModestTnyMsgView widget, or NULL if there's an error
 */
GtkWidget*   modest_tny_msg_view_new          (TnyMsgIface *tny_msg);


/**
 * modest_tny_msg_view_set_message
 * @self: a ModestTnyMsgView instance
 * @tny_msg: a TnyMsgIface instance, or NULL
 *
 * display the @tny_msg e-mail message. If @tny_msg is NULL,
 * then a blank page will be displayed
 *  */
void         modest_tny_msg_view_set_message  (ModestTnyMsgView *self,
						TnyMsgIface *tny_msg);
						
GtkTextBuffer *      modest_tny_msg_view_get_selected_text (ModestTnyMsgView *self);

G_END_DECLS

#endif /* __MODEST_TNY_MSG_VIEW_H__ */
