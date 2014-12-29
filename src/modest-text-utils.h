/* Copyright (c) 2006, Nokia Corporation
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


/* modest-text-utils.h */

#ifndef __MODEST_TEXT_UTILS_H__
#define __MODEST_TEXT_UTILS_H__

#include <time.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define _FM(str) dgettext("hildon-fm",str)
#define _CS(str) dgettext("hildon-common-strings",str)
#define _HL(str) dgettext("hildon-libs",str)
#define _MD(str) dgettext("maemo-af-desktop",str)
#define _AB(str) dgettext("osso-addressbook",str)
#define _KR(str) dgettext("ke-recv",str)
#define _UR(str) dgettext("osso-uri",str)

#ifdef MODEST_TOOLKIT_HILDON2
#define _HL_SAVE _HL("wdgt_bd_save")
#define _HL_YES _HL("wdgt_bd_yes")
#define _HL_NO _HL("wdgt_bd_no")
#define _HL_VIEW _HL("wdgt_bd_view")
#define _HL_TITLE_SORT _HL("ckdg_ti_sort")
#define _HL_TITLE_NEW_FOLDER _HL("ckdg_ti_new_folder")
#define _HL_TITLE_RENAME_FOLDER _HL("ckdg_ti_rename_folder")
#define _HL_DATE _HL("wdgt_va_date")
#define _HL_24H_TIME _HL("wdgt_va_24h_time")
#define _HL_WEEK _HL("wdgt_va_week")
#define _HL_DATE_MEDIUM _HL("wdgt_va_date_medium")
#define _HL_12H_TIME_PM _HL("wdgt_va_12h_time_pm")
#define _HL_12H_TIME_AM _HL("wdgt_va_12h_time_am")
#define _HL_DONE _HL("wdgt_bd_done")
#define _HL_DELETE _HL("wdgt_bd_delete")
#define _HL_MOVE _HL("wdgt_bd_move")
#define _HL_RENAME _HL("wdgt_bd_rename")
#define _HL_IB_FIND_COMPLETE _HL("ckct_ib_find_search_complete")
#define _HL_IB_FIND_NO_MATCHES _HL("ckct_ib_find_no_matches")
#define _HL_IB_ZOOM _HL("wdgt_ib_zoom")

#define _CS_ILLEGAL_CHARACTERS_ENTERED _CS("ckdg_ib_illegal_characters_entered")
#define _CS_CANNOT_ZOOM_HERE _CS("ckct_ib_cannot_zoom_here")
#define _CS_MAX_ZOOM_LEVEL_REACHED _CS("ckct_ib_max_zoom_level_reached")
#define _CS_MIN_ZOOM_LEVEL_REACHED _CS("ckct_ib_min_zoom_level_reached")
#define _CS_MAXIMUM_CHARACTERS_REACHED _CS("ckdg_ib_maximum_characters_reached")
#define _CS_FOLDER_ALREADY_EXISTS _CS("ckdg_ib_folder_already_exists")
#define _CS_SET_PASSWORD_INCORRECT _CS("ecdg_ib_set_password_incorrect")
#define _CS_UNABLE_TO_PASTE_HERE _CS("ckct_ib_unable_to_paste_here")
#define _CS_UNABLE_TO_OPEN_FILE_NOT_FOUND _CS("sfil_ni_unable_to_open_file_not_found")
#define _CS_UNABLE_TO_RENAME _CS("ckdg_ib_unable_to_rename")
#define _CS_UNABLE_TO_DELETE _CS("ckdg_ib_unable_to_delete")
#define _CS_NOTHING_TO_SORT _CS("ckdg_ib_nothing_to_sort")
#define _CS_NOT_ENOUGH_MEMORY _CS("sfil_ni_not_enough_memory")
#define _CS_FOLDER_ALREADY_EXISTS _CS("ckdg_ib_folder_already_exists")
#define _CS_PASTING _CS("ckct_nw_pasting")
#define _CS_GETTING_ITEMS _CS("mcen_ib_getting_items")
#define _CS_COPIED _CS("ecoc_ib_edwin_copied")
#define _CS_UNABLE_TO_SEND _CS("sfil_ib_unable_to_send")
#define _CS_FIND_REP_ENTER_TEXT _CS("ecdg_ib_find_rep_enter_text")
#define _CS_UPDATING _CS("ckdg_pb_updating")
#define _CS_SAVED _CS("sfil_ib_saved")

