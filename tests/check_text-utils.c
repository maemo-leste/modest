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
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <modest-text-utils.h>

typedef struct {
	const gchar *original;
	const gchar *expected;
} StringPair;

/* ----------------- display address tests -------------- */

/**
 * Test regular usage of modest_text_utils_get_display_address
 *  - Test 1: Check "<...>" deletion
 *  - Test 2: Check "(...)" deletion
 *  - Test 3: Check address left and right trim, also non ASCII chars
 *  - Test 4: Check issues in tests 1, 2 and 3 together
 *  - Test 5: Check with an empty address
 */
START_TEST (test_get_display_address_regular)
{
	gint i;
	const StringPair tests[] = {
		{ "John Doe <foo@bar>", "John Doe" },
		{ "Rupert Griffin (test)", "Rupert Griffin"},
		{ "    Hyvää päivää    ", "Hyvää päivää"},
		{ "  John Doe  <foo@bar>  (test)  ", "John Doe" },
		{ "", "" },
	};

	/* Tests 1, 2, 3, 4 */
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		gchar *str = g_strdup (tests[i].original);
		str = modest_text_utils_get_display_address (str);
		fail_unless (str && strcmp(str, tests[i].expected) == 0,
			"modest_text_utils_get_display_address failed for '%s': "
			     "expected '%s' but got '%s'",
			     tests[i].original, tests[i].expected, str);
		g_free (str);
	}
}
END_TEST


/**
 * Test invalid usage of modest_text_utils_get_display_address
 *  - Test 1: Check with NULL address (should return NULL)
 */
START_TEST (test_get_display_address_invalid)
{
	/* Test 1 */
	fail_unless (modest_text_utils_get_display_address (NULL) == NULL,
		     "modest_text_utils_get_display_address(NULL) should be NULL");
}
END_TEST

/* ----------------- derived address tests -------------- */

/**
 * Test regular usage of modest_text_utils_derived_subject
 *  - Test 1: Check with a normal subject
 *  - Test 2: Test with NULL subject
 *  - Test 3: Test with non ASCII chars
 *  - Test 4: Test with an empty subject
 */
START_TEST (test_derived_subject_regular)
{
	gint i;
	const gchar *prefix="Re:";
	const StringPair tests[] = {
		{ "subject", "Re: subject" },
		{ NULL,      "Re:"},
		{ "Hyvää päivää", "Re: Hyvää päivää"},
		{ "", "Re: "},
	};

	/* Tests 1, 2, 3, 4 */
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		gchar *orig = g_strdup (tests[i].original);
		gchar *str = modest_text_utils_derived_subject (orig, prefix);
		fail_unless (str && strcmp(str, tests[i].expected) == 0,
			"modest_text_utils_derived_subject failed for '%s': "
			     "expected '%s' but got '%s'",
			     tests[i].original, tests[i].expected, str);
		g_free (orig);
		g_free (str);
	}
}
END_TEST

/**
 * Test invalid usage of modest_text_utils_derived_subject
 *  - Test 1: Check with NULL prefix (should return NULL)
 */
START_TEST (test_derived_subject_invalid)
{
	/* Test 1 */
	fail_unless (modest_text_utils_derived_subject (NULL, NULL) == NULL,
		     "modest_text_utils_derived_subject (*,NULL) should be NULL");
}
END_TEST

/* --------------------- quote tests -------------------- */

/**
 * Test regular usage of modest_text_utils_quote
 *  - Test 1: Quote empty text
 *  - Test 2: Quote 1 line of plain text 
 *  - Test 3: Quote several lines of plain text 
 *  - Test 4: Quote non ASCII chars
 *  - Test 5: Quote with a limit lower than the any word in the text
 *  - TODO: Test HTML quotation once its implementation if finished
 */
