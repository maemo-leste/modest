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

#include <glib/gi18n.h>
#include "modest-header-view.h"
#include <tny-list-iface.h>
#include <string.h>
#include <modest-marshal.h>

#include <modest-icon-names.h>
#include "modest-icon-factory.h"

static void modest_header_view_class_init  (ModestHeaderViewClass *klass);
static void modest_header_view_init        (ModestHeaderView *obj);
static void modest_header_view_finalize    (GObject *obj);

static void     on_selection_changed (GtkTreeSelection *sel, gpointer user_data);

enum {
	MESSAGE_SELECTED_SIGNAL,
	ITEM_NOT_FOUND_SIGNAL,
	STATUS_UPDATE_SIGNAL,
	LAST_SIGNAL
};


typedef struct _ModestHeaderViewPrivate ModestHeaderViewPrivate;
struct _ModestHeaderViewPrivate {

	TnyFolderIface *tny_folder;
	TnyListIface      *headers;
	GSList            *columns;
	GMutex		  *lock;
	ModestHeaderViewStyle style;

	gulong            sig1;

};
#define MODEST_HEADER_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
						MODEST_TYPE_HEADER_VIEW, \
                                                ModestHeaderViewPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

GType
modest_header_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestHeaderViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_header_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestHeaderView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_header_view_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestHeaderView",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_header_view_class_init (ModestHeaderViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_header_view_finalize;
	
	g_type_class_add_private (gobject_class, sizeof(ModestHeaderViewPrivate));
	
	signals[MESSAGE_SELECTED_SIGNAL] = 
		g_signal_new ("message_selected",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass,message_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

	signals[ITEM_NOT_FOUND_SIGNAL] = 
		g_signal_new ("item_not_found",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass,message_not_found),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

	signals[STATUS_UPDATE_SIGNAL] = 
		g_signal_new ("status_update",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass,message_selected),
			      NULL, NULL,
			      modest_marshal_VOID__STRING_INT,
			      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT); 	
}

static void
msgtype_cell_data (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
		   GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
	TnyHeaderFlags flags;
	GdkPixbuf *pixbuf = NULL;

	gtk_tree_model_get (tree_model, iter, TNY_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);

	if (flags & TNY_HEADER_FLAG_DELETED)
		pixbuf = modest_icon_factory_get_small_icon (MODEST_HEADER_ICON_DELETED);
	else if (flags & TNY_HEADER_FLAG_SEEN)
		pixbuf = modest_icon_factory_get_small_icon (MODEST_HEADER_ICON_READ);
	else
		pixbuf = modest_icon_factory_get_small_icon (MODEST_HEADER_ICON_UNREAD);
		
	g_object_set (G_OBJECT (renderer), "pixbuf", pixbuf, NULL);
}

static void
attach_cell_data (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
		  GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
	TnyHeaderFlags flags;
	GdkPixbuf *pixbuf = NULL;

	gtk_tree_model_get (tree_model, iter, TNY_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);

	if (flags & TNY_HEADER_FLAG_ATTACHMENTS)
		pixbuf = modest_icon_factory_get_small_icon (MODEST_HEADER_ICON_ATTACH);

	g_object_set (G_OBJECT (renderer), "pixbuf", pixbuf, NULL);
}


static void
header_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		  GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data)
{
	TnyHeaderFlags flags;
	
	gtk_tree_model_get (tree_model, iter, TNY_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);

	g_object_set (G_OBJECT(renderer),
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? 400: 800,
		      "style",  (flags & TNY_HEADER_FLAG_DELETED) ?
		                 PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,
		      NULL);	
}



/* try to make a shorter display address; changes it arg in-place */
static gchar*
display_address (gchar *address)
{
	gchar *cursor;

	if (!address)
		return NULL;
	
	/* simplistic --> remove <email@address> from display name */
	cursor = g_strstr_len (address, strlen(address), "<");
	if (cursor) 
		cursor[0]='\0';

	/* simplistic --> remove (bla bla) from display name */
	cursor = g_strstr_len (address, strlen(address), "(");
	if (cursor) 
		cursor[0]='\0';
	
	/* FIXME */
	if (!g_utf8_validate (address, -1, NULL)) 
		g_printerr ("modest: invalid: '%s'", address);

	return address;
}



