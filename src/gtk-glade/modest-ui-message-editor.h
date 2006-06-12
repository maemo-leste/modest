#ifndef _MODEST_UI_MESSAGE_EDITOR_H
#define _MODEST_UI_MESSAGE_EDITOR_H

typedef enum {
	QUOTED_SEND_REPLY,
	QUOTED_SEND_REPLY_ALL,
	QUOTED_SEND_FORWARD
} quoted_send_type;


void quoted_send_msg (ModestUI *modest_ui, quoted_send_type qstype);

void on_new_mail_clicked (GtkWidget *widget, ModestUI *modest_ui);

#endif /* _MODEST_UI_MESSAGE_EDITOR_H */
