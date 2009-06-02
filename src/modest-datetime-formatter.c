/* Copyright (c) 2008, Nokia Corporation
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

#include <config.h>
#include <modest-datetime-formatter.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <gconf/gconf-client.h>
#include <gtk/gtkmarshal.h>
#endif
#include <glib/gi18n.h>
#include "modest-text-utils.h"
#ifdef MODEST_USE_LIBTIME
#include <clockd/libtime.h>
#include <libosso.h>
#include <modest-platform.h>
#endif

typedef enum {
	DATETIME_FORMAT_12H,
	DATETIME_FORMAT_24H,
	DATETIME_FORMAT_LOCALE,
} DatetimeFormat;

#define HILDON2_GCONF_FORMAT_DIR "/apps/clock"
#define HILDON2_GCONF_FORMAT_KEY HILDON2_GCONF_FORMAT_DIR "/time-format"

/* 'private'/'protected' functions */
static void   modest_datetime_formatter_class_init (ModestDatetimeFormatterClass *klass);
static void   modest_datetime_formatter_finalize   (GObject *obj);
static void   modest_datetime_formatter_instance_init (ModestDatetimeFormatter *obj);

typedef struct _ModestDatetimeFormatterPrivate ModestDatetimeFormatterPrivate;
struct _ModestDatetimeFormatterPrivate {
	DatetimeFormat current_format;
#ifdef MODEST_TOOLKIT_HILDON2
	guint gconf_handler;
#endif
};

#define MODEST_DATETIME_FORMATTER_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
										   MODEST_TYPE_DATETIME_FORMATTER, \
										   ModestDatetimeFormatterPrivate))

enum {
	FORMAT_CHANGED_SIGNAL,
	LAST_SIGNAL
};

/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

GType
modest_datetime_formatter_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestDatetimeFormatterClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) modest_datetime_formatter_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ModestDatetimeFormatter),
			0,      /* n_preallocs */
			(GInstanceInitFunc) modest_datetime_formatter_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestDatetimeFormatter",
						  &my_info, 0);
	}
	return my_type;
}

