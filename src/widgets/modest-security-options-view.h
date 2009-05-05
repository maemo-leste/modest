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

#ifndef __MODEST_SECURITY_OPTIONS_VIEW_H__
#define __MODEST_SECURITY_OPTIONS_VIEW_H__

#include <glib.h>
#include <glib-object.h>
#include "modest-account-settings.h"
#include "modest-protocol.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_SECURITY_OPTIONS_VIEW             (modest_security_options_view_get_type())
#define MODEST_SECURITY_OPTIONS_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_SECURITY_OPTIONS_VIEW,ModestSecurityOptionsView))
#define MODEST_SECURITY_OPTIONS_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_SECURITY_OPTIONS_VIEW,ModestSecurityOptionsViewClass))
#define MODEST_IS_SECURITY_OPTIONS_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_SECURITY_OPTIONS_VIEW))
#define MODEST_IS_SECURITY_OPTIONS_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_SECURITY_OPTIONS_VIEW))
#define MODEST_SECURITY_OPTIONS_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_SECURITY_OPTIONS_VIEW,ModestSecurityOptionsViewClass))

typedef enum {
	MODEST_SECURITY_OPTIONS_INCOMING,
	MODEST_SECURITY_OPTIONS_OUTGOING,
} ModestSecurityOptionsType;

typedef struct _ModestSecurityOptionsView      ModestSecurityOptionsView;
typedef struct _ModestSecurityOptionsViewClass ModestSecurityOptionsViewClass;

struct _ModestSecurityOptionsView {
	GtkVBox parent;

	/* Incoming or outgoing */
	ModestSecurityOptionsType type;
};

struct _ModestSecurityOptionsViewClass {
	GtkVBoxClass parent_class;

	void (*load_settings) (ModestSecurityOptionsView* self, ModestAccountSettings *settings);
	void (*save_settings) (ModestSecurityOptionsView* self, ModestAccountSettings *settings);
	gboolean (*changed) (ModestSecurityOptionsView* self, ModestAccountSettings *settings);

	/* Signals */
	void (*missing_mandatory_data) (ModestSecurityOptionsView* self, 
					gboolean missing, 
					gpointer user_data);
};

GType        modest_security_options_view_get_type    (void) G_GNUC_CONST;

void modest_security_options_view_load_settings (ModestSecurityOptionsView* self, 
						 ModestAccountSettings *settings);

void modest_security_options_view_save_settings (ModestSecurityOptionsView* self, 
						 ModestAccountSettings *settings);

void modest_security_options_view_set_server_type (ModestSecurityOptionsView* self, 
						   ModestProtocolType server_type);

ModestProtocolType modest_security_options_view_get_connection_protocol (ModestSecurityOptionsView *self);

gboolean modest_security_options_view_changed (ModestSecurityOptionsView* self,
					       ModestAccountSettings *settings);

void modest_security_options_view_enable_changes (ModestSecurityOptionsView* self,
						  gboolean enable);

/**
 * modest_security_options_view_auth_check:
 * @self: a #ModestSecurityOptionsView
 * 
 * checks if the supported authentication methods for the server
 * should be checked. This happens when the user have checked the
 * "require auth" option but is not selecting a secure protocol (like
 * TLS or SSL)
 * 
 * Returns: TRUE if require auth is true and no security protocol is
 * selected
 **/
gboolean modest_security_options_view_auth_check (ModestSecurityOptionsView* self);

gboolean modest_security_options_view_has_missing_mandatory_data (ModestSecurityOptionsView* self);


G_END_DECLS

#endif /* __MODEST_SECURITY_OPTIONS_VIEW_H__ */