START_TEST (test_quote_regular)
{
	gint i;
	gchar *text = NULL;
	gchar *expected_quoted_text = NULL;
	gchar *quoted_text = NULL;

	/* Tests 1, 2, 3 */
	const StringPair tests[] = {
		{ "", 
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n> \n" },
		{ "This text should be quoted", 
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n> This text\n> should be quoted\n> \n" },
		{ "These are several\nlines\nof plain text", 
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n> These are\n> several lines\n> of plain text\n" },
		{ "áéíÍÓÚäëïÏÖÜñÑçÇŽÊîš", 
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n> áéíÍÓÚäëïÏÖÜñÑçÇŽÊîš\n" },
	};

	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		text = g_strdup (tests[i].original);
		expected_quoted_text = g_strdup (tests[i].expected);
		quoted_text = modest_text_utils_quote (text, "text/plain", "foo@bar",  0, 15);
		fail_unless (quoted_text && strcmp(expected_quoted_text, quoted_text)==0,
			     "modest_text_utils_quote failed:\nOriginal text:\n\"%s\"\n" \
			     "Expected quotation:\n\"%s\"\nQuoted text:\n\"%s\"",
			     text, expected_quoted_text, quoted_text);
		g_free (text);
		g_free (expected_quoted_text);
		g_free (quoted_text);
	}

	/* Test 5 */
	text = g_strdup ("Quotation test example");
	expected_quoted_text = 
		g_strdup ("On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n> Quotation\n> test\n> example\n> \n");
	quoted_text = modest_text_utils_quote (text, "text/plain", "foo@bar",  0, 1);
	fail_unless (quoted_text && !strcmp (expected_quoted_text, quoted_text),
		     "modest_text_utils_quote failed:\nOriginal text:\n\"%s\"\n" \
		     "Expected quotation:\n\"%s\"\nQuoted text:\n\"%s\"",
		     text, expected_quoted_text, quoted_text);
	g_free (text);
	g_free (expected_quoted_text);
	g_free (quoted_text);
}
END_TEST

/**
 * Test invalid usage of modest_text_utils_quote
 *  - Test 1: Check NULL text (should return NULL)
 *  - Test 2: Check NULL content_type (should quote as text/plain)
 *  - Test 3: Check NULL address (should cite address as "(null)")
 *  - Test 4: Check negative limit (should write each word in its own line)
 *  - TODO: Repeat tests for HTML quotation when its implementation is finished
 */
START_TEST (test_quote_invalid)
{
	gchar *text = NULL;
	gchar *expected_quoted_text = NULL;
	gchar *quoted_text = NULL;

	/* Test 1 (Fault) */
	text = NULL;
	quoted_text = modest_text_utils_quote (NULL, "text/plain", "foo@bar",  0, 15);
	fail_unless (quoted_text == NULL,
		     "modest_text_utils_quote failed:\nOriginal text: NULL\n" \
		     "Expected quotation: NULL\nQuoted text: \"%s\"",
		     quoted_text);
	g_free (quoted_text);
	
	
	/* Test 2 (Fault) */
	text = g_strdup ("Text");
	expected_quoted_text = g_strdup ("On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n> Text\n");
	quoted_text = modest_text_utils_quote (text, NULL, "foo@bar",  0, 15);
	fail_unless (quoted_text == NULL,
		     "modest_text_utils_quote failed:\nOriginal text: NULL\n" \
		     "Expected quotation: NULL\nQuoted text: \"%s\"",
		     quoted_text);
	g_free (text);
	g_free (expected_quoted_text);
	g_free (quoted_text);

	/* Test 3 */
	text = g_strdup ("Text");
	expected_quoted_text = g_strdup ("On Thu Jan  1 01:00:00 1970, (null) wrote:\n> Text\n");
	quoted_text = modest_text_utils_quote (text, "text/plain", NULL,  0, 15);
	fail_unless (quoted_text == NULL,
		     "modest_text_utils_quote failed:\nOriginal text: NULL\n" \
		     "Expected quotation: NULL\nQuoted text: \"%s\"",
		     quoted_text);
	g_free (text);
	g_free (expected_quoted_text);
	g_free (quoted_text);

	/* Test 4 */
	text = g_strdup ("This is a text");
	expected_quoted_text = g_strdup ("On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n> This\n> is\n> a\n> text\n> \n");
	quoted_text = modest_text_utils_quote (text, "text/plain", "foo@bar",  0, 0);
	fail_unless (quoted_text && !strcmp (expected_quoted_text, quoted_text),
		     "modest_text_utils_quote failed:\nOriginal text:\n\"%s\"\n" \
		     "Expected quotation:\n\"%s\"\nQuoted text:\n\"%s\"",
		     text, expected_quoted_text, quoted_text);
	g_free (text);
	g_free (expected_quoted_text);
	g_free (quoted_text);
}
END_TEST

/* ---------------------- cite tests -------------------- */

/**
 * Test regular usage of modest_text_utils_cite
 *  - Test 1: Check cite of an empty text
 *  - Test 2: Check cite of a line of text
 *  - Test 3: Check cite of several lines of text
 *  - Test 4: Check with non ASCII chars
 */
START_TEST (test_cite_regular)
{
	gint i;
	gchar *cited_text = NULL;
	const StringPair tests[] = {
		{ "", 
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n\n" },
		{ "This is some text", 
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\nThis is some text\n" },
		{ "This\nis some\ntext", 
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\nThis\nis some\ntext\n" },
		{ "áéíÍÓÚäëïÏÖÜñÑçÇŽÊîš",
		  "On Thu Jan  1 01:00:00 1970, foo@bar wrote:\náéíÍÓÚäëïÏÖÜñÑçÇŽÊîš\n" },
	};

	/* Tests 1, 2, 3, 4 */
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		cited_text = modest_text_utils_cite (tests[i].original, "text/plain", "foo@bar", 0);
		fail_unless (cited_text && !strcmp (tests[i].expected, cited_text),
			     "modest_text_utils_cite failed:\nOriginal text:\n\"%s\"\n" \
			     "Expected cite:\n\"%s\"\nCite obtained:\n\"%s\"",
			     tests[i].original, tests[i].expected, cited_text);
		g_free (cited_text);
	}
}
END_TEST

/**
 * Test invalid usage of modest_text_utils_cite
 *  - Test 1: Check NULL text (should cite text as (null))
 *  - Test 2: Check NULL address (should cite address as (null)
 *  TODO: add tests for content_type once it is used in the implementation
 */
START_TEST (test_cite_invalid)
{
	gchar *text = NULL;
	gchar *cited_text = NULL;
	gchar *expected_cite = NULL;

	/* Test 1 */
	text = NULL;
	expected_cite = g_strdup ("On Thu Jan  1 01:00:00 1970, foo@bar wrote:\n(null)\n");
	cited_text = modest_text_utils_cite (text, "text/plain", "foo@bar", 0);
	fail_unless (cited_text && !strcmp (expected_cite, cited_text),
		     "modest_text_utils_cite failed:\nOriginal text:\nNULL\n" \
		     "Expected cite:\n\"%s\"\nCite obtained:\n\"%s\"",
		     expected_cite, cited_text);
	g_free (expected_cite);
	g_free (cited_text);

	/* Test 2 */
	text = g_strdup ("This is some text");
	expected_cite = g_strdup ("On Thu Jan  1 01:00:00 1970, (null) wrote:\nThis is some text\n");
	cited_text = modest_text_utils_cite (text, "text/plain", NULL, 0);
	fail_unless (cited_text && !strcmp (expected_cite, cited_text),
		     "modest_text_utils_cite failed:\nOriginal text:\n\"%s\"\n" \
		     "Expected cite:\n\"%s\"\nCite obtained:\n\"%s\"",
		     text, expected_cite, cited_text);
	g_free (text);
	g_free (expected_cite);
	g_free (cited_text);
}
END_TEST

/* -------------------- inline tests -------------------- */

/**
 * Test regular usage of modest_text_utils_inline
 *  - Test 1: Check inline of an empty text
 *  - Test 2: Check inline of a regular text
 *  - Test 3: Check with non ASCII chars
 *  TODO: add tests for HTML when implementation is finished
 */
START_TEST (test_inline_regular)
{
	gint i;
	gchar *inlined_text = NULL;
	const StringPair tests[] = {
		{ "", 
		  "-----Forwarded Message-----\n" \
		  "From: foo@bar\n" \
		  "Sent: Thu Jan  1 01:00:00 1970\n" \
		  "To: bar@foo\n" \
		  "Subject: Any subject\n\n" },
		{ "Some text\nto inline", 
		  "-----Forwarded Message-----\n" \
		  "From: foo@bar\n" \
		  "Sent: Thu Jan  1 01:00:00 1970\n" \
		  "To: bar@foo\n" \
		  "Subject: Any subject\n\n" \
		  "Some text\nto inline" },
		{ "áéíÍÓÚäëïÏÖÜñÑçÇŽÊîš", 
		  "-----Forwarded Message-----\n" \
		  "From: foo@bar\n" \
		  "Sent: Thu Jan  1 01:00:00 1970\n" \
		  "To: bar@foo\n" \
		  "Subject: Any subject\n\n" \
		  "áéíÍÓÚäëïÏÖÜñÑçÇŽÊîš" },
	};

	/* Tests 1, 2, 3 */
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		inlined_text = 	modest_text_utils_inline (tests[i].original, 
							  "text/plain", 
							  "foo@bar", 
							  0, 
							  "bar@foo", 
							  "Any subject");
		fail_unless (inlined_text && !strcmp (tests[i].expected, inlined_text),
			     "modest_text_utils_inline failed:\nOriginal text:\n\"%s\"\n" \
			     "Expected inline:\n\"%s\"\nInline obtained:\n\"%s\"",
			     tests[i].original, tests[i].expected, inlined_text);		
		g_free (inlined_text);
	}
}
END_TEST

/**
 * Test invalid usage of modest_text_utils_inline
 *  - Test 1: Check NULL text (should inline text as (null))
 *  - Test 2: Check NULL content_type (should assume text/plain)
 *  - Test 3: Check NULL from (should write "(null)")
 *  - Test 4: Check NULL to (should write ("null"))
 *  - Test 5: Check NULL subject (should write ("null"))
 * TODO: repeat tests from HTML when implemented
 */
START_TEST (test_inline_invalid)
{
	gchar *text = NULL;
	gchar *expected_inline = NULL;
	gchar *inlined_text = NULL;

	/* Test 1 */
	expected_inline = g_strdup("-----Forwarded Message-----\n" \
				   "From: foo@bar\n" \
				   "Sent: Thu Jan  1 01:00:00 1970\n" \
				   "To: bar@foo\n" \
				   "Subject: Any subject\n\n" \
				   "(null)");
	inlined_text = 	modest_text_utils_inline (NULL, 
						  "text/plain", 
						  "foo@bar", 
						  0, 
						  "bar@foo", 
						  "Any subject");
	fail_unless (inlined_text == NULL, 
		     "modest_text_utils_inline failed: it should return NULL");
	g_free (inlined_text);

	/* Test 2 (Fault) */
	text = g_strdup ("Some text");
	expected_inline = g_strdup("-----Forwarded Message-----\n" \
				   "From: foo@bar\n" \
				   "Sent: Thu Jan  1 01:00:00 1970\n" \
				   "To: bar@foo\n" \
				   "Subject: Any subject\n\n" \
				   "Some text");
	inlined_text = 	modest_text_utils_inline (text,
						  NULL,
						  "foo@bar",
						  0,
						  "bar@foo",
						  "Any subject");
	fail_unless (inlined_text == NULL, 
		     "modest_text_utils_inline failed: it should return NULL");
	g_free (inlined_text);

	/* Test 3 */
	text = g_strdup ("Some text");
	expected_inline = g_strdup("-----Forwarded Message-----\n" \
				   "From: (null)\n" \
				   "Sent: Thu Jan  1 01:00:00 1970\n" \
				   "To: bar@foo\n" \
				   "Subject: Any subject\n\n" \
				   "Some text");
	inlined_text = 	modest_text_utils_inline (text, 
						  "text/plain", 
						  NULL, 
						  0, 
						  "bar@foo", 
						  "Any subject");
	fail_unless (inlined_text == NULL, 
		     "modest_text_utils_inline failed: it should return NULL");

	g_free (inlined_text);

	/* Test 4 */
	text = g_strdup ("Some text");
	expected_inline = g_strdup("-----Forwarded Message-----\n" \
				   "From: foo@bar\n" \
				   "Sent: Thu Jan  1 01:00:00 1970\n" \
				   "To: (null)\n" \
				   "Subject: Any subject\n\n" \
				   "Some text");
	inlined_text = 	modest_text_utils_inline (text, 
						  "text/plain", 
						  "foo@bar", 
						  0, 
						  NULL, 
						  "Any subject");
	fail_unless (inlined_text == NULL, 
		     "modest_text_utils_inline failed: it should return NULL");
	g_free (inlined_text);
	
	/* Test 5 */
	text = g_strdup ("Some text");
	expected_inline = g_strdup("-----Forwarded Message-----\n" \
				   "From: foo@bar\n" \
				   "Sent: Thu Jan  1 01:00:00 1970\n" \
				   "To: bar@foo\n" \
				   "Subject: (null)\n\n" \
				   "Some text");
	inlined_text = 	modest_text_utils_inline (text, 
						  "text/plain", 
						  "foo@bar", 
						  0, 
						  "bar@foo", 
						  NULL);
	fail_unless (inlined_text == NULL, 
		     "modest_text_utils_inline failed: it should return NULL");

	g_free (inlined_text);
}
END_TEST

/* ---------------- remove address tests ---------------- */

/**
 * Test regular usage of modest_text_utils_remove_address
 *  - Test 1: Remove a single address in the middle of a list
 *  - Test 2: Remove an extended address in the middle of a list
 *  - Test 3: Remove an address that appears more than once in the list
 *  - Test 4: Remove address from an empty list
 *  - Test 5: Check with non ASCII characters
 */
START_TEST (test_remove_address_regular)
{
	gchar *list = NULL;
	gchar *new_list = NULL;
	gchar *address = NULL;

	/* Test 1 */
	list = g_strdup ("foo@bar <foo>, myname@mycompany.com (myname), " \
			 "xxx@yyy.zzz (xxx yyy zzz), bar@foo");
	address = g_strdup ("xxx@yyy.zzz");
	new_list = modest_text_utils_remove_address (list, address);

	fail_unless (new_list && strstr (new_list, "foo@bar <foo>") != NULL,
		     "modest_text_utils_remove_address failed: "\
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "myname@mycompany.com (myname)") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "bar@foo") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "xxx@yyy.zzz") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "(xxx yyy zzz)") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);

	g_free (list);
	g_free (new_list);
	g_free (address);

	/* Test 2 */
	list = g_strdup ("foo@bar <foo>, xxx@yyy.zzz (xxx yyy zzz), bar@foo");
	address = g_strdup ("xxx@yyy.zzz (xxx yyy zzz)");
	new_list = modest_text_utils_remove_address (list, address);

	fail_unless (new_list && strstr (new_list, "foo@bar <foo>") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "bar@foo") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "xxx@yyy.zzz") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "(xxx yyy zzz)") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);

	g_free (list);
	g_free (new_list);
	g_free (address);

	/* Test 3 */
	list = g_strdup ("foo@bar <foo>, bar@foo (BAR), xxx@yyy.zzz (xxx yyy zzz), bar@foo");
	address = g_strdup ("bar@foo");
	new_list = modest_text_utils_remove_address (list, address);

	fail_unless (new_list && strstr (new_list, "foo@bar <foo>") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "xxx@yyy.zzz (xxx yyy zzz)") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "bar@foo") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "(BAR)") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);

	g_free (list);
	g_free (new_list);
	g_free (address);

	/* Test 4 */
	list = g_strdup ("");
	address = g_strdup ("bar@foo");
	new_list = modest_text_utils_remove_address (list, address);

	fail_unless (new_list && !strcmp (new_list, list),
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);

	g_free (list);
	g_free (new_list);
	g_free (address);

	/* Test 5 */
	list = g_strdup ("foo@bar, áñï@Èž.com (Àç ÑŸž), bar@foo");
	address = g_strdup ("áñï@Èž.com");
	new_list = modest_text_utils_remove_address (list, address);

	fail_unless (new_list && strstr (new_list, "foo@bar") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "bar@foo") != NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "áñï@Èž.com") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);
	fail_unless (new_list && strstr (new_list, "(Àç ÑŸž)") == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing \"%s\" from address list \"%s\" returns \"%s\"",
		     address, list, new_list);

	g_free (list);
	g_free (new_list);
	g_free (address);
}
END_TEST