#define _FM_CHANGE_FOLDER _FM("ckdg_ti_change_folder")
#define _FM_NEW_FOLDER_NAME_STUB _FM("ckdg_va_new_folder_name_stub")
#define _FM_NEW_FOLDER_DIALOG_OK _FM("ckdg_bd_new_folder_dialog_ok")
#define _FM_NEW_FOLDER_LOCATION _FM("ckdg_fi_new_folder_location")
#define _FM_NEW_FOLDER_NAME _FM("ckdg_fi_new_folder_name")
#define _FM_FOLDER_UP _FM("filemanager_folder_up")
#define _FM_SIZE_KB _FM("sfil_li_size_kb")
#define _FM_SIZE_1KB_99KB _FM("sfil_li_size_1kb_99kb")
#define _FM_SIZE_100KB_1MB _FM("sfil_li_size_100kb_1mb")
#define _FM_SIZE_1MB_10MB _FM("sfil_li_size_1mb_10mb")
#define _FM_SIZE_10MB_1GB _FM("sfil_li_size_10mb_1gb")
#define _FM_SIZE_1GB_OR_GREATER _FM("sfil_li_size_1gb_or_greater")
#define _FM_OPENING_NOT_ALLOWED _FM("sfil_ib_opening_not_allowed")
#define _FM_REPLACE_FILE _FM("docm_nc_replace_file")
#define _FM_REPLACE_MULTIPLE _FM("docm_nc_replace_multiple")
#define _FM_READ_ONLY_LOCATION _FM("sfil_ib_readonly_location")
#define _FM_SAVE_OBJECT_FILES _("mcen_me_viewer_save_attachments")
#define _FM_CHANGE_FOLDER_NEW_FOLDER ("ckdg_bd_change_folder_new_folder")
#else
#define _HL_SAVE _("Save")
#define _HL_YES _("Yes")
#define _HL_NO _("Yes")
#define _HL_VIEW _("View")
#define _HL_TITLE_SORT _("Sort")
#define _HL_TITLE_NEW_FOLDER _("New folder")
#define _HL_TITLE_RENAME_FOLDER _HL("Rename folder")
#define _HL_RENAME_NAME _("Name")
#define _HL_24H_TIME _("%H:%M")
#define _HL_DATE _("%m/%d/%Y")
#define _HL_WEEK _("%A")
#define _HL_DATE_MEDIUM _("%e %B %Y")
#define _HL_12H_TIME_PM _("%l:%M pm")
#define _HL_12H_TIME_AM _("%l:%M am")
#define _HL_DONE _("Done")
#define _HL_DELETE _("Delete")
#define _HL_MOVE _("Move")
#define _HL_RENAME _("Rename")
#define _HL_IB_FIND_COMPLETE _("Search complete")
#define _HL_IB_FIND_NO_MATCHES _("No matches")
#define _HL_IB_ZOOM _("Zoom %d")

