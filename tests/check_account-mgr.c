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

#include <check.h>
#include <string.h>
#include <modest-defs.h>
#include <modest-conf.h>
#include <modest-account-mgr.h>
#include <modest-utils.h>
#include <gtk/gtk.h>
#include <modest-init.h>

/* ----------------------- Defines ---------------------- */

#define TEST_MODEST_ACCOUNT_NAME  "modest-unit-tests-àccount"

/* ------------------ Global variables ------------------ */

static ModestAccountMgr *account_mgr = NULL;

/* ---------------------- Fixtures --------------------- */

static void
fx_setup_default_account_mgr ()
{
	fail_unless (gtk_init_check (NULL, NULL));

	fail_unless (g_setenv (MODEST_DIR_ENV, ".modesttest", TRUE));
	fail_unless (g_setenv (MODEST_NAMESPACE_ENV, "/apps/modesttest", TRUE));
	fail_unless (modest_init (0, NULL), "Failed running modest_init");

	account_mgr = modest_runtime_get_account_mgr ();
	fail_unless (MODEST_IS_ACCOUNT_MGR (account_mgr),
		     "modest_account_mgr_new failed");

  	/* cleanup old garbage (from previous runs)*/
	if (modest_account_mgr_account_exists(account_mgr,
					      TEST_MODEST_ACCOUNT_NAME,
					      FALSE))
		modest_account_mgr_remove_account (account_mgr,
						   TEST_MODEST_ACCOUNT_NAME);
	if (modest_account_mgr_account_exists(account_mgr,
					      TEST_MODEST_ACCOUNT_NAME,
					      TRUE)) {
		modest_account_mgr_remove_server_account (account_mgr,
							  TEST_MODEST_ACCOUNT_NAME);
	}
}

/* ---------- add/exists/remove account tests  ---------- */

/**
 * Test regular usage of 
 * modest_account_mgr_add_account
 * modest_account_mgr_add_server_account
 * modest_account_mgr_account_exists
 * modest_account_mgr_remove_account
 *  - Test 1: Create anaccount
 *  - Test 2: Check account exists
 *  - Test 3: Remove account
 *  - Test 4: Create a server account
 *  - Test 5: Check server account exists
 *  - Test 6: Remove server account
 *  - Test 7: Check if a non-existing account exists
 *  - Test 8: Check if a non-existing server account exists
 */
START_TEST (test_add_exists_remove_account_regular)
{
	gchar *name = NULL;
	gchar *store_account = NULL;
	gchar *transport_account = NULL;
	gchar *hostname = NULL;
	gchar *username = NULL;
	gchar *password = NULL;
	gint portnum;
	ModestProtocolType proto;
	gboolean result;
	
	name = g_strdup (TEST_MODEST_ACCOUNT_NAME);
	/* Test 1 */
	store_account = g_strdup ("imap://me@myserver");
	transport_account = g_strdup ("local-smtp");
	result = modest_account_mgr_add_account (account_mgr,
						 name,
						 "test display name",
						 "user fullname",
						 "user@email.com",
						 MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY,
						 store_account,
						 transport_account, TRUE);
	fail_unless (result,
		     "modest_account_mgr_add_account failed:\n" \
		     "name: %s\nstore: %s\ntransport: %s\n",
		     name, store_account, transport_account, TRUE);
	
	g_free (store_account);
	g_free (transport_account);

	/* Test 2 */
	result = modest_account_mgr_account_exists (account_mgr,
						    name,
						    FALSE);
	fail_unless (result,
		     "modest_account_mgr_account_exists failed: " \
		     "Account with name \"%s\" should exist.\n", name);

	/* Test 2b */
	gchar *account_name_from_recipient;
	account_name_from_recipient = modest_utils_get_account_name_from_recipient ("user@email.com", NULL);
	fail_unless (account_name_from_recipient != NULL,
		     "modest_utils_get_account_name_from_recipient failed: "\
		     "From user@email.com should match account");
	
	/* Test 2c */
	account_name_from_recipient = modest_utils_get_account_name_from_recipient ("egg@egg.com", NULL);
	fail_unless (account_name_from_recipient == NULL,
		     "modest_utils_get_account_name_from_recipient failed: "\
		     "From egg@egg.com shouldn't match account");
	

	/* Test 3 */
	result = modest_account_mgr_remove_account (account_mgr,
						    name);
	fail_unless (result,
		     "modest_account_mgr_remove_account failed:\nname: %s\nerror: %s",
		     name);


	/* Test 4 */
	hostname = g_strdup ("myhostname.mydomain.com");
	username = g_strdup ("myusername");
	password = g_strdup ("mypassword");
	proto = MODEST_PROTOCOLS_TRANSPORT_SMTP;
	portnum = 25;
	result = modest_account_mgr_add_server_account (account_mgr,
							name,
							hostname,
							portnum,
							username,
							password,
							proto,
							MODEST_PROTOCOLS_CONNECTION_NONE,
							MODEST_PROTOCOLS_AUTH_NONE); 
	fail_unless (result,
		     "modest_account_mgr_add_server_account failed:\n" \
		     "name: %s\nhostname: %s\nusername: %s\npassword: %s\nproto: %d",
		     name, hostname, username, password, proto);

	g_free (hostname);
	g_free (username);
	g_free (password);
	
	/* Test 5 */
	result = modest_account_mgr_account_exists (account_mgr,name,TRUE);
	fail_unless (result,
		     "modest_account_mgr_account_exists failed: " \
		     "Server account with name \"%s\" should exist. Error: %s", name);

	/* Test 6 */
	result = modest_account_mgr_remove_server_account (account_mgr,
							   name);
	fail_unless (result,
		     "modest_account_mgr_remove_server_account failed:\nname: %s\nerror: %s",
		     name);


	/* Test 7 */
	result = modest_account_mgr_account_exists (account_mgr,
						    "a_name_that_does_not_exist",
						    FALSE);
	fail_unless (!result,
		     "modest_account_mgr_exists_account does not return " \
		     "FALSE when passing an account that does not exist");

	/* Test 8 */
	result = modest_account_mgr_account_exists (account_mgr,
						    "a_name_that_does_not_exist",
						    TRUE);
	fail_unless (!result,
		     "modest_account_mgr_exists_account does not return " \
		     "FALSE when passing a server account that does not exist");
	
	g_free (name);
}
END_TEST