/**
 * Test limits usage of modest_text_utils_remove_address
 *  - Test 1: Remove address at the beginning of the list
 *  - Test 2: Remove address at the end  of the list
 *  - Test 3: Remove the address of a one-element-size list
 */
START_TEST (test_remove_address_limits)
{
	gchar *list = NULL;
	gchar *new_list = NULL;
	gchar *expected_list = NULL;
	//gchar *address = NULL;
	gint i;

	const StringPair tests[] = {
		{ "foo@bar <FOO BAR>, bar@foo (BAR FOO)", "bar@foo (BAR FOO)" },
		{ "foo@bar <FOO BAR>, bar@foo (BAR FOO)", "foo@bar <FOO BAR>" },
		{ "foo@bar <FOO BAR>", "" },
	};
	const gchar *remove[] = { "foo@bar", "bar@foo", "foo@bar" };

	/* Tests 1, 2, 3 */
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		list = g_strdup (tests[i].original);
		expected_list = g_strdup (tests[i].expected);
		new_list = modest_text_utils_remove_address (list, remove[i]);
		fail_unless (new_list && !strcmp (expected_list, new_list),
			     "modest_text_utils_remove_address failed: " \
			     "Removing \"%s\" from address list \"%s\" returns \"%s\" instead of \"%s\"",
			     remove[i], list, new_list, expected_list);
		g_free (list);
		g_free (expected_list);
		g_free (new_list);
	}
}
END_TEST

