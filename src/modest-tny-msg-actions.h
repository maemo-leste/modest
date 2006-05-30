/* modest-tny-msg-actions.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_MSG_ACTIONS_H__
#define __MODEST_TNY_MSG_ACTIONS_H__

/* public */

/**
 * modest_tny_msg_actions_quote:
 * @self: the message to quote
 * 
 * Returns: a string containing the quoted message
 */

gchar *modest_tny_msg_actions_quote (const TnyMsgIface * self,
				     const gchar * from,
				     time_t sent_date,
				     gint limit, char *to_quote);

TnyMsgMimePartIface *modest_tny_msg_actions_find_body_part (TnyMsgIface * self,
							    const gchar * mime_type);

#endif /* __MODEST_TNY_MSG_ACTIONS_H__ */
