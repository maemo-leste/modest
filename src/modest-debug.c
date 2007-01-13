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

#include <modest-debug.h>

/* http://primates.ximian.com/~federico/news-2006-04.html#memory-debugging-infrastructure*/
ModestDebugFlags
modest_debug_get_debug_flags ()
{
	GDebugKey debug_keys[] = {
		{ "abort-on-warning", MODEST_DEBUG_ABORT_ON_WARNING },
		{ "log-actions",      MODEST_DEBUG_LOG_ACTIONS },
		{ "debug-objects",    MODEST_DEBUG_DEBUG_OBJECTS },
		{ "debug-signals",    MODEST_DEBUG_DEBUG_SIGNALS }
	};
	const gchar *str;
	ModestDebugFlags debug_flags = -1;

	if (debug_flags != -1)
		return debug_flags;
	
	str = g_getenv (MODEST_DEBUG);
	
	if (str != NULL)
		debug_flags = g_parse_debug_string (str, debug_keys, G_N_ELEMENTS (debug_keys));
	else
		debug_flags = 0;
	
	return debug_flags;
}


void
modest_debug_g_type_init (void)
{
	GTypeDebugFlags gflags;
	ModestDebugFlags mflags;

	gflags = 0;
	mflags = modest_debug_get_debug_flags ();

	if (mflags & MODEST_DEBUG_DEBUG_OBJECTS)
		gflags |= G_TYPE_DEBUG_OBJECTS;
	if (mflags & MODEST_DEBUG_DEBUG_SIGNALS)
		gflags |= G_TYPE_DEBUG_SIGNALS;
	
	g_type_init_with_debug_flags (gflags);

}

void
modest_debug_logging_init (void)
{
	ModestDebugFlags mflags;
	mflags = modest_debug_get_debug_flags ();
	
	if (mflags & MODEST_DEBUG_ABORT_ON_WARNING)
		g_log_set_always_fatal (G_LOG_LEVEL_ERROR |
					G_LOG_LEVEL_CRITICAL |
					G_LOG_LEVEL_WARNING);
}
