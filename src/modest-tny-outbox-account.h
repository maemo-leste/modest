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

#ifndef __MODEST_TNY_OUTBOX_ACCOUNT_H__
#define __MODEST_TNY_OUTBOX_ACCOUNT_H__

#include <tny-camel-store-account.h>

G_BEGIN_DECLS

#define MODEST_TYPE_TNY_OUTBOX_ACCOUNT modest_tny_outbox_account_get_type()

#define MODEST_TNY_OUTBOX_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MODEST_TYPE_TNY_OUTBOX_ACCOUNT, ModestTnyOutboxAccount))

#define MODEST_TNY_OUTBOX_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MODEST_TYPE_TNY_OUTBOX_ACCOUNT, ModestTnyOutboxAccountClass))

#define MODEST_IS_TNY_OUTBOX_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MODEST_TYPE_TNY_OUTBOX_ACCOUNT))

#define MODEST_IS_TNY_OUTBOX_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MODEST_TYPE_TNY_OUTBOX_ACCOUNT))

#define MODEST_TNY_OUTBOX_ACCOUNT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MODEST_TYPE_TNY_OUTBOX_ACCOUNT, ModestTnyOutboxAccountClass))

/** ModestTnyOutboxAccount is a non-user-visible account
 * that provides maildir-based access to the on-disk 
 * per-account outbox directory.
 * The actual user-visible outbox folder is actually a TnyMergeFolder that 
 * merges the outbox folder from several ModestTnyOutboxAccount instances.
 * 
 * This is a simple alternative to reimplementing get_folders_func() for 
 * every tinymail camel account type.
 */
typedef struct {
  TnyCamelStoreAccount parent;
} ModestTnyOutboxAccount;

typedef struct {
  TnyCamelStoreAccountClass parent_class;
} ModestTnyOutboxAccountClass;

GType modest_tny_outbox_account_get_type (void);

ModestTnyOutboxAccount* modest_tny_outbox_account_new (void);

G_END_DECLS

#endif /* __MODEST_TNY_OUTBOX_ACCOUNT_H__ */
