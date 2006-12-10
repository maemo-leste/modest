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
#include <modest-conf.h>



START_TEST (test_modest_conf_new)
{
	ModestConf *conf = modest_conf_new ();
	fail_unless (MODEST_IS_CONF(conf),
		     "modest_conf_new should return a valid"
		     " ModestConf instance");
	g_object_unref (conf);
}
END_TEST



static Suite*
modest_conf_suite (void)
{
	Suite *suite = suite_create ("ModestConf");

	TCase *tc_core = tcase_create ("core");
	tcase_add_test (tc_core, test_modest_conf_new);

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

	suite   = modest_conf_suite ();
	srunner = srunner_create (suite);

	srunner_run_all (srunner, CK_NORMAL);
	failures = srunner_ntests_failed (srunner);
	srunner_free (srunner);
	
	return failures;
}