static void
modest_datetime_formatter_class_init (ModestDatetimeFormatterClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_datetime_formatter_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof(ModestDatetimeFormatterPrivate));

	signals[FORMAT_CHANGED_SIGNAL] =
		g_signal_new ("format_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestDatetimeFormatterClass, format_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

#ifdef MODEST_TOOLKIT_HILDON2
static void
update_format (ModestDatetimeFormatter *obj)
{
	GConfClient *gconf;
	GError *err = NULL;
	gboolean gconf_value;
	ModestDatetimeFormatterPrivate *priv;

	priv = MODEST_DATETIME_FORMATTER_GET_PRIVATE (obj);

	gconf = gconf_client_get_default ();
	gconf_value = gconf_client_get_bool (gconf, HILDON2_GCONF_FORMAT_KEY,
					     &err);

	if (err != NULL) {
		g_warning ("Error reading time format in gconf %s", err->message);
		g_error_free (err);
	} else {
		priv->current_format = gconf_value?DATETIME_FORMAT_24H:DATETIME_FORMAT_12H;
	}
}

static void
clock_format_changed (GConfClient *gconf,
		      guint cnxn_id,
		      GConfEntry *entry,
		      gpointer userdata)
{
	ModestDatetimeFormatter *self = (ModestDatetimeFormatter *) userdata;

	update_format (self);
	g_signal_emit (G_OBJECT (self), signals[FORMAT_CHANGED_SIGNAL], 0);
}
#endif

#ifdef MODEST_USE_LIBTIME
static void 
time_changed_cb (gpointer userdata)
{
	time_get_synced ();
}
#endif

static void
init_format (ModestDatetimeFormatter *obj)
{
	ModestDatetimeFormatterPrivate *priv;

	priv = MODEST_DATETIME_FORMATTER_GET_PRIVATE (obj);

	priv->current_format = DATETIME_FORMAT_LOCALE;

#ifdef MODEST_TOOLKIT_HILDON2
	GConfClient *gconf;
	GError *err = NULL;

	gconf = gconf_client_get_default ();
	gconf_client_add_dir (gconf, HILDON2_GCONF_FORMAT_DIR,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      &err);
	priv->gconf_handler = gconf_client_notify_add (gconf, HILDON2_GCONF_FORMAT_KEY,
						       clock_format_changed, (gpointer) obj,
						       NULL, &err);

	if (err != NULL) {
		g_warning ("Error listening to time format in gconf %s", err->message);
		g_error_free (err);
	}

	update_format (obj);
#endif

#ifdef MODEST_USE_LIBTIME
	osso_time_set_notification_cb (modest_platform_get_osso_context (),
				       time_changed_cb,
				       obj);
#endif

}

static void
modest_datetime_formatter_instance_init (ModestDatetimeFormatter *obj)
{
	init_format (obj);
}

static void   
modest_datetime_formatter_finalize   (GObject *obj)
{
#ifdef MODEST_TOOLKIT_HILDON2
	ModestDatetimeFormatterPrivate *priv;
	GConfClient *gconf;

	priv = MODEST_DATETIME_FORMATTER_GET_PRIVATE (obj);
	gconf = gconf_client_get_default ();
	gconf_client_notify_remove (gconf,
				    priv->gconf_handler);
	priv->gconf_handler = 0;
	gconf_client_remove_dir (gconf, HILDON2_GCONF_FORMAT_DIR,
				 NULL);
#endif
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

ModestDatetimeFormatter*
modest_datetime_formatter_new (void)
{
	return g_object_new (MODEST_TYPE_DATETIME_FORMATTER, NULL);
}

const gchar *
modest_datetime_formatter_format_date (ModestDatetimeFormatter *self,
				       time_t date)
{
#define DATE_BUF_SIZE 64 

	static gchar date_buf[DATE_BUF_SIZE];
	ModestDatetimeFormatterPrivate *priv;
	const gchar *format_string = NULL;

	g_return_val_if_fail (MODEST_IS_DATETIME_FORMATTER (self), NULL);
	priv = MODEST_DATETIME_FORMATTER_GET_PRIVATE (self);

	switch (priv->current_format) {
	case DATETIME_FORMAT_12H:
	case DATETIME_FORMAT_24H:
		format_string = _HL("wdgt_va_date");
		break;
	case DATETIME_FORMAT_LOCALE:
		format_string = "%x";
		break;
	}
	modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, format_string, date);

	return date_buf;
}

const gchar *
modest_datetime_formatter_format_time (ModestDatetimeFormatter *self,
				       time_t date)
{
#define DATE_BUF_SIZE 64 

	static gchar date_buf[DATE_BUF_SIZE];
	ModestDatetimeFormatterPrivate *priv;
	const gchar *format_string = NULL;
	gboolean is_pm;
	struct tm localtime_tm = {0, };

	g_return_val_if_fail (MODEST_IS_DATETIME_FORMATTER (self), NULL);
	priv = MODEST_DATETIME_FORMATTER_GET_PRIVATE (self);
#ifdef MODEST_USE_LIBTIME
	time_get_local_ex (date, &localtime_tm);
#else
	time_t date_copy;
	date_copy = date;
	localtime_r (&date_copy, &localtime_tm);
#endif
	is_pm = (localtime_tm.tm_hour/12) % 2;

	switch (priv->current_format) {
	case DATETIME_FORMAT_12H:
		format_string = is_pm?_HL("wdgt_va_12h_time_pm"):_HL("wdgt_va_12h_time_am");
		break;
	case DATETIME_FORMAT_24H:
		format_string = _HL("wdgt_va_24h_time");
		break;
	case DATETIME_FORMAT_LOCALE:
		format_string = "%X";
		break;
	}
	modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, format_string, date);

	return date_buf;
}

const gchar *
modest_datetime_formatter_display_long_datetime (ModestDatetimeFormatter *self,
						 time_t date)
{

#define DATE_BUF_DOUBLE_SIZE 128 

	static gchar date_buf[DATE_BUF_DOUBLE_SIZE];
	
	snprintf (date_buf, DATE_BUF_DOUBLE_SIZE, 
		  "%s %s", modest_datetime_formatter_format_date (self, date), 
		  modest_datetime_formatter_format_time (self, date));

	return date_buf;
}

const gchar *
modest_datetime_formatter_display_datetime (ModestDatetimeFormatter *self,
					    time_t date)
{

	struct tm today_localtime_tm = {0, };
	struct tm date_localtime_tm = {0, };
	time_t today;

	today = time (NULL);
#ifdef MODEST_USE_LIBTIME
	time_get_local_ex (today, &today_localtime_tm);
	time_get_local_ex (date, &date_localtime_tm);
#else
	time_t date_copy;
	date_copy = today;
	localtime_r (&date_copy, &today_localtime_tm);
	date_copy = date;
	localtime_r (&date_copy, &date_localtime_tm);
#endif

	if (today_localtime_tm.tm_mday == date_localtime_tm.tm_mday &&
	    today_localtime_tm.tm_mon == date_localtime_tm.tm_mon &&
	    today_localtime_tm.tm_year == date_localtime_tm.tm_year)
		return modest_datetime_formatter_format_time (self, date);
	else
		return modest_datetime_formatter_format_date (self, date);
}
