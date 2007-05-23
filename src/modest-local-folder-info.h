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

#ifndef __MODEST_LOCAL_FOLDER_INFO_H__
#define __MODEST_LOCAL_FOLDER_INFO_H__

G_BEGIN_DECLS

#include <glib.h>
#include <modest-defs.h>
#include <tny-folder.h>

/**
 * modest_local_folder_info_get_type
 * @name: the name of the local folder (ie. "trash", "inbox" etc.,
 * these name can be found with modest_local_folder_get_name)
 *
 * get the type of some local folder
 *  
 * Returns: the local folder type, or MODEST_LOCAL_FOLDER_TYPE_UNKNOWN
 * in case of error
 *
 */
TnyFolderType modest_local_folder_info_get_type (const gchar *name);

/**
 * modest_local_folder_get_type_name
 * @type: the type of the local folder
 * 
 * get the name of some local folder
 *  
 * Returns: the local folder name, or NULL in case of error
 * the returned name should NOT be freed or modified
 *
 */
const gchar* modest_local_folder_info_get_type_name (TnyFolderType type);

/**
 * modest_local_folder_info_get_type_display_name
 * @type: the type of the local folder
 * 
 * get the localized display name for some local folder
 *  
 * Returns: the local folder display name, or NULL in case of error
 * the returned name should NOT be freed or modified
 *
 */
const gchar* modest_local_folder_info_get_type_display_name (TnyFolderType type);


/**
 * modest_local_folder_info_get_maildir_path
 * 
 * Get the path to the Maildir where the local folders are stored.
 *  
 * Returns: the local_folders Maildir path as a newly allocated
 * string, which must be freed by the caller.
 *
 */
gchar *modest_local_folder_info_get_maildir_path (void);

/**
 * modest_per_account_local_outbox_folder_info_get_maildir_path
 * 
 * Get the path to the Maildir where the per-account local outbox folder is stored.
 *  
 * Returns: the local outbox account Maildir path as a newly allocated
 * string, which must be freed by the caller.
 *
 */
gchar *modest_per_account_local_outbox_folder_info_get_maildir_path (TnyAccount *account);

/**
 * modest_per_account_local_outbox_folder_info_get_maildir_path_to_outbox_folder
 * 
 * Get the path to the "outbox" folder directory under the local outbox account MailDir directory.
 *  
 * Returns: the local outbox folder Maildir path as a newly allocated
 * string, which must be freed by the caller.
 *
 */
gchar *modest_per_account_local_outbox_folder_info_get_maildir_path_to_outbox_folder (TnyAccount *account);

G_END_DECLS
#endif /* __MODEST_LOCAL_FOLDER_INFO_H__ */