/**
 * Test invalid usage of modest_text_utils_remove_address
 *  - Test 1: Remove a NULL address (should return the same list)
 *  - Test 2: Remove an address form a NULL list (should return NULL)
 */
START_TEST (test_remove_address_invalid)
{
	gchar *list = NULL;
	gchar *new_list = NULL;

	/* Test 1 (Fault) */
	list = g_strdup ("foo@bar, bar@foo");
	new_list = modest_text_utils_remove_address (list, NULL);
	fail_unless (new_list && !strcmp (list, new_list),
		     "modest_text_utils_remove_address failed: " \
		     "Removing a NULL address from \"%s\" returns \"%s\"",
		     list, new_list);
	g_free (list);
	g_free (new_list);

	/* Test 2 */
	new_list = modest_text_utils_remove_address (NULL, "foo@bar");
	fail_unless (new_list == NULL,
		     "modest_text_utils_remove_address failed: " \
		     "Removing an address from a NULL list does not return NULL");
	g_free (new_list);
}
END_TEST

/* --------------- convert to html tests ---------------- */

/**
 * Test regular usage of modest_text_utils_convert_to_html
 *  - Test 1: Check conversion of a regular text
 *  - Test 2: Check conversion of a regular text with links
 *  - Test 3: Check conversion of a regular text with special html chars
 */
