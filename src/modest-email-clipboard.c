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

#include <modest-email-clipboard.h>
#include <modest-tny-folder.h>

/* 'private'/'protected' functions */
static void modest_email_clipboard_class_init (ModestEmailClipboardClass * klass);
static void modest_email_clipboard_init       (ModestEmailClipboard * obj);
static void modest_email_clipboard_finalize   (GObject * obj);

/* globals */
static GObjectClass *parent_class = NULL;

typedef struct _ModestEmailClipboardPrivate ModestEmailClipboardPrivate;
struct _ModestEmailClipboardPrivate {
	TnyList    *selection;
	TnyFolder  *src;	
	gchar      **hidding;
	gboolean   delete;
};


#define MODEST_EMAIL_CLIPBOARD_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
						MODEST_TYPE_EMAIL_CLIPBOARD, \
                                                ModestEmailClipboardPrivate))


GType
modest_email_clipboard_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof (ModestEmailClipboardClass),
			NULL,	/* base init */
			NULL,	/* base finalize */
			(GClassInitFunc) modest_email_clipboard_class_init,
			NULL,	/* class finalize */
			NULL,	/* class data */
			sizeof (ModestEmailClipboard),
			1,	/* n_preallocs */
			(GInstanceInitFunc) modest_email_clipboard_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestEmailClipboard",
						  &my_info, 0);
	}
	return my_type;
}



static void
modest_email_clipboard_class_init (ModestEmailClipboardClass * klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_email_clipboard_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof (ModestEmailClipboardPrivate));

	/* signal definitions */
}


static void
modest_email_clipboard_init (ModestEmailClipboard * obj)
{
	ModestEmailClipboardPrivate *priv =
		MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (obj);
	
	priv->src = NULL;
	priv->selection = NULL;
	priv->hidding = NULL;
	priv->delete = FALSE;
}

