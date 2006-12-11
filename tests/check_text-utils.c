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
#include <modest-text-utils.h>

typedef struct {
	const gchar *original;
	const gchar *expected;
} StringPair;

START_TEST (test_display_address)
{
	int i;
	const StringPair  tests[] = {
		{ "John Doe <foo@bar>", "John Doe" },
		{ "Rupert Griffin (test)", "Rupert Griffin"},
		{ "Hyvää päivää    ", "Hyvää päivää"},
	};

	fail_unless (modest_text_utils_display_address (NULL) == NULL,
		     "modest_text_utils_display_address(NULL) should be NULL");
	
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		gchar *str = g_strdup (tests[i].original);
		str = modest_text_utils_display_address (str);
		fail_unless (str && strcmp(str, tests[i].expected) == 0,
			"modest_text_utils_display_address failed for '%s': "
			     "expected '%s' but got '%s'",
			     tests[i].original, tests[i].expected, str);
		g_free (str);
	}
}
END_TEST

START_TEST (test_derived_address)
{
	int i;
	const gchar *prefix="Re:";
	
	const StringPair  tests[] = {
		{ "subject", "Re: subject" },
		{ NULL,      "Re:"},
		{ "Hyvää päivää", "Re: Hyvää päivää"},
	};

	fail_unless (modest_text_utils_derived_subject (NULL, NULL) == NULL,
		     "modest_text_utils_derived_subject (NULL,NULL) should be NULL");
	
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		gchar *str = g_strdup (tests[i].original);
		str = modest_text_utils_derived_subject (str, prefix);
		fail_unless (str && strcmp(str, tests[i].expected) == 0,
			"modest_text_utils_derived_subject failed for '%s': "
			     "expected '%s' but got '%s'",
			     tests[i].original, tests[i].expected, str);
		g_free (str);
	}
}
END_TEST






static Suite*
text_utils_suite (void)
{
	Suite *suite = suite_create ("ModestTextUtils");

	TCase *tc_core = tcase_create ("modest");
	tcase_add_test (tc_core, test_display_address);
	tcase_add_test (tc_core, test_derived_address);
	
	suite_add_tcase (suite, tc_core);

	return suite;
}


int
main ()
{
	SRunner *srunner;
	Suite   *suite;
	int     failures;

	suite   = text_utils_suite ();
	srunner = srunner_create (suite);
	
	srunner_run_all (srunner, CK_NORMAL);
	failures = srunner_ntests_failed (srunner);
	srunner_free (srunner);
	
	return failures;
}