#define _CS_ILLEGAL_CHARACTERS_ENTERED _("Illegal characters entered")
#define _CS_CANNOT_ZOOM_HERE _("Cannot zoom here")
#define _CS_MAX_ZOOM_LEVEL_REACHED _("Max. zoom level reached")
#define _CS_MIN_ZOOM_LEVEL_REACHED _("Min. zoom level reached")
#define _CS_MAXIMUM_CHARACTERS_REACHED _("Maximum characters reached")
#define _CS_FOLDER_ALREADY_EXISTS _("Folder already exists")
#define _CS_SET_PASSWORD_INCORRECT _("Password incorrect")
#define _CS_UNABLE_TO_PASTE_HERE _("Unable to paste here")
#define _CS_UNABLE_TO_OPEN_FILE_NOT_FOUND _("File not found")
#define _CS_UNABLE_TO_RENAME _("Unable to rename")
#define _CS_UNABLE_TO_DELETE _("Unable to delete")
#define _CS_NOTHING_TO_SORT _("Nothing to sort")
#define _CS_NOT_ENOUGH_MEMORY _("Not enough memory")
#define _CS_FOLDER_ALREADY_EXISTS _("Folder already exists")
#define _CS_PASTING _("Pasting")
#define _CS_GETTING_ITEMS _("Getting items")
#define _CS_COPIED _("Copied")
#define _CS_UNABLE_TO_SEND _("Unable to send")
#define _CS_FIND_REP_ENTER_TEXT _("Enter text to search")
#define _CS_UPDATING _("Updating...")
#define _CS_SAVED _("Saved")

#define _FM_CHANGE_FOLDER _("Change folder")
#define _FM_NEW_FOLDER_NAME_STUB _("Folder")
#define _FM_NEW_FOLDER_DIALOG_OK _("Save")
#define _FM_NEW_FOLDER_LOCATION _("Location")
#define _FM_NEW_FOLDER_NAME _("Name")
#define _FM_FOLDER_UP GTK_STOCK_GO_UP
#define _FM_SIZE_KB _("%d KB")
#define _FM_SIZE_1KB_99KB _("%d KB")
#define _FM_SIZE_100KB_1MB _("%d KB")
#define _FM_SIZE_1MB_10MB _("%.2f MB")
#define _FM_SIZE_10MB_1GB _("%.1f MB")
#define _FM_SIZE_1GB_OR_GREATER _("%.2f GB")
#define _FM_OPENING_NOT_ALLOWED _("Opening not allowed")
#define _FM_REPLACE_FILE _("Replace the existing file with another with the same name?")
#define _FM_REPLACE_MULTIPLE _("Replace existing contents in folder with the same name?")
#define _FM_READ_ONLY_LOCATION _("Selected location is read only")
#define _FM_SAVE_OBJECT_FILES _("Save files")
#define _FM_CHANGE_FOLDER_NEW_FOLDER ("New folder")
#endif

/* Forbidden char arrays */
extern const gchar account_title_forbidden_chars[];
extern const gchar folder_name_forbidden_chars[];
extern const gchar user_name_forbidden_chars[];
extern const guint ACCOUNT_TITLE_FORBIDDEN_CHARS_LENGTH;
extern const guint FOLDER_NAME_FORBIDDEN_CHARS_LENGTH;
extern const guint USER_NAME_FORBIDDEN_CHARS_LENGTH;

/* It includes a white space as RFC 3676 Section 4.3 about usenet
   message signatures defines */
#define MODEST_TEXT_UTILS_SIGNATURE_MARKER "-- "

/**
 * modest_text_utils_derived_subject:
 * @subject: a string which contains the original subject
 * @is_reply: whether the derived subject is for a reply or a forward message
 *
 * create a 'derived' subject line for eg. replies and forwards. Note
 * that this function will use the localized versions of "Re" and
 * "Fw", unless one of these two versions was already included. For
 * example replying to an email in Finish would work as:
 *
 * "some subject"     -> "VS: some subject"
 * "VS: some subject" -> "VS: some subject"
 * "Re: some subject" -> "Re: some subject"
 * "Fw: some subject" -> "VS: Fw: some subject"
 *
 * Returns: a newly allocated string containing the resulting subject
 */
gchar* modest_text_utils_derived_subject (const gchar *subject,
					  gboolean is_reply);


/**
 * modest_text_utils_quote:
 * @text: a non-NULL string which contains the message to quote
 * @from: a non-NULL  sender of the original message
 * @content_type: the non-NULL content type for the quoting, e.g. "text/html"
 * @signature: NULL or the signature to add
 * @sent_date: sent date/time of the original message
 * @attachments: a #GList of the attachments
 * @limit: specifies the maximum characters per line in the quoted text
 * 
 * quote an existing message
 * 
 * Returns: a newly allocated string containing the quoted message
 */
