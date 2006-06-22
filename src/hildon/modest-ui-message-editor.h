#ifndef _MODEST_UI_MESSAGE_EDITOR_H
#define _MODEST_UI_MESSAGE_EDITOR_H

typedef enum {
	QUOTED_SEND_REPLY,
	QUOTED_SEND_REPLY_ALL,
	QUOTED_SEND_FORWARD,
	QUOTED_SEND_FORWARD_ATTACHED
} quoted_send_type;

/**
 * quoted_send_msg:
 * @modest_ui: a ModestUI instance
 * @qstype: determines whether to REPLY, REPLY_ALL or FORWARD
 *
 * open a new editor window quoting the currently selected message
 * the quote type determines which parts are to be quoted
 */
void quoted_send_msg (ModestUI *modest_ui, quoted_send_type qstype);

/**
 * on_new_mail_clicked:
 * @widget: the button widget that received the signal
 * @user_data: pointer to user-data, here ModestUI
 *
 * callback used in main-window
 * called when user presses the "New Mail" button
 */
void on_new_mail_clicked (GtkWidget *widget, gpointer user_data);

void ui_on_mailto_clicked (GtkWidget *widget, const gchar * uri, gpointer user_data);
#endif /* _MODEST_UI_MESSAGE_EDITOR_H */
