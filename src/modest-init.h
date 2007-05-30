/* Copyright (c) 2006, 2007 Nokia Corporation
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

#ifndef __MODEST_INIT_H__
#define __MODEST_INIT_H__

#include <glib.h>
#include <glib-object.h>
#include <modest-runtime.h>

G_BEGIN_DECLS

/**
 * modest_init_init_core:
 *
 * initialize the modest runtime system (which sets up the
 * environment, instantiates singletons and so on)
 * modest_runtime_init should only be called once, and
 * when done with it, modest_runtime_uninit should be called
 *  
 * TRUE if this succeeded, FALSE otherwise.
 */
gboolean modest_init_init_core (void);


/**
 * modest_init_init_ui:
 * @argc: the #argc argument to the main function
 * @argv: the #argv argument to the main function
 * 
 * initialize the modest UI; this replaces the call to
 * gtk_init
 *  
 * TRUE if this succeeded, FALSE otherwise.
 */
gboolean modest_init_init_ui (gint argc, gchar** argv);

/**
 * modest_init_uninit:
 *
 * uninitialize the modest runtime system; free all the
 * resources and so on.
 *
 * TRUE if this succeeded, FALSE otherwise
 */
gboolean modest_init_uninit (void);

/**
 * modest_init_local_folders:
 * @location_filepath: The location at which the local-folders directory should be created, 
 * or NULL to specify $HOME.
 * 
 * create the Local Folders folder under cache, if they
 * do not exist yet.
 * 
 * Returns: TRUE if the folder were already there, or
 * they were created, FALSE otherwise
 */
gboolean modest_init_local_folders  (const gchar* location_filepath);

/**
 * modest_init_one_local_folder:
 *
 * Create the directory structure for a maildir folder,
 * so that camel can use it as a maildir folder in a 
 * local maildir store account.
 */
gboolean modest_init_one_local_folder (gchar *maildir_path);


G_END_DECLS

#endif /*__MODEST_INIT_H__*/