gchar* modest_text_utils_quote (const gchar *text, 
				const gchar *content_type,
				const gchar *signature,
			        const gchar *from,
			        const time_t sent_date, 
				GList *attachments,
				int limit);


/**
 * modest_text_utils_cited_text:
 * @from: sender of the message
 * @sent_date: the sent date of the original message
 * @text: the text of the original message
 *
 * cite the text in a message
 * 
 * Returns: a newly allocated string containing the cited text
 */
gchar* modest_text_utils_cite (const gchar *text,
			       const gchar *content_type,
			       const gchar *signature,
			       const gchar *from,
			       time_t sent_date);

/**
 * modest_text_utils_inlined_text
 * @from: the non-NULL sender of the original message
 * @sent_date: sent date/time of the original message
 * @to: 
 * @subject: 
 * @text: 
 *
 * creates a new string with the "Original message" text prepended to
 * the text passed as argument and some data of the header
 * 
 * Returns: a newly allocated string containing the quoted message
 */
gchar*   modest_text_utils_inline (const gchar *text,
				   const gchar *content_type,
				   const gchar *signature,
				   const gchar *from,
				   time_t sent_date,
				   const gchar *to,
				   const gchar *subject);

/**
 * modest_text_utils_remove_address
 * @address_list: non-NULL string with a comma-separated list of email addresses
 * @address: an specific e-mail address 
 *
 * remove a specific address from a list of email addresses; if @address
 * is NULL, returns an unchanged (but newly allocated) @address_list
 * 
 * Returns: a newly allocated string containing the new list, or NULL
 * in case of error or the original @address_list was NULL
 */
gchar*   modest_text_utils_remove_address (const gchar *address_list, 
					   const gchar *address);


/**
 * modest_text_utils_remove_duplicate_addresses
 * @address_list: non-NULL string with a comma-separated list of email addresses
 *
 * remove duplicate addresses from a list of email addresses
 * 
 * Returns: a newly allocated string containing the new list, or NULL
 * in case of error or the original @address_list was NULL
 */
gchar*   modest_text_utils_remove_duplicate_addresses (const gchar *address_list); 


/**
 * modest_text_utils_address_range_at_position:
 * @address_list: non-NULL utf8 string containing a list of addresses
 * @position: a gint
 * @start: a gint pointer
 * @end: a gint pointer
 *
 * Finds the start and end positions of the address at @position,
 * in @recipients_list, a list of addresses in the format of a 
 * recipient list in email. It stores the results in @start and
 * @end
 */
void     modest_text_utils_address_range_at_position (const gchar *recipients_list,
						      guint position,
						      guint *start,
						      guint *end);

/**
 * modest_text_utils_hyperlinkify_begin:
 *
 * begin a linkify block, compiling the caches to be reused. Use it in mainloop.
 */
void modest_text_utils_hyperlinkify_begin (void);

/**
 * modest_text_utils_hyperlinkify_end:
 *
 * end a linkify block, freeing the caches to be reused. Use it in mainloop.
 */
void modest_text_utils_hyperlinkify_end (void);

/**
 * modest_text_utils_convert_to_html:
 * @txt: a string
 *
 * convert plain text (utf8) into html
 * 
 * Returns: a newly allocated string containing the html
 */
gchar*  modest_text_utils_convert_to_html (const gchar *txt);

/**
 * modest_text_utils_convert_to_html_body:
 * @txt: a string
 *
 * convert plain text (utf8) into html without adding html headers.
 * 
 * Returns: a newly allocated string containing the html
 */
gchar*  modest_text_utils_convert_to_html_body (const gchar *data, gssize n, gboolean hyperlinkify);


