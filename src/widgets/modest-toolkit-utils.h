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


#ifndef __MODEST_TOOLKIT_UTILS_H__
#define __MODEST_TOOLKIT_UTILS_H__

#include <gtk/gtk.h>
#include <stdio.h> /* for FILE* */
#include <tny-fs-stream.h>
#include "widgets/modest-global-settings-dialog.h"
#include "widgets/modest-validating-entry.h"

#ifdef MAEMO_CHANGES
#include <hildon/hildon-gtk.h>
#define ModestToolkitSizeType HildonSizeType
#else
#define ModestToolkitSizeType gint
#endif

GtkWidget *modest_toolkit_utils_create_captioned    (GtkSizeGroup *title_size_group,
						   GtkSizeGroup *value_size_group,
						   const gchar *title,
						   gboolean use_markup,
						   GtkWidget *control);
GtkWidget *modest_toolkit_utils_create_vcaptioned    (GtkSizeGroup *size_group,
						    const gchar *title,
						    gboolean use_markup,
						    GtkWidget *control);

GtkWidget *modest_toolkit_utils_create_captioned_with_size_type    (GtkSizeGroup *title_size_group,
								    GtkSizeGroup *value_size_group,
								    const gchar *title,
								    gboolean use_markup,
								    GtkWidget *control,
								    ModestToolkitSizeType size_type);

GtkWidget *modest_toolkit_utils_create_vcaptioned_with_size_type    (GtkSizeGroup *size_group,
								     const gchar *title,
								     gboolean use_markup,
								     GtkWidget *control,
								     ModestToolkitSizeType size_type);

void       modest_toolkit_utils_captioned_set_label (GtkWidget *captioned,
						     const gchar *new_label,
						     gboolean use_markup);

GtkWidget *modest_toolkit_utils_captioned_get_label_widget (GtkWidget *captioned);

void       modest_toolkit_utils_set_hbutton_layout (GtkSizeGroup *title_sizegroup, 
						    GtkSizeGroup *value_sizegroup,
						    const gchar *title, 
						    GtkWidget *button);
void       modest_toolkit_utils_set_vbutton_layout (GtkSizeGroup *sizegroup, 
						    const gchar *title, 
						    GtkWidget *button);

GtkWidget *modest_toolkit_utils_create_group_box (const gchar *label, GtkWidget *contents);

gboolean   modest_toolkit_utils_select_attachments (GtkWindow *window, TnyList *att_list, gboolean include_msgs);



#endif /*__MODEST_TOOLKIT_UTILS_H__*/