static void
sender_receiver_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
			    GtkTreeModel *tree_model,  GtkTreeIter *iter,  gboolean is_sender)
{
	TnyHeaderFlags flags;
	gchar *address;
	gint sender_receiver_col;

	if (is_sender)
		sender_receiver_col = TNY_HEADER_LIST_MODEL_FROM_COLUMN;
	else
		sender_receiver_col = TNY_HEADER_LIST_MODEL_TO_COLUMN;
		
	gtk_tree_model_get (tree_model, iter,
			    sender_receiver_col,  &address,
			    TNY_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    -1);
	
	g_object_set (G_OBJECT(renderer),
		      "text",
		      display_address (address),
		      "weight",
		      (flags & TNY_HEADER_FLAG_SEEN) ? 400 : 800,
		      "style",
		      (flags & TNY_HEADER_FLAG_DELETED)?PANGO_STYLE_ITALIC:PANGO_STYLE_NORMAL,
		      NULL);

	g_free (address);	
}



/* just to prevent warnings:
 * warning: `%x' yields only last 2 digits of year in some locales
 */
static size_t
my_strftime(char *s, size_t max, const char  *fmt,  const
	    struct tm *tm) {
	return strftime(s, max, fmt, tm);
}



/* not reentrant/thread-safe */
const gchar*
display_date (time_t date)
{
	struct tm date_tm, now_tm; 
	time_t now;

	const gint buf_size = 64; 
	static gchar date_buf[64]; /* buf_size is not ... */
	static gchar now_buf[64];  /* ...const enough... */
	
	now = time (NULL);
	
	localtime_r(&now, &now_tm);
	localtime_r(&date, &date_tm);

	/* get today's date */
	my_strftime (date_buf, buf_size, "%x", &date_tm);
	my_strftime (now_buf,  buf_size, "%x",  &now_tm);  /* today */

	/* if this is today, get the time instead of the date */
	if (strcmp (date_buf, now_buf) == 0)
		strftime (date_buf, buf_size, _("%X"), &date_tm); 
		
	return date_buf;
}


static void
compact_header_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
			   GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data)
{
	GObject *rendobj;
	TnyHeaderFlags flags;
	gchar *from, *subject;
	gchar *header;
	time_t date;
		
	gtk_tree_model_get (tree_model, iter,
			    TNY_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    TNY_HEADER_LIST_MODEL_FROM_COLUMN,  &from,
			    TNY_HEADER_LIST_MODEL_SUBJECT_COLUMN, &subject,
			    TNY_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN, &date,   
			    -1);
	rendobj = G_OBJECT(renderer);		

	header = g_strdup_printf ("%s %s\n%s",
				  display_address (from),
				  display_date(date),
				  subject);
	
	g_object_set (G_OBJECT(renderer),
		      "text",  header,
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? 400: 800,
		      "style",  (flags & TNY_HEADER_FLAG_DELETED) ?
		                 PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,
		      NULL);	
	g_free (header);
	g_free (from);
	g_free (subject);
}


static GtkTreeViewColumn*
get_new_column (const gchar *name, GtkCellRenderer *renderer,
		gboolean resizable, gint sort_col_id, gboolean show_as_text,
		GtkTreeCellDataFunc cell_data_func, gpointer user_data)
{
	GtkTreeViewColumn *column;

	column =  gtk_tree_view_column_new_with_attributes(name, renderer, NULL);
	gtk_tree_view_column_set_resizable (column, resizable);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	
	if (show_as_text) 
		gtk_tree_view_column_add_attribute (column, renderer, "text",
						    sort_col_id);
	if (sort_col_id >= 0)
		gtk_tree_view_column_set_sort_column_id (column, sort_col_id);

	gtk_tree_view_column_set_sort_indicator (column, FALSE);
	gtk_tree_view_column_set_reorderable (column, TRUE);

	if (cell_data_func)
		gtk_tree_view_column_set_cell_data_func(column, renderer, cell_data_func,
							user_data, NULL);

	return column;
}