START_TEST (test_convert_to_html_regular)
{
	gchar *text_in_html;
	gchar *html_start = NULL;
	gchar *html_end = NULL;
	gchar *html = NULL;
	gint bytes;
	const StringPair tests[] = {
		{ "This is some text.", 
		  "<tt>This is some text.</tt>" },
		{ "http://xxx.yyy.zzz/myfile", 
		  "<tt><a href=\"http://xxx.yyy.zzz/myfile\">http://xxx.yyy.zzz/myfile</a></tt>" },
		{ "<a  &  b>\n\tx", 
		  "<tt>&lt;a &nbsp;&quot;&nbsp;&nbsp;b&gt;<br>\n&nbsp; &nbsp;&nbsp;x</tt>" },
	};

	/* Tests 1, 2, 3 */
	for (i = 0; i !=  sizeof(tests)/sizeof(StringPair); ++i) {
		html = modest_text_utils_convert_to_html (tests[i].original);
		fail_unless (html != NULL,
			     "modest_text_utils_convert_to_html failed:" \
			     "Original text:\n\"%s\"\nExpected html:\n\"%s\"\nObtained html:\nNULL",
			     tests[i].original, tests[i].expected);
		html_start = strstr (html, "<tt>");
		html_end = strstr (html, "</body>");
		bytes = html_end - html_start;
		text_in_html = g_strndup (html_start, bytes);
		
		fail_unless (strstr (html, tests[i].expected) != NULL,
			     "modest_text_utils_convert_to_html failed:" \
			     "Original text:\n\"%s\"\nExpected html:\n\"%s\"\nObtained html:\n\"%s\"",
			     tests[i].original, tests[i].expected, text_in_html);
//		g_free (html_start);
//		g_free (text_in_html);
//		g_free (html);
	}
}
END_TEST

