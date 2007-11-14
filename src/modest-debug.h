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

#ifndef __MODEST_DEBUG_H__
#define __MODEST_DEBUG_H__

#include "modest-runtime.h"

/* some debugging macros */

/**
 * MODEST_RUNTIME_VERIFY_OBJECT_LAST_REF:
 * @OBJ: some (GObject) ptr
 * @NAME: name of @OBJ
 * 
 * macro to check whether @obj holds only one more ref (ie. after the
 * next unref it will die)
 * 
 * not, a g_warning will be issued on stderr. NOTE: this is only active
 * when MODEST_DEBUG contains "debug-objects".
 *
 ***/
#define MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(OBJ,name)			                               \
	do {								                               \
		if (modest_runtime_get_debug_flags() & MODEST_RUNTIME_DEBUG_OBJECTS)                   \
			if (G_IS_OBJECT(OBJ) && G_OBJECT(OBJ)->ref_count != 1)			       \
				g_warning ("%s:%d: %s ("	                                       \
					   #OBJ ") still holds a ref count of %d",                     \
					   __FILE__,__LINE__,name, G_OBJECT(OBJ)->ref_count);          \
	} while (0)




/**
 * MODEST_DEBUG_BLOCK:
 * @BLOCK: some block of code
 * 
 * macro to which run its argument (block) only when MODEST_DEBUG contains "debug-code"
 * 
 ***/
#define MODEST_DEBUG_BLOCK(BLOCK)				                                       \
	do {								                               \
		if (modest_runtime_get_debug_flags() & MODEST_RUNTIME_DEBUG_CODE)   {                  \
			BLOCK						                               \
		}                                                                                      \
        } while (0)                                                                                    


/**
 * MODEST_DEBUG_NOT_IMPLEMENTED:
 * @WIN: the parent GtkWindow, or NULL
 *
 * give a not-implemented-yet warning popup or g_warning
 *
 ***/
#define MODEST_DEBUG_NOT_IMPLEMENTED(WIN)    \
	do {				       \
		if (gtk_main_level() > 0) {    \
			GtkWidget *popup;      \
			popup = gtk_message_dialog_new (WIN,\
							GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,\
							GTK_MESSAGE_WARNING, \
							GTK_BUTTONS_OK,	\
							"Not yet implemented");\
			gtk_dialog_run (GTK_DIALOG(popup));		\
			gtk_widget_destroy (popup);			\
		} else							\
			g_warning ("%s:%d: Not yet implemented",__FILE__,__LINE__); \
	} while (0)							\
									

#endif /*__MODEST_DEBUG_H__*/
