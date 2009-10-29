/* Copyright (c) 2009, Igalia
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
#include <modest-hildon-pannable-area-scrollable.h>
#include "modest-toolkit-factory.h"

static void modest_toolkit_factory_class_init (ModestToolkitFactoryClass *klass);
static void modest_toolkit_factory_init (ModestToolkitFactory *self);

/* GObject interface */
static GtkWidget * modest_toolkit_factory_create_scrollable_default (ModestToolkitFactory *self);
/* globals */
static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE    (ModestToolkitFactory,
		  modest_toolkit_factory,
		  G_TYPE_OBJECT);

ModestToolkitFactory *
modest_toolkit_factory_get_instance                            (void)
{
    GObject* self = g_object_new (MODEST_TYPE_TOOLKIT_FACTORY, NULL);

    return (ModestToolkitFactory *) self;
}

static void
modest_toolkit_factory_class_init (ModestToolkitFactoryClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	klass->create_scrollable = modest_toolkit_factory_create_scrollable_default;
}

static void
modest_toolkit_factory_init (ModestToolkitFactory *self)
{
}

GtkWidget *
modest_toolkit_factory_create_scrollable (ModestToolkitFactory *self)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_scrollable (self);
}

static GtkWidget *
modest_toolkit_factory_create_scrollable_default (ModestToolkitFactory *self)
{
	return modest_hildon_pannable_area_scrollable_new ();
}