/**
 * Test regular usage of 
 * modest_account_mgr_add_account,
 * modest_account_mgr_add_server_account
 * modest_account_mgr_account_exists
 * modest_account_mgr_remove_account
 *  - Test 1: Create account with NULL account_mgr
 *  - Test 2: Create account with NULL name
 *  - Test 3: Create account with invalid name string
 *  - Test 4: Create server account with NULL account_mgr
 *  - Test 5: Create server account with NULL name
 *  - Test 6: Create server account with invalid name string
 *  - Test 7: Remove a non-existing account
 *  - Test 8: Remove a non-existing server account (REMOVED)
 *  - Test 9: Remove with NULL acount manager
 *  - Test 10: Remove with NULL name
 *  - Test 11: Check if an  account exists with NULL account_mgr
 *  - Test 12: Check if a server account exists with a NULL account_mgr
 *  - Test 13: Check if a NULL account exists
 *  - Test 14: Check if a NULL server account exists
 */
START_TEST (test_add_exists_remove_account_invalid)
{
	gboolean result;

	/* Test 1 */
	result = modest_account_mgr_add_account (NULL,
						 TEST_MODEST_ACCOUNT_NAME,
						 "test display name",
						 "user fullname",
						 "user@email.com",
						 MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY,
						 "store_account",
						 "transport_account", TRUE);
	fail_unless (!result,
		     "modest_account_mgr_add_account does not return FALSE when" \
		     "passing a NULL ModestAccountMgr");

	/* Test 2 */
	result = modest_account_mgr_add_account (account_mgr,
						 NULL,
						 "test display name",
						 "user fullname",
						 "user@email.com",
						 MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY,
						 "store_account",
						 "transport_account", TRUE);
	fail_unless (!result,
		     "modest_account_mgr_add_account does not return FALSE when" \
		     "passing a NULL account name");

	/* Test 3*/
	result = modest_account_mgr_add_account (account_mgr,
						 "ïnválid//accountñ//nÄméç",
						 "test display name",
						 "user fullname",
						 "user@email.com",
						 MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY,
						 "store_account",
						 "transport_account", TRUE);
	fail_unless (!result,
		     "modest_account_mgr_add_account does not return FALSE when" \
		     "passing an invalid account name");

	/* Test 4 */
	result = modest_account_mgr_add_server_account (NULL,
							TEST_MODEST_ACCOUNT_NAME,
							"hostname",
							143,
							"username",
							"password",
							MODEST_PROTOCOLS_STORE_IMAP,
						       MODEST_PROTOCOLS_CONNECTION_NONE,
						       MODEST_PROTOCOLS_AUTH_NONE);
	fail_unless (!result,
		     "modest_account_mgr_add_server_account does not return " \
		     "FALSE when passing a NULL ModestAccountMgr");

	/* Test 5 */
	result = modest_account_mgr_add_server_account (account_mgr,
							NULL,
							"hostname",
							143,
							"username",
							"password",
							MODEST_PROTOCOLS_STORE_IMAP,
							MODEST_PROTOCOLS_CONNECTION_NONE,
							MODEST_PROTOCOLS_AUTH_NONE); 
	fail_unless (!result,
		     "modest_account_mgr_add_server_account does not return " \
		     "FALSE when passing a NULL account name");

	/* Test 6 */
 	result = modest_account_mgr_add_server_account (account_mgr, 
 							"ïnválid//accountñ//nÄméç",
 							"hostname", 
							143,
 							"username", 
 							"password", 
 							MODEST_PROTOCOLS_STORE_IMAP,
							MODEST_PROTOCOLS_CONNECTION_NONE,
							MODEST_PROTOCOLS_AUTH_NONE); 
 	fail_unless (!result, 
 		     "modest_account_mgr_add_server_account does not return " \
 		     "FALSE when passing an invalid account name"); 

	/* Test 7 */
	result = modest_account_mgr_remove_account (account_mgr,
						    "a_name_that_does_not_exist");
	fail_unless (!result,
		     "modest_account_mgr_remove_acccount does not return FALSE " \
		     "when trying to remove an account that does not exist");

	/* Test 9 */
	result = modest_account_mgr_remove_account (NULL,
						    TEST_MODEST_ACCOUNT_NAME);
	fail_unless (!result,
		     "modest_account_mgr_remove_acccount does not return " \
		     "FALSE when passing a NULL ModestAccountMgr");

	/* Test 10 */
	result = modest_account_mgr_remove_account (account_mgr,
						    NULL);
	fail_unless (!result,
		     "modest_account_mgr_remove_acccount does not return " \
		     "FALSE when passing a NULL account name");

	/* Test 11 */
	result = modest_account_mgr_account_exists (NULL,
						    TEST_MODEST_ACCOUNT_NAME,
						    TRUE);
	fail_unless (!result,
		     "modest_account_mgr_exists_account does not return " \
		     "FALSE when passing a NULL ModestAccountMgr");

	/* Test 12 */
	result = modest_account_mgr_account_exists (NULL,
						    TEST_MODEST_ACCOUNT_NAME,
						    FALSE);
	fail_unless (!result,
		     "modest_account_mgr_exists_account does not return " \
		     "FALSE when passing a NULL ModestAccountMgr");

	/* Test 13 */
	result = modest_account_mgr_account_exists (account_mgr,
						    NULL,
						    FALSE);
	fail_unless (!result,
		     "modest_account_mgr_exists_acccount does not return " \
		     "FALSE when passing a NULL account name");

	/* Test 14 */
	result = modest_account_mgr_account_exists (account_mgr,
						    NULL,
						    TRUE);
	fail_unless (!result,
		     "modest_account_mgr_exists_account does not return " \
		     "FALSE when passing a NULL server account name");
}
END_TEST

/* ------------------- Suite creation ------------------- */

static Suite*
account_mgr_suite (void)
{
	Suite *suite = suite_create ("ModestAccountMgr");
	TCase *tc = NULL;

	/* Tests case for "add/exists/remove account" */
	tc = tcase_create ("add_exists_remove_account");
	tcase_add_checked_fixture (tc, 
				     fx_setup_default_account_mgr, NULL);
	tcase_add_test (tc, test_add_exists_remove_account_regular);
	tcase_add_test (tc, test_add_exists_remove_account_invalid);
	suite_add_tcase (suite, tc);

	return suite;
}

/* --------------------- Main program ------------------- */

gint
main ()
{
	SRunner *srunner;
	Suite   *suite;
	int     failures;

	suite   = account_mgr_suite ();
	srunner = srunner_create (suite);
	
	srunner_run_all (srunner, CK_ENV);
	failures = srunner_ntests_failed (srunner);
	srunner_free (srunner);
	
	return failures;
}
