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

#include <glib.h>
#include <glib-object.h>

#define MODEST_DEBUG "MODEST_DEBUG"

typedef enum {
	MODEST_DEBUG_ABORT_ON_WARNING      = 1 << 0,
	MODEST_DEBUG_LOG_ACTIONS           = 1 << 1, /* not in use atm */
	MODEST_DEBUG_DEBUG_OBJECTS         = 1 << 2, /* for g_type_init */
	MODEST_DEBUG_DEBUG_SIGNALS         = 1 << 3  /* for g_type_init */
} ModestDebugFlags;


/**
 * modest_debug_get_flags 
 *
 * get the debug flags for modest; they are read from the MODEST_DEBUG
 * environment variable; the flags specified as strings, separated by ':'.
 * Possible values are:
 * - "abort-on-warning": abort the program when a gtk/glib/.. warning occurs.
 * useful when running in debugger
 * - "log-actions": log user actions (not in use atm)
 * - "track-object": track the use of (g)objects in the program. this option influences
 *  g_type_init_with_debug_flags
 *  - "track-signals": track the use of (g)signals in the program. this option influences
 *  g_type_init_with_debug_flags
 * if you would want to track signals and log actions, you could do something like:
 *  MODEST_DEBUG="log-actions:track-signals" ./modest
 * NOTE that the flags will stay the same during the run of the program, even
 * if the environment variable changes.
 * 
 * Returns: the bitwise OR of the debug flags
 */
ModestDebugFlags modest_debug_get_flags  (void) G_GNUC_CONST;

/**
 * modest_g_type_init
 *
 * do a g_type_init_with_debug_flags based on the modest debug flags
 */
void modest_debug_g_type_init (void);

/**
 * modest_g_type_init
 *
 * do set the logging based on the modest debug flags (ie. whether
 * we should abort when a warning occurs.
 */
void modest_debug_logging_init (void);