/**
 * modest_text_utils_strftime:
 * @s:
 * @max:
 * @fmt:
 * @timet:
 *
 * this is just an alias for strftime(3), so we can use that without
 * getting warning from gcc
 * 
 * Returns: a formatted string of max length @max in @s
 */
size_t modest_text_utils_strftime(char *s, size_t max, const char  *fmt, time_t timet);

/**
 * modest_text_utils_hyperlinkify:
 * @string_buffer: buffer where we replace uri strings with links
 *
 * Replace uri's with links in the buffer. This is required that the document
 * do not contain linkified links already.
 */
void modest_text_utils_hyperlinkify (GString *string_buffer);

/**
 * modest_text_utils_get_display_address:
 * @address: original address (UTF8 string)
 *
 * make a 'display address' from an address:
 * "Foo Bar &lt;foo@bar.cx&gt;" --&gt; "Foo Bar"
 * ie. removes "&lt;...&gt;" parts
 * the change is in-place; removes leading whitespace
 * 
 * NOTE: for optimization reasons, this function changes @address
 * in-place
 */
void modest_text_utils_get_display_address (gchar *address);

/**
 * modest_text_utils_get_display_addresses:
 * @addresses: a list of comma-separated addresses
 *
 * Transforms a list of email addresses in a list of recipients,
 * replacing each plain email address by the correspondent display
 * address.
 *
 * Returns: a newly allocated string, that must be freed by the caller
 **/
gchar *modest_text_utils_get_display_addresses (const gchar *addresses);


/**
 * modest_text_utils_get_email_address:
 * @full_address: original address (UTF8 string)
 *
 * make a 'foo@bar.cx' from an address:
 * "Foo Bar <foo@bar.cx> (Bla)" --> "foo@bar.cx"
 * If no "<...>" is found, then it returns the full
 * strings.
 * 
 * Returns: a newly allocated string with the copy.
 * 
 * NULL in case of error or if address == NULL
 */
gchar* modest_text_utils_get_email_address (const gchar *email_address);


/**
 * modest_text_utils_get_subject_prefix_len:
 * @subject: original subject (UTF8 string)
 *
 * determine the length of the "Re:/RE:/Fwd:" prefix in an e-mail address
 * 
 * Returns: the length of the  prefix, or 0 if there is none
 */
gint modest_text_utils_get_subject_prefix_len (const gchar *subject);


/**
 * modest_text_utils_utf8_strcmp:
 * @s1: the first string
 * @s2: the second string
 * @insensitive: should the comparison be case-insensitive?
 *
 * a strcmp that is NULL-safe, can deal with UTF8 and case-insensitive comparison 
 *
 * Returns: an integer less than, equal to, or greater than zero if s1 is found,
 * respectively, to be less than, to match, or be greater than s2.
 */
gint modest_text_utils_utf8_strcmp (const gchar* s1, const gchar *s2, gboolean insensitive);



/**
 * modest_text_utils_get_display_date:
 * @date: the date to display
 *
 * get a string representation for a date.
 * 
 * Returns: the new display date, as a *static* string.
 * This string should not be modified, and will change
 * upon recalling this function. g_strdup it if you to
 * do so.
 * 
 */
const gchar* modest_text_utils_get_display_date (time_t date);


/**
 * modest_text_utils_get_display_size:
 * @size: size in bytes
 *
 * get a string representation for a size in bytes.
 * 
 * Returns: the newly allocated display string for the
 * size in bytes. must be freed.
 */
gchar * modest_text_utils_get_display_size (guint64 size);



/**
 * modest_text_utils_validate_domain_name:
 * @email_address: a NULL-terminated string
 * 
 * validates the domain name passed as argument
 * 
 * Returns: TRUE if the domain name is valid, FALSE otherwise
 **/
gboolean modest_text_utils_validate_domain_name (const gchar *domain);

/**
 * modest_text_utils_validate_email_address:
 * @email_address: a string
 * @invalid_char_position: pointer to the position of the invalid
 * character in case validation failed because of this, or %NULL.
 * 
 * validates the email address passed as argument
 * 
 * Returns: TRUE if the address is valid, FALSE otherwise
 **/
