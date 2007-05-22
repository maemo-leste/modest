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
	MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE       = 1 << 1,
	MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE       = 1 << 2,
	MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE        = 1 << 3,
	MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE      = 1 << 4,
	MODEST_FOLDER_RULES_FOLDER_DONT_ACCEPT_FOLDERS = 1 << 5,
	MODEST_FOLDER_RULES_FOLDER_DONT_ACCEPT_MSGS    = 1 << 6
} ModestTnyFolderRules;

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
 * modest_tny_folder_is_local_folder:
 * @folder: a valid tnymail folder
 * 
 * checks if the folder is part of the "local folders" pseudo-account
 *  
 * Returns: TRUE if it's a local folder, FALSE otherwise
 */
gboolean modest_tny_folder_is_local_folder   (const TnyFolder *folder);


/**
 * modest_tny_folder_get_local_folder_type:
 * @folder: a valid tnymail folder
 * 
 * checks if the folder is part of the "local folders" pseudo-account
 *  
 * Returns: TRUE if it's a local folder, FALSE otherwise
 */
TnyFolderType modest_tny_folder_get_local_folder_type  (const TnyFolder *folder);


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
ModestTnyFolderRules  modest_tny_folder_get_rules   (const TnyFolder *folder);

G_END_DECLS

#endif /* __MODEST_TNY_FOLDER_H__*/
