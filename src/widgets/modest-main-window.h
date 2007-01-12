/* Copyright (c) 2006,2007 Nokia Corporation
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


#ifndef __MODEST_MAIN_WINDOW_H__
#define __MODEST_MAIN_WINDOW_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include "modest-widget-factory.h"
#include "modest-window.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAIN_WINDOW             (modest_main_window_get_type())
#define MODEST_MAIN_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindow))
#define MODEST_MAIN_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MAIN_WINDOW,ModestWindow))

#define MODEST_IS_MAIN_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_IS_MAIN_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_MAIN_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindowClass))

typedef struct _ModestMainWindow      ModestMainWindow;
typedef struct _ModestMainWindowClass ModestMainWindowClass;

struct _ModestMainWindow {
	ModestWindow parent;
	/* insert public members, if any */
};

struct _ModestMainWindowClass {
	ModestWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestMainWindow* obj); */
};

/* member functions */
GType modest_main_window_get_type (void) G_GNUC_CONST;


ModestWindow* modest_main_window_new (ModestWidgetFactory *factory,
				      TnyAccountStore *account_store);

ModestWidgetFactory *   modest_main_window_get_widget_factory    (ModestMainWindow *main_window);

TnyAccountStore *       modest_main_window_get_account_store     (ModestMainWindow *main_window);


G_END_DECLS

#endif /* __MODEST_MAIN_WINDOW_H__ */
