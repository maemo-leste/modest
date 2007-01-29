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
 * TnyAccount Decorator
 */

#ifndef __MODEST_TNY_ACCOUNT_H__
#define __MODEST_TNY_ACCOUNT_H__

#include <tny-account.h>
#include <tny-folder.h>
#include <modest-local-folder-info.h>

G_BEGIN_DECLS

/**
 * modest_tny_account_get_special_folder
 * @self: a TnyAccount
 * @special_type: the special folder to get
 * 
 * get the special (Inbox,Outbox,Sent,Draft etc.) folder for this account
 * Note: currently, the implementation will always return a local folder for this.
 * This can be changed later to return really account-specific special folders,
 * such as (for example) server-side Sent/Junk mail for IMAP accounts 
 * 
 * Returns: the tny folder corresponding to this special folder, or NULL in case
 * of error, or if the special folder does not exist for this account
 */
TnyFolder*    modest_tny_account_get_special_folder   (TnyAccount *self,
						       TnyFolderType special_type);

G_END_DECLS

#endif /* __MODEST_TNY_ACCOUNT_H__*/