static void
remove_all_columns (ModestHeaderView *obj)
{
	GList *columns, *cursor;

	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW(obj));

	for (cursor = columns; cursor; cursor = cursor->next)
		gtk_tree_view_remove_column (GTK_TREE_VIEW(obj),
					     GTK_TREE_VIEW_COLUMN(cursor->data));
	g_list_free (columns);	
}




static void
init_columns (ModestHeaderView *obj)
{
	GtkTreeViewColumn *column=NULL;
	GtkCellRenderer *renderer_msgtype,
		*renderer_header,
		*renderer_attach;

	ModestHeaderViewPrivate *priv;
	GSList *cursor;
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(obj); 
			
	renderer_msgtype = gtk_cell_renderer_pixbuf_new ();
	renderer_attach  = gtk_cell_renderer_pixbuf_new ();
	renderer_header = gtk_cell_renderer_text_new (); 
	
	remove_all_columns (obj);
	
	for (cursor = priv->columns; cursor; cursor = cursor->next) {
		ModestHeaderViewColumn col =
			(ModestHeaderViewColumn) GPOINTER_TO_INT(cursor->data);
		
		switch (col) {
			
		case MODEST_HEADER_VIEW_COLUMN_MSGTYPE:

			column = get_new_column (_("M"), renderer_msgtype, FALSE,
						 TNY_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 FALSE, (GtkTreeCellDataFunc)msgtype_cell_data,
						 NULL);
			break;

		case MODEST_HEADER_VIEW_COLUMN_ATTACH:

			column = get_new_column (_("A"), renderer_attach, FALSE,
						 TNY_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 FALSE, (GtkTreeCellDataFunc)attach_cell_data,
						 NULL);
			break;
			
		case MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE:
			column = get_new_column (_("Received"), renderer_header, TRUE,
						 TNY_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
						 TRUE, (GtkTreeCellDataFunc)header_cell_data,
						 NULL);
			break;
			
		case MODEST_HEADER_VIEW_COLUMN_FROM:
			column = get_new_column (_("From"), renderer_header, TRUE,
						 TNY_HEADER_LIST_MODEL_FROM_COLUMN,
						 TRUE, (GtkTreeCellDataFunc)sender_receiver_cell_data,
						 GINT_TO_POINTER(TRUE));
			break;

		case MODEST_HEADER_VIEW_COLUMN_TO:
			column = get_new_column (_("To"), renderer_header, TRUE,
						 TNY_HEADER_LIST_MODEL_TO_COLUMN,
						 TRUE, (GtkTreeCellDataFunc)sender_receiver_cell_data,
						 GINT_TO_POINTER(FALSE));
			break;
			
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER:
			column = get_new_column (_("Header"), renderer_header, TRUE,
						 TNY_HEADER_LIST_MODEL_FROM_COLUMN,
						 TRUE, (GtkTreeCellDataFunc)compact_header_cell_data,
						 NULL);
			break;
			
		case MODEST_HEADER_VIEW_COLUMN_SUBJECT:
			column = get_new_column (_("Subject"), renderer_header, TRUE,
						 TNY_HEADER_LIST_MODEL_SUBJECT_COLUMN,
						 TRUE, (GtkTreeCellDataFunc)header_cell_data,
						 NULL);
			break;
			
			
		case MODEST_HEADER_VIEW_COLUMN_SENT_DATE:
			column = get_new_column (_("Sent"), renderer_header, TRUE,
						 TNY_HEADER_LIST_MODEL_DATE_SENT_COLUMN,
						 TRUE, (GtkTreeCellDataFunc)header_cell_data,
						 NULL);
			break;

		default:
			g_assert_not_reached ();
		}

		gtk_tree_view_column_set_visible (column, TRUE);
		gtk_tree_view_column_set_min_width (column, 36);
		gtk_tree_view_column_set_resizable (column, TRUE);
		
		gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);		
	}	
}



static void
modest_header_view_init (ModestHeaderView *obj)
{
	ModestHeaderViewPrivate *priv;
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(obj); 

	priv->lock = g_mutex_new ();
	priv->sig1 = 0;
}

