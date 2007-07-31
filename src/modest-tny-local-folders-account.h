/* Copyright (c) 2007, Nokia Corporation
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

#ifndef _MODEST_TNY_LOCAL_FOLDERS_ACCOUNT
#define _MODEST_TNY_LOCAL_FOLDERS_ACCOUNT

#include <tny-camel-store-account.h>

G_BEGIN_DECLS

#define MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT modest_tny_local_folders_account_get_type()

#define MODEST_TNY_LOCAL_FOLDERS_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT, ModestTnyLocalFoldersAccount))

#define MODEST_TNY_LOCAL_FOLDERS_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT, ModestTnyLocalFoldersAccountClass))

#define MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT))

#define MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT))

#define MODEST_TNY_LOCAL_FOLDERS_ACCOUNT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MODEST_TYPE_TNY_LOCAL_FOLDERS_ACCOUNT, ModestTnyLocalFoldersAccountClass))

/** ModestTnyLocalFoldersAccount contains references to folders that exist 
 * in other folder stores or accounts. It does not instantiate any folders 
 * of its own.
 */
typedef struct {
  TnyCamelStoreAccount parent;
} ModestTnyLocalFoldersAccount;

typedef struct {
  TnyCamelStoreAccountClass parent_class;
} ModestTnyLocalFoldersAccountClass;

GType modest_tny_local_folders_account_get_type (void);

ModestTnyLocalFoldersAccount* modest_tny_local_folders_account_new (void);

gboolean   modest_tny_local_folders_account_folder_name_in_use   (ModestTnyLocalFoldersAccount *self,
								  const gchar *name);

void       modest_tny_local_folders_account_add_folder_to_outbox (ModestTnyLocalFoldersAccount *self, 
								  TnyFolder *per_account_outbox);

G_END_DECLS

#endif /* _MODEST_TNY_LOCAL_FOLDERS_ACCOUNT */
