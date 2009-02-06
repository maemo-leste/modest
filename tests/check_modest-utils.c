/* Copyright (c) 2009, Nokia Corporation
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
#include <modest-defs.h>
#include <gtk/gtk.h>
#include <string.h>
#include <modest-init.h>
#include <modest-utils.h>

static void
fx_setup_modest_utils ()
{
	fail_unless (gtk_init_check (NULL, NULL));

	fail_unless (g_setenv (MODEST_DIR_ENV, ".modesttest", TRUE));
	fail_unless (g_setenv (MODEST_NAMESPACE_ENV, "/apps/modesttest", TRUE));

	fail_unless (modest_init (0, NULL), "Failed running modest_init");

}

START_TEST (test_modest_utils_folder_writable)
{
	gboolean result;
	const gchar *path;

	result = modest_utils_folder_writable(NULL);
	fail_unless (result == FALSE,
		     "modest_utils_folder_writeable should return FALSE for NULL");

	path = g_getenv ("HOME");
	result = modest_utils_folder_writable(path);
	fail_unless (result == TRUE,
		     "modest_utils_folder_writeable should return TRUE for HOME");
}
END_TEST

START_TEST (test_modest_utils_file_exists)
{
	gboolean result;

	result = modest_utils_file_exists(NULL);
	fail_unless (result == FALSE,
		     "modest_utils_file_exists should return FALSE for NULL");

	result = modest_utils_file_exists("/etc/host.conf");
	fail_unless (result == TRUE,
		     "modest_utils_file_exists should return FALSE for hostname");

	result = modest_utils_file_exists("/etc/does-not-exist");
	fail_unless (result == FALSE,
		     "modest_utils_folder_writeable should return FALSE for /");

}
END_TEST


static Suite*
modest_utils_suite (void)
{
	Suite *suite = suite_create ("ModestUtils");

	TCase *tc_core = tcase_create ("core");
	tcase_add_checked_fixture (tc_core,
				   fx_setup_modest_utils,
				   NULL);
	tcase_add_test (tc_core, test_modest_utils_folder_writable);
	tcase_add_test (tc_core, test_modest_utils_file_exists);

	suite_add_tcase (suite, tc_core);

	return suite;
}


int
main ()
{
	SRunner *srunner;
	Suite   *suite;
	int     failures;
	
	g_type_init();

	suite   = modest_utils_suite ();
	srunner = srunner_create (suite);

	srunner_run_all (srunner, CK_ENV);
	failures = srunner_ntests_failed (srunner);
	srunner_free (srunner);
	
	return failures;
}
