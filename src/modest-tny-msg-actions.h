/* modest-tny-msg-actions.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_MSG_ACTIONS_H__
#define __MODEST_TNY_MSG_ACTIONS_H__

/* public */

/**
 * modest_tny_msg_actions_quote:
 * @self: the message to quote
 * @from: the original sender of the message
 * @sent_date: the date the original message was sent
 * @limit: characters per line limit for the quoted message
 * @to_quote: a string to quote instead of the message body
 * 
 * reply-quotes a message or @to_quote if it's not NULL.

 * Note: @from and @sent_date may be eliminated from the API in future versions
 * 
 * Returns: a newly allocated string containing the quoted message
 */

gchar *modest_tny_msg_actions_quote (const TnyMsgIface * self,
				     const gchar * from,
				     time_t sent_date,
				     gint limit, const gchar *to_quote);

/**
 * modest_tny_msg_actions_find_body_part:
 * @self: a message
 * @want_html: prefer HTML-part when there are multiple body parts?
 * 
 * search a message for a body part 
 * 
 * Returns: the TnyMsgMimePartIface for the found part, or NULL if no matching part is found
 */
					 
TnyMsgMimePartIface *modest_tny_msg_actions_find_body_part (TnyMsgIface * self,
							    gboolean want_html);

#endif /* __MODEST_TNY_MSG_ACTIONS_H__ */
