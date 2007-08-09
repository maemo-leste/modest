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

/*
 * TnyFolder Decorator
 */

#ifndef __MODEST_TNY_FOLDER_H__
#define __MODEST_TNY_FOLDER_H__

#include <tny-folder.h>
#include <modest-local-folder-info.h>

G_BEGIN_DECLS

typedef enum {
	MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE  = 1 << 1,
	MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE  = 1 << 2,
	MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE   = 1 << 3,
	MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE = 1 << 4,
} ModestTnyFolderRules;

/** Note: This is not a derived TnyFolder type. These are just convenience 
 * functions for working with a TnyFolder. tinymail does not seem to offer any 
 * easy way to cause derived TnyFolders to be instantiated.
 */
 
/* TODO: These "const TnyFolder*" arguments will eventually need to 
 * be "TnyFolder*". C cannot support constness for complex objects like C++ 
 * can, because it lacks the mutable keyword and doesn't allow both const 
 * and non-const get function overloads.
 */
 
/**
 * modest_tny_folder_guess_type:
 * @folder: a valid tnymail folder
 * 
 * determine the type of the folder. first, we see if tinymail
 * can give a specific type. if it cannot, we try to guess the
 * type, using modest_tny_folder_guess_type_from_name
 *  
 * Returns: the folder type, or TNY_FOLDER_TYPE_UNKNOWN
 */
TnyFolderType  modest_tny_folder_guess_folder_type   (const TnyFolder *folder);

/**
 * modest_tny_folder_guess_type_from_name:
 * @folder_name: a folder name
 * 
 * determine the type of the folder. first, we see if tinymail
 * can give a specific type. if it cannot, we try to guess the
 * type, based on the name of the folder
 *
 * Note: this is a Class function, there does not require a tnyfolder instance
 *  
 * Returns: the folder type, or TNY_FOLDER_TYPE_UNKNOWN
 */
TnyFolderType  modest_tny_folder_guess_folder_type_from_name   (const gchar *folder_name);


/**
 * modest_tny_folder_is_remote_folder:
 * @folder: a valid tnymail folder
 * 
 * checks if the folder is part of the "remote" account
 *  
 * Returns: TRUE if it's a remote folder, FALSE otherwise
 */
gboolean
modest_tny_folder_is_remote_folder   (TnyFolder *folder);

/**
 * modest_tny_folder_is_local_folder:
 * @folder: a valid tnymail folder
 * 
 * checks if the folder is part of the "local folders" pseudo-account
 *  
 * Returns: TRUE if it's a local folder, FALSE otherwise
 */
gboolean modest_tny_folder_is_local_folder   (TnyFolder *folder);

/**
 * modest_tny_folder_is_memory_card_folder:
 * @folder: a valid tnymail folder
 * 
 * checks if the folder is part of the memory card account.
 *  
 * Returns: TRUE if it's a memory card folder, FALSE otherwise
 */
gboolean
modest_tny_folder_is_memory_card_folder   (TnyFolder *folder);

/**
 * modest_tny_folder_get_local_folder_type:
 * @folder: a valid tnymail folder
 * 
 * checks if the folder is part of the "local folders" pseudo-account
 *  
 * Returns: TRUE if it's a local folder, FALSE otherwise
 */
TnyFolderType modest_tny_folder_get_local_or_mmc_folder_type  (TnyFolder *folder);


/**
 * modest_tny_folder_get_rules:
 * @folder: a valid tnymail folder
 * 
 * get the rules for this folder; can messages be put in it,
 * can the folder be deleted, etc.
 *  
 * Returns: the ModestTnyFolderRules rules (bitwise-OR) for this
 * folder
 */
ModestTnyFolderRules  modest_tny_folder_get_rules   (TnyFolder *folder);

/**
 * modest_tny_folder_is_outbox_for_account:
 * @folder: a valid tnymail folder
 * 
 * Discover whether this folder is the per-account outbox for the specified 
 * account.
 *  
 * Returns: TRUE if this folder is the per-account outbox for the account.
 */
gboolean modest_tny_folder_is_outbox_for_account (TnyFolder *folder, 
						  TnyAccount *account);
					
/**
 * modest_tny_folder_get_account:
 * @folder: a folder
 * 
 * Get the parent account of the folder or, for TnyMergeFolder 
 * instances, get the local-folders account.
 *  
 * Returns: the account. You should call g_object_unref() on this.
 */	  
TnyAccount *modest_tny_folder_get_account (TnyFolder *folder);

/**
 * modest_tny_msg_get_header_unique_id:
 * @header: a #TnyHeader
 * 
 * This function returns a unique id for a message summary from 
 * a TnyHeader retrieved with tny_folder_get_headers. You can not use
 * the TnyHeader returned by tny_msg_get_header because it has no uid.
 *
 * This uid is built from the folder URL string and the header uid,
 * the caller of the function must free the unique id when no longer
 * needed
 * 
 * Returns: a unique identificator for a header object
 **/
gchar* modest_tny_folder_get_header_unique_id (TnyHeader *header);

G_END_DECLS

#endif /* __MODEST_TNY_FOLDER_H__*/