/**
 * Test invalid usage of modest_text_utils_convert_to_html
 *  - Test 1: Check with NULL data
 */
START_TEST (test_convert_to_html_invalid)
{
	gchar *html = NULL;

	/* Test 1 */
	html = modest_text_utils_convert_to_html (NULL);
	fail_unless (html == NULL,
		     "modest_text_utils_convert_to_html failed:" \
		     "Does not return NULL when passed NULL data");
}
END_TEST


/* ------------------- Suite creation ------------------- */

static Suite*
text_utils_suite (void)
{
	Suite *suite = suite_create ("ModestTextUtils");
	TCase *tc = NULL;

	/* Tests case for "display adress" */
	tc = tcase_create ("display_adress");
	tcase_add_test (tc, test_get_display_address_regular);
	tcase_add_test (tc, test_get_display_address_invalid);
	suite_add_tcase (suite, tc);

	/* Test case for "derived subject" */
	tc = tcase_create ("derived_subject");
        tcase_add_test (tc, test_derived_subject_regular);
	tcase_add_test (tc, test_derived_subject_invalid);
	suite_add_tcase (suite, tc);

	/* Test case for "quote" */
	tc = tcase_create ("quote");
        tcase_add_test (tc, test_quote_regular);
	tcase_add_test (tc, test_quote_invalid);
	suite_add_tcase (suite, tc);

	/* Test case for "cite" */
	tc = tcase_create ("cite");
        tcase_add_test (tc, test_cite_regular);
	tcase_add_test (tc, test_cite_invalid);
	suite_add_tcase (suite, tc);

	/* Test case for "inline" */
	tc = tcase_create ("inline");
        tcase_add_test (tc, test_inline_regular);
	tcase_add_test (tc, test_inline_invalid);
	suite_add_tcase (suite, tc);

	/* Test case for "remove address" */
	tc = tcase_create ("remove_address");
        tcase_add_test (tc, test_remove_address_regular);
        tcase_add_test (tc, test_remove_address_limits);
	tcase_add_test (tc, test_remove_address_invalid);
	suite_add_tcase (suite, tc);

	/* Test case for "convert to html" */
	tc = tcase_create ("convert_to_html");
        tcase_add_test (tc, test_convert_to_html_regular);
	tcase_add_test (tc, test_convert_to_html_invalid);
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

	setenv ("TZ", "Europe/Paris", 1);
	
	suite   = text_utils_suite ();
	srunner = srunner_create (suite);
		
	srunner_run_all (srunner, CK_ENV);
	failures = srunner_ntests_failed (srunner);
	srunner_free (srunner);
	
	return failures;
}
