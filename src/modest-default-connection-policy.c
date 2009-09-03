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

#include <config.h>
#include <glib.h>

#include "modest-default-connection-policy.h"
#include "modest-account-mgr.h"
#include "modest-account-mgr-helpers.h"
#include "modest-runtime.h"
#include "modest-tny-account.h"
#include "modest-ui-actions.h"
#include <tny-account.h>
#include <tny-store-account.h>

static GObjectClass *parent_class = NULL;

static void
modest_default_connection_policy_set_current (TnyConnectionPolicy *self, TnyAccount *account, TnyFolder *folder)
{
	return;
}

static void
modest_default_connection_policy_on_connect (TnyConnectionPolicy *self, TnyAccount *account)
{
	/* Set the username as succedded */
	if (TNY_IS_STORE_ACCOUNT (account)) {
		gboolean first_time = FALSE;
		const gchar *id;
		ModestAccountMgr *acc_mgr;

		acc_mgr = modest_runtime_get_account_mgr ();
		id = tny_account_get_id (account);
		first_time = !modest_account_mgr_get_server_account_username_has_succeeded (acc_mgr, id);

		/* If it's the first connection then perform a send&receive */
		if (first_time) {
			const gchar *account_name;
			ModestWindow *top_window;

			modest_account_mgr_set_server_account_username_has_succeeded (acc_mgr, id, TRUE);

			/* Perform a send receive */
			account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (account);
			top_window = modest_window_mgr_get_current_top (modest_runtime_get_window_mgr ());
			if (top_window)
				modest_ui_actions_do_send_receive (account_name, FALSE, FALSE, TRUE, top_window);
		}
	}

	/* Reset the attempt count */
	modest_tny_account_store_reset_attempt_count (modest_runtime_get_account_store (), account);

	return;
}

static void
modest_default_connection_policy_on_connection_broken (TnyConnectionPolicy *self, TnyAccount *account)
{
	return;
}

static void
modest_default_connection_policy_on_disconnect (TnyConnectionPolicy *self, TnyAccount *account)
{
	return;
}

static void
modest_default_connection_policy_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
modest_default_connection_policy_instance_init (GTypeInstance *instance, gpointer g_class)
{
}

static void
tny_connection_policy_init (TnyConnectionPolicyIface *klass)
{
	klass->on_connect= modest_default_connection_policy_on_connect;
	klass->on_connection_broken= modest_default_connection_policy_on_connection_broken;
	klass->on_disconnect= modest_default_connection_policy_on_disconnect;
	klass->set_current= modest_default_connection_policy_set_current;
}

static void
modest_default_connection_policy_class_init (ModestDefaultConnectionPolicyClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = modest_default_connection_policy_finalize;
}



/**
 * modest_default_connection_policy_new:
 * 
 * A connection policy
 *
 * Return value: A new #TnyConnectionPolicy instance 
 **/
TnyConnectionPolicy*
modest_default_connection_policy_new (void)
{
	return TNY_CONNECTION_POLICY (g_object_new (MODEST_TYPE_DEFAULT_CONNECTION_POLICY, NULL));
}

GType
modest_default_connection_policy_get_type (void)
{
	static GType type = 0;
	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
			sizeof (ModestDefaultConnectionPolicyClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) modest_default_connection_policy_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (ModestDefaultConnectionPolicy),
			0,      /* n_preallocs */
			modest_default_connection_policy_instance_init,    /* instance_init */
			NULL
		};


		static const GInterfaceInfo tny_connection_policy_info = 
		{
			(GInterfaceInitFunc) tny_connection_policy_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
			"ModestDefaultConnectionPolicy",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_CONNECTION_POLICY,
			&tny_connection_policy_info);

	}
	return type;
}