gboolean     modest_text_utils_validate_email_address (const gchar *email_address, 
						       const gchar **invalid_char_position);


/**
 * modest_text_utils_validate_folder_name:
 * @folder_name: a string
 * 
 * validates the folder name passed as argument. a 'valid folder name'
 * is a name which should be valid on both Unix and Windows file systems.
 * of course, this might be stricter than strictly needed in some cases,
 * but it's better to err on the safe side.
 * 
 * Returns: TRUE if the folder name is valid, FALSE otherwise
 **/
gboolean modest_text_utils_validate_folder_name (const gchar *folder_name);

/**
 * modest_text_utils_validate_recipient:
 * @recipient: a string
 * @invalid_char_position: pointer to the position of the invalid char,
 * if validation failed because there's an invalid char there, or %NULL.
 *
 * validates @recipient as a valid recipient field for header.
 * It's different from modest_text_utils_validate_email_address()
 * as it validates a whole recipient, and not only the part between
 * the &lt; and &gt; symbols.
 *
 * Returns: %TRUE if the recipient is valid, FALSE otherwise
 **/
gboolean     modest_text_utils_validate_recipient (const gchar *recipient,
						   const gchar **invalid_char_position);

/**
 * modest_text_utils_split_addresses_list:
 * @addresses: a string
 *
 * obtains a GSList of addresses from a string of addresses
 * in the format understood by email protocols
 *
 * Returns: a newly allocated GSList of strings
 **/
GSList      *modest_text_utils_split_addresses_list (const gchar *addresses);

/**
 * modest_text_utils_join_addresses:
 * @from: comma separated string of addresses
 * @to: comma separated string of addresses
 * @cc: comma separated string of addresses
 * @bcc: comma separated string of addresses
 *
 * joins all the addresses in a single comma-separated string
 *
 * Returns: a newly allocated string with a list of addresses
 **/
gchar       *modest_text_utils_join_addresses (const gchar *from,
					       const gchar *to,
					       const gchar *cc,
					       const gchar *bcc);

/**
 * modest_text_utils_get_addresses_indexes:
 * @addresses: a string
 * @start_indexes: a #GSList pointer
 * @end_indexes: a #GSList pointer
 *
 * obtains two #GSList of @addresses with the range offsets of the addresses in
 * the string
 *
 * Returns: a GSList of strings
 **/
void         modest_text_utils_get_addresses_indexes (const gchar *addresses, GSList **start_indexes, GSList **end_indexes);

/**
 * modest_text_utils_address_with_standard_length:
 * @recipients_list: a string
 *
 * obtains the list of recipients, but making sure that lines are not longer than 1000 chars
 *
 * Returns: a newly allocated string
 */
gchar *      modest_text_utils_address_with_standard_length (const gchar *recipients_list);

/**
 * modest_text_utils_get_color_string:
 * @color: a #GdkColor
 *
 * Obtains a proper markup string for @color, in the format used
 * by Pango and HTML.
 *
 * Returns: a newly allocated string
 */
gchar *      modest_text_utils_get_color_string (GdkColor *color);

/**
 * modest_text_utils_text_buffer_get_text:
 * @buffer: a #GtkTextBuffer
 *
 * Obtains the contents of a @buffer in a string, replacing image
 * pixbufs with blank spaces.
 *
 * Returns: a newly allocated UTF-8 string
 */
gchar *      modest_text_utils_text_buffer_get_text (GtkTextBuffer *buffer);

typedef enum {
	ACCOUNT_TITLE_FORBIDDEN_CHARS,
	FOLDER_NAME_FORBIDDEN_CHARS,
	USER_NAME_FORBIDDEN_NAMES,
} ModestTextUtilsForbiddenCharType;

/**
 * modest_text_utils_label_get_selection:
 * @label: a #GtkLabel
 *
 * Obtain the current selection of @label
 *
 * Returns: a string with current selection, or %NULL if no selection in @label
 */