static void
modest_header_view_finalize (GObject *obj)
{
	ModestHeaderView        *self;
	ModestHeaderViewPrivate *priv;
	GtkTreeSelection        *sel;
	
	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->headers)	
		g_object_unref (G_OBJECT(priv->headers));

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));

	if (sel && priv->sig1 != 0) {
		g_signal_handler_disconnect (G_OBJECT(sel), priv->sig1);
		priv->sig1 = 0;
	}
		
	if (priv->lock) {
		g_mutex_free (priv->lock);
		priv->lock = NULL;
	}

	priv->headers       = NULL;
	priv->tny_folder    = NULL;
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_header_view_new (TnyFolderIface *folder,
			GSList *columns,
			ModestHeaderViewStyle style)
{
	GObject *obj;
	GtkTreeSelection *sel;
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_HEADER_VIEW, NULL));
	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);
	
	if (!modest_header_view_set_folder (self, NULL)) {
		g_warning ("could not set the folder");
		g_object_unref (obj);
		return NULL;
	}
	
	modest_header_view_set_style   (self, style);
	modest_header_view_set_columns (self, columns);
	
	/* all cols */
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(obj), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(obj), TRUE);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(obj));
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(obj),
				      TRUE); /* alternating row colors */

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	
	priv->sig1 = g_signal_connect (sel, "changed",
				       G_CALLBACK(on_selection_changed), self);
	

	return GTK_WIDGET(self);
}

gboolean
modest_header_view_set_columns (ModestHeaderView *self, GSList *columns)
{
	ModestHeaderViewPrivate *priv;
	GSList *cursor;

	g_return_val_if_fail (self, FALSE);
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	g_slist_free (priv->columns);
	
	for (cursor = columns; cursor; cursor = cursor->next) {
		ModestHeaderViewColumn col = 
			(ModestHeaderViewColumn) GPOINTER_TO_INT(cursor->data);
		if (0 > col || col >= MODEST_HEADER_VIEW_COLUMN_NUM)
			g_printerr ("modest: invalid column %d in column list\n", col);
		else
			priv->columns = g_slist_append (priv->columns, cursor->data);
	}

	init_columns (self); /* redraw them */
	return TRUE;
}



const GSList*
modest_header_view_get_columns (ModestHeaderView *self)
{
	ModestHeaderViewPrivate *priv;

	g_return_val_if_fail (self, FALSE);
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);
	return priv->columns;
}




gboolean
modest_header_view_set_style (ModestHeaderView *self,
				       ModestHeaderViewStyle style)
{
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (style >= 0 && style < MODEST_HEADER_VIEW_STYLE_NUM,
			      FALSE);
	
	MODEST_HEADER_VIEW_GET_PRIVATE(self)->style = style;
	
	return TRUE;
}

ModestHeaderViewStyle
modest_header_view_get_style (ModestHeaderView *self)
{
	g_return_val_if_fail (self, FALSE);

	return MODEST_HEADER_VIEW_GET_PRIVATE(self)->style;
}



/* get the length of any prefix that should be ignored for sorting */
static inline int 
get_prefix_len (const gchar *sub)
{
	gint i = 0;
	const static gchar* prefix[] = {"Re:", "RE:", "Fwd:", "FWD:", "FW:", "AW:", NULL};

	if (sub[0] != 'R' && sub[0] != 'F') /* optimization */
		return 0;
	
	while (prefix[i]) {
		if (g_str_has_prefix(sub, prefix[i])) {
			int prefix_len = strlen(prefix[i]); 
			if (sub[prefix_len] == ' ')
				++prefix_len; /* ignore space after prefix as well */
			return prefix_len; 
		}
		++i;
	}
	return 0;
}


static inline gint
cmp_normalized_subject (const gchar* s1, const gchar *s2)
{
	gint result = 0;
	register gchar *n1, *n2;
	
	n1 = g_utf8_collate_key (s1 + get_prefix_len(s1), -1);
	n2 = g_utf8_collate_key (s2 + get_prefix_len(s2), -1);
	
	result = strcmp (n1, n2);
	g_free (n1);
	g_free (n2);
	
	return result;
}


