
#ifndef _MODEST_UI_MESSAGE_VIEWER_H
#define _MODEST_UI_MESSAGE_VIEWER_H

/*
 * callback used in main-window
 * called when the menu-item "Open" is activated
 */
void on_open_message_clicked (GtkWidget *widget, gpointer user_data);

/*
 * callback used in main-window
 * called when double clicking on a message header
 */
void on_message_activated (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

#endif /* _MODEST_UI_MESSAGE_VIEWER_H */
