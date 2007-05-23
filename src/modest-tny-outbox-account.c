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

#include <modest-tny-outbox-account.h>

G_DEFINE_TYPE (ModestTnyOutboxAccount, modest_tny_outbox_account, TNY_TYPE_CAMEL_STORE_ACCOUNT);

#define TNY_OUTBOX_ACCOUNT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_TNY_OUTBOX_ACCOUNT, ModestTnyOutboxAccountPrivate))

typedef struct _ModestTnyOutboxAccountPrivate ModestTnyOutboxAccountPrivate;

struct _ModestTnyOutboxAccountPrivate
{
	/* This it the outbox account for this store account: */
	gchar *parent_account_id;
};

static void
modest_tny_outbox_account_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (modest_tny_outbox_account_parent_class)->dispose)
    G_OBJECT_CLASS (modest_tny_outbox_account_parent_class)->dispose (object);
}

static void
modest_tny_outbox_account_finalize (GObject *object)
{
  G_OBJECT_CLASS (modest_tny_outbox_account_parent_class)->finalize (object);
}

static void
modest_tny_outbox_account_class_init (ModestTnyOutboxAccountClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ModestTnyOutboxAccountPrivate));

  object_class->dispose = modest_tny_outbox_account_dispose;
  object_class->finalize = modest_tny_outbox_account_finalize;
}

static void
modest_tny_outbox_account_init (ModestTnyOutboxAccount *self)
{
}

ModestTnyOutboxAccount*
modest_tny_outbox_account_new (void)
{
  return g_object_new (MODEST_TYPE_TNY_OUTBOX_ACCOUNT, NULL);
}