static gint
cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
	  gpointer user_data)
{
	gint col_id = GPOINTER_TO_INT (user_data);
	gint t1, t2;
	gint val1, val2;
	gchar *s1, *s2;
	gint cmp;
	
	g_return_val_if_fail (GTK_IS_TREE_MODEL(tree_model), -1);
	
	switch (col_id) {

		/* first one, we decide based on the time */
	case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER:
	case MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE:
		gtk_tree_model_get (tree_model, iter1,
				    TNY_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
				    &t1,-1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
				    &t2,-1);
		return t1 - t2;
		
	case MODEST_HEADER_VIEW_COLUMN_SENT_DATE:
		gtk_tree_model_get (tree_model, iter1,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
				    &t1,-1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
				    &t2,-1);
		return t1 - t2;

		
		/* next ones, we try the search criteria first, if they're the same, then we use 'sent date' */
	case MODEST_HEADER_VIEW_COLUMN_SUBJECT: {

		gtk_tree_model_get (tree_model, iter1,
				    TNY_HEADER_LIST_MODEL_SUBJECT_COLUMN, &s1,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,
				    -1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_HEADER_LIST_MODEL_SUBJECT_COLUMN, &s2,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,
				    -1);
		
		cmp = cmp_normalized_subject(s1, s2);

		g_free (s1);
		g_free (s2);
		
		return cmp ? cmp : t1 - t2;
	}
		
	case MODEST_HEADER_VIEW_COLUMN_FROM:
		
		gtk_tree_model_get (tree_model, iter1,
				    TNY_HEADER_LIST_MODEL_FROM_COLUMN, &s1,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,
				    -1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_HEADER_LIST_MODEL_FROM_COLUMN, &s2,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,
				    -1);
		cmp = strcmp (s1, s2);
		g_free (s1);
		g_free (s2);
		
		return cmp ? cmp : t1 - t2;
		
	case MODEST_HEADER_VIEW_COLUMN_TO: 
		
		gtk_tree_model_get (tree_model, iter1,
				    TNY_HEADER_LIST_MODEL_TO_COLUMN, &s1,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,
				    -1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_HEADER_LIST_MODEL_TO_COLUMN, &s2,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,
				    -1);
		cmp = strcmp (s1, s2);
		g_free (s1);
		g_free (s2);
		
		return cmp ? cmp : t1 - t2;

	case MODEST_HEADER_VIEW_COLUMN_ATTACH:

		gtk_tree_model_get (tree_model, iter1, TNY_HEADER_LIST_MODEL_FLAGS_COLUMN, &val1,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1, -1);
		gtk_tree_model_get (tree_model, iter2, TNY_HEADER_LIST_MODEL_FLAGS_COLUMN, &val2,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2, -1);
		
		cmp = (val1 & TNY_HEADER_FLAG_ATTACHMENTS) -
			(val2 & TNY_HEADER_FLAG_ATTACHMENTS);

		return cmp ? cmp : t1 - t2;
		
	case MODEST_HEADER_VIEW_COLUMN_MSGTYPE:
		gtk_tree_model_get (tree_model, iter1, TNY_HEADER_LIST_MODEL_FLAGS_COLUMN, &val1,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_HEADER_LIST_MODEL_FLAGS_COLUMN, &val2,
				    TNY_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,-1);
		cmp =  (val1 & TNY_HEADER_FLAG_SEEN) - (val2 & TNY_HEADER_FLAG_SEEN);

		return cmp ? cmp : t1 - t2;

	default:
		return &iter1 - &iter2; /* oughhhh  */
	}
}