gchar *      modest_text_utils_label_get_selection (GtkLabel *label);

/**
 * modest_text_utils_is_forbidden_char:
 * @character: some character
 * @type: the type of forbidden char (see #ModestTextUtilsForbiddenCharType)
 * 
 * check whether the given character is 'forbidden'
 *
 * Returns: TRUE if it's forbidden, FALSE otherwise
 */
gboolean     modest_text_utils_is_forbidden_char (const gchar character,
						  ModestTextUtilsForbiddenCharType type);

/**
 * modest_text_utils_buffer_selection_is_valid:
 * @buffer: a #GtkTextBuffer
 *
 * Checks if @buffer contains a valid selection for cut/copy. This means it's
 * not empty, and no images are in the selection.
 *
 * Returns: %TRUE if there's a valid selection, false otherwise.
 */
gboolean     modest_text_utils_buffer_selection_is_valid (GtkTextBuffer *buffer);

/**
 * modest_text_utils_escape_mnemonics:
 * @text: a string
 *
 * obtains the representation of text, but escaping mnemonics (we duplicate _)
 *
 * Returns: a newly allocated string
 */
gchar *modest_text_utils_escape_mnemonics (const gchar *text);

/**
 * modest_text_utils_simplify_recipients:
 * @recipients: a list of recipients
 *
 * returns a list of simplified recipients:
 *   * a@b <a@b> converted to a@b
 *   * NULL converted to ""
 *
 * It's mainly intended for printing in screen addresses, but it can
 * also be used for reply/forward.
 */
gchar *modest_text_utils_simplify_recipients (const gchar *recipient);

/**
 * modest_text_utils_remove_duplicate_addresses_list
 * @address_list: non-NULL #GSList of email addresses
 *
 * remove duplicate addresses from a list of email addresses
 *
 * Returns: a list without the duplicate addresses or NULL in case of
 * error or the original @address_list was NULL
 */
GSList *modest_text_utils_remove_duplicate_addresses_list (GSList *address_list);

/**
 * modest_text_utils_get_secure_header:
 * @value: the value of a mail header
 * @header: the header that we're evaluating
 *
 * This function returns the secure value for a header. Basically it
 * avoids DoS attacks caused by specially malformed headers like for
 * example. From:From:From...From: some@mail.com
 *
 * Returns: returns the secured header
 **/
gchar * modest_text_utils_get_secure_header (const gchar *value, const gchar *header);

/**
 * modest_text_utils_quote_names:
 * @recipients: a list of valid email addresses separated by ',' or ';'
 *
 * This function quotes the name part of an email address if it's not
 * quoted and if it exists. For example
 * aaa@bbb.com -> aaa@bbb.com
 * "my name" <aaa@bbb.com> -> "my name" <aaa@bbb.com>
 * my name aaa@bbb.com -> "my name" aaa@bbb.com
 *
 * It even supports things like
 * my, name <aaa@bbb.com>, aaa@ccc.com -> "my, name" <aaa@bbb.com>; aaa@ccc.com
 *
 * Returns: a newly allocated string with the quoted email addresses
 **/
gchar * modest_text_utils_quote_names (const gchar *recipients);

gboolean modest_text_utils_no_recipient (GtkTextBuffer *buffer);

gchar * modest_text_utils_create_colored_signature (const gchar *signature);

gboolean modest_text_utils_live_search_find (const gchar *haystack, const gchar *needles);

/**
 * Create a valid filename based on the filename
 * @param[in] filename The oridinal filename, it might not be a valid filename
 * @return The allocated valid filename. Should be released with 'g_free ()'
 * @note This function allocates a new string that confirms to the filename rules.
 *       All invalid characters are substituted to '_'.
 *       For example: "name!/.txt" -> "name__.txt"
 */
gchar* modest_text_utils_create_filename (const gchar *filename);

#endif /* __MODEST_TEXT_UTILS_H__ */