static void
modest_email_clipboard_finalize (GObject * obj)
{
	/* Clear objects stored on clipboard */
	modest_email_clipboard_clear (MODEST_EMAIL_CLIPBOARD(obj));		

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestEmailClipboard *
modest_email_clipboard_new ()
{
	GObject *obj;
	ModestEmailClipboardPrivate *priv;

	obj = G_OBJECT (g_object_new (MODEST_TYPE_EMAIL_CLIPBOARD, NULL));
	priv = MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (obj);
	
	return MODEST_EMAIL_CLIPBOARD (obj);
}


void
modest_email_clipboard_get_data (ModestEmailClipboard *self,
				 TnyFolder **src_folder,
				 TnyList **data,
				 gboolean *delete)
{
	ModestEmailClipboardPrivate *priv = NULL;;

	*src_folder = NULL;
	*data = NULL;
	*delete = FALSE;

	g_return_if_fail (MODEST_IS_EMAIL_CLIPBOARD (self));
	priv = MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (self);
	g_return_if_fail (TNY_IS_FOLDER (priv->src));

	/* if no data into clipboard, do nothing */
	if (modest_email_clipboard_cleared (self)) return;
		
	/* Get data stored on clipboard */
	*src_folder = g_object_ref(priv->src);
	if (priv->selection)
		*data = g_object_ref(priv->selection);
	*delete = priv->delete;

	/* Clear objects stored on clipboard */
	modest_email_clipboard_clear (MODEST_EMAIL_CLIPBOARD(self));		
}

void
modest_email_clipboard_set_data (ModestEmailClipboard *self,
				 TnyFolder *src_folder, 
				 TnyList *data,
				 gboolean delete)
{
	ModestEmailClipboardPrivate *priv = NULL;;
	TnyIterator *iter = NULL;
	GObject *obj = NULL;			
	gchar *id = NULL;
	guint i, n_selected;

	g_return_if_fail (MODEST_IS_EMAIL_CLIPBOARD (self));
	g_return_if_fail (TNY_IS_FOLDER (src_folder));
	priv = MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (self);

	/* if data into clipboard, clear them  */
	if (!modest_email_clipboard_cleared (self)) 
		modest_email_clipboard_clear (self);
	
	/* set new data */
	priv->src = g_object_ref (src_folder);
	if (data != NULL)
		priv->selection = g_object_ref (data);
	priv->delete = delete;
	priv->hidding = NULL;

	/* Fill hidding array (for cut operation) */	
	if (delete) {
		n_selected = 1;
		if (data != NULL) {
			n_selected = tny_list_get_length (data);	
			priv->hidding = g_malloc0(sizeof(gchar *) * n_selected);	
			iter = tny_list_create_iterator (priv->selection);
			i = 0;
			while (!tny_iterator_is_done (iter)) {
				obj = tny_iterator_get_current (iter);
				if (obj && TNY_IS_HEADER (obj))
					id = g_strdup(tny_header_get_message_id (TNY_HEADER (obj)));
				
				priv->hidding[i++] = id;
				tny_iterator_next (iter);
				
				if (obj)
					g_object_unref (obj);
			}
			g_object_unref (iter);
		}
		else {
			priv->hidding = g_malloc0(sizeof(gchar *));	
			id = g_strdup (tny_folder_get_id (src_folder));
			priv->hidding[0] = id;			
		}
	}
}

void
modest_email_clipboard_clear (ModestEmailClipboard *self)
{
	ModestEmailClipboardPrivate *priv = NULL;
	guint i, n_selected;

	g_return_if_fail (MODEST_IS_EMAIL_CLIPBOARD (self));
	priv = MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (self);

	n_selected = 1;
	if (priv->src) 
		g_object_unref (priv->src);
	if (priv->selection) {
		n_selected = tny_list_get_length(priv->selection);
		g_object_unref (priv->selection);
	}
	if (priv->hidding) {		
		for (i=0; i < n_selected; i++) 
			g_free (priv->hidding[i]);
		g_free(priv->hidding);
	}

	priv->src = NULL;
	priv->selection = NULL;
	priv->hidding = NULL;
	priv->delete = FALSE;
}

gboolean
modest_email_clipboard_cleared (ModestEmailClipboard *self)
{
	ModestEmailClipboardPrivate *priv = NULL;;
	gboolean cleared = FALSE;

	g_return_val_if_fail (MODEST_IS_EMAIL_CLIPBOARD (self), TRUE);
	priv = MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (self);

	cleared = ((priv->src == NULL) && (priv->selection == NULL));

	return cleared;
}

gboolean 
modest_email_clipboard_check_source_folder (ModestEmailClipboard *self,
					    const TnyFolder *folder)
{
	TnyFolderType folder_type1;
	TnyFolderType folder_type2;
	ModestEmailClipboardPrivate *priv = NULL;;
	const gchar *id1 = NULL;
	const gchar *id2 = NULL;
	gboolean same_folder = FALSE;

	g_return_val_if_fail (MODEST_IS_EMAIL_CLIPBOARD (self), TRUE);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), TRUE);
	priv = MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (self);
	
	/* If cleared, return always FALSE*/
	if (modest_email_clipboard_cleared (self)) return FALSE;

	/* Check target and source folders */
	id1 = tny_folder_get_id (priv->src);
	id2 = tny_folder_get_id (TNY_FOLDER(folder));	
	folder_type1 = modest_tny_folder_guess_folder_type (priv->src);
	folder_type2 = modest_tny_folder_guess_folder_type (folder);
	same_folder = ((folder_type1 == folder_type2) && 
		       (!g_ascii_strcasecmp (id1, id2)));
	
	return same_folder;
}

const gchar **
modest_email_clipboard_get_hidding_ids (ModestEmailClipboard *self,
					guint *n_selected)
{
	ModestEmailClipboardPrivate *priv = NULL;;

	*n_selected = 0;

	g_return_val_if_fail (MODEST_IS_EMAIL_CLIPBOARD (self), NULL);
	priv = MODEST_EMAIL_CLIPBOARD_GET_PRIVATE (self);

	if (priv->selection != NULL)
		*n_selected = tny_list_get_length (priv->selection);
	else
		*n_selected = 1;

	return (const gchar **) priv->hidding;
}