static void
on_refresh_folder (TnyFolderIface *folder, gboolean cancelled, gpointer user_data)
{
	GtkTreeModel *oldsortable, *sortable;
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;

	if (cancelled)
		return;

	self = MODEST_HEADER_VIEW(user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	g_mutex_lock (priv->lock);

	if (!folder)  /* when there is no folder */
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(self), FALSE);
	
	else { /* it's a new one or a refresh */
		GSList *col;

		priv->headers = TNY_LIST_IFACE(tny_header_list_model_new ());
 		
		tny_folder_iface_get_headers (folder, priv->headers, FALSE);
		tny_header_list_model_set_folder (TNY_HEADER_LIST_MODEL(priv->headers),
						      folder, TRUE); /* async */
		
		oldsortable = gtk_tree_view_get_model(GTK_TREE_VIEW (self));
		if (oldsortable && GTK_IS_TREE_MODEL_SORT(oldsortable)) {
			GtkTreeModel *oldmodel = gtk_tree_model_sort_get_model
				(GTK_TREE_MODEL_SORT(oldsortable));
			if (oldmodel)
				g_object_unref (G_OBJECT(oldmodel));
			g_object_unref (oldsortable);
		}
	
		sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(priv->headers));

		/* install our special sorting functions */
		col = priv->columns;
		while (col) {
			gint col_id = GPOINTER_TO_INT (col->data);
			gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable), col_id,
							 (GtkTreeIterCompareFunc)cmp_rows,
							 GINT_TO_POINTER(col_id), NULL);
			col = col->next;
		}

		gtk_tree_view_set_model (GTK_TREE_VIEW (self), sortable);
		gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(self), TRUE);
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(self), TRUE);
		/* no need to unref sortable */
	}
	g_mutex_unlock (priv->lock);
}


static void
on_refresh_folder_status_update (TnyFolderIface *folder, const gchar *msg,
				 gint status_id,  gpointer user_data)
{
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;

	self = MODEST_HEADER_VIEW(user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	g_signal_emit (G_OBJECT(self), signals[STATUS_UPDATE_SIGNAL],
		       0, msg, status_id);
}



gboolean
modest_header_view_set_folder (ModestHeaderView *self,
			       TnyFolderIface *folder)
{
	ModestHeaderViewPrivate *priv;
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);
	
	g_mutex_lock (priv->lock);

	if (!folder)  {/* when there is no folder */
		GtkTreeModel *model;
		model = gtk_tree_view_get_model (GTK_TREE_VIEW(self));
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(self), FALSE);
		gtk_tree_view_set_model (GTK_TREE_VIEW (self), NULL);
		if (model)
			g_object_unref (model);
	} else { /* it's a new one or a refresh */
		tny_folder_iface_refresh_async (folder,
						    on_refresh_folder,
						    on_refresh_folder_status_update,
						    self);
	}

	/* no message selected */
	g_signal_emit (G_OBJECT(self), signals[MESSAGE_SELECTED_SIGNAL], 0,
		       NULL);

	g_mutex_unlock (priv->lock);

	return TRUE;
}



static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel            *model;
	TnyHeaderIface       *header;
	GtkTreeIter             iter;
	ModestHeaderView        *self;
	ModestHeaderViewPrivate *priv;
	const TnyMsgIface *msg = NULL;
	const TnyFolderIface *folder;
	
	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	self = MODEST_HEADER_VIEW (user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);	
	
	if (!gtk_tree_selection_get_selected (sel, &model, &iter))
		return; /* msg was _un_selected */
	
	gtk_tree_model_get (model, &iter,
			    TNY_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);
	
	if (!header) {
		g_printerr ("modest: cannot find header\n");
		return;
	}

	folder = tny_header_iface_get_folder (TNY_HEADER_IFACE(header));
	if (!folder) {
		g_signal_emit (G_OBJECT(self), signals[ITEM_NOT_FOUND_SIGNAL], 0,
			       MODEST_ITEM_TYPE_FOLDER);
		return;
	}
	
	msg = tny_folder_iface_get_message (TNY_FOLDER_IFACE(folder),
						header);
	if (!msg) {
		g_signal_emit (G_OBJECT(self), signals[ITEM_NOT_FOUND_SIGNAL], 0,
			       MODEST_ITEM_TYPE_MESSAGE);
		return;
	}
					
	g_signal_emit (G_OBJECT(self), signals[MESSAGE_SELECTED_SIGNAL], 0,
		       msg);
	
	/* mark message as seen; _set_flags crashes, bug in tinymail? */
	//flags = tny_header_iface_get_flags (TNY_HEADER_IFACE(header));
	//tny_header_iface_set_flags (header, TNY_HEADER_FLAG_SEEN);
}	

