/* modest-text-utils.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TEXT_UTILS_H__
#define __MODEST_TEXT_UTILS_H__

/* public */

/**
 * modest_text_utils_quote:
 * @buf: a string which contains the message to quote
 * @from: the sender of the original message
 * @sent_date: sent date/time of the original message
 * @limit: specifies the maximum characters per line in the quoted text
 * 
 * Returns: a string containing the quoted message
 */
gchar *
modest_text_utils_quote(const gchar *buf,
                        const gchar *from,
                        const time_t sent_date,
                        const int limit);

/**
 * modest_text_utils_quote_text_buffer:
 * @buf: a GtkTextBuffer which contains the message to quote
 * @from: the sender of the original message
 * @sent_date: sent date/time of the original message
 * @limit: specifies the maximum characters per line in the quoted text
 * 
 * Returns: a string containing the quoted message
 * Please do not use this function, it may be removed
 */
gchar *
modest_text_utils_quote_text_buffer(GtkTextBuffer *buf,
                        const gchar *from,
                        const time_t sent_date,
                        const int limit);



#endif
