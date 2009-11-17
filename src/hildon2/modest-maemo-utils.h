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


#ifndef __MODEST_MAEMO_UTILS_H__
#define __MODEST_MAEMO_UTILS_H__

#include <gtk/gtk.h>
#include <stdio.h> /* for FILE* */
#include <tny-fs-stream.h>
#include <libosso.h>
#include "widgets/modest-global-settings-dialog.h"
#include "widgets/modest-validating-entry.h"
#include <hildon/hildon-gtk.h>

#define MODEST_MAEMO_UTILS_MYDOCS_ENV "MYDOCSDIR"
#define MODEST_MAEMO_UTILS_DEFAULT_IMAGE_FOLDER ".images"


/**
 * modest_maemo_utils_get_device_name
 *
 * get the name for this device. Note: this queries the bluetooth
 * name over DBUS, and may block. The result will be available in
 * MODEST_CONF_DEVICE_NAME in ModestConf; it will be updated when it
 * changes
 * 
 */
void modest_maemo_utils_get_device_name (void);


/**
 * modest_maemo_utils_setup_images_filechooser:
 * @chooser: a #GtkFileChooser
 *
 * Configures the default folder, and mime filter of a filechooser
 * for images.
 */
void modest_maemo_utils_setup_images_filechooser (GtkFileChooser *chooser);


/**
 * modest_maemo_utils_get_osso_context:
 *
 * get the osso_context pointer for this application
 * 
 * Return: the osso context pointer 
 */
osso_context_t *modest_maemo_utils_get_osso_context (void);

/**
 * modest_maemo_get_osso_context:
 *
 * retrieve the osso context for this application
 * 
 * Returns: the current osso_context_t ptr  
 */
osso_context_t* modest_maemo_utils_get_osso_context (void);

/**
 * modest_maemo_set_osso_context:
 *
 * remember the osso-context for this application 
 * 
 * @osso_context: a valid osso_context_t pointer
 *  
 */
void modest_maemo_utils_set_osso_context (osso_context_t *osso_context);

/**
 * modest_maemo_utils_get_manager_menubar_as_menu:
 * @manager: a #GtkUIManager
 * @item_name: a string
 *
 * obtains the node with name @item_name in @manager (which happens to be a menubar) as a
 * #GtkMenu.
 *
 * Returns: a #GtkMenu
 */
GtkWidget *modest_maemo_utils_get_manager_menubar_as_menu (GtkUIManager *manager, const gchar *item_name);

#ifdef MODEST_PLATFORM_MAEMO
/**
 * modest_maemo_utils_in_usb_mode:
 *
 * Check if the device is working in mass storage mode
 *
 * Returns: returns TRUE if the internal memory of the device is
 * working in mass storage mode
 **/
gboolean modest_maemo_utils_in_usb_mode ();
#endif

/**
 * modest_heartbeat_add:
 * @function: function to call
 * @userdata: data to pass to @function.
 *
 * Adds a function to be called when heartbeat is called. If the
 * function returns FALSE it is automatically removed from the
 * list of event sources and will not be called again.
 *
 * Returns: the ID (greater than 0) of the event source
 */
guint
modest_heartbeat_add (GSourceFunc function,
		      gpointer userdata);

#endif /*__MODEST_MAEMO_UTILS_H__*/
