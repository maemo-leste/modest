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

#ifndef __MODEST_PROGRESS_OBJECT_H__
#define __MODEST_PROGRESS_OBJECT_H__

/* other include files */
#include "modest-mail-operation.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_PROGRESS_OBJECT             (modest_progress_object_get_type())
#define MODEST_PROGRESS_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_PROGRESS_OBJECT,ModestProgressObject))
#define MODEST_IS_PROGRESS_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_PROGRESS_OBJECT))
#define MODEST_PROGRESS_OBJECT_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE((inst),MODEST_TYPE_PROGRESS_OBJECT,ModestProgressObjectIface))

typedef struct _ModestProgressObject      ModestProgressObject;
typedef struct _ModestProgressObjectIface ModestProgressObjectIface;

struct _ModestProgressObjectIface {
	GTypeInterface parent;

	/* the 'vtable': declare function pointers here, eg.: */
	void (*add_operation_func) (ModestProgressObject *self, ModestMailOperation *mail_op);
	void (*remove_operation_func) (ModestProgressObject *self, ModestMailOperation *mail_op);
	void (*cancel_current_operation_func) (ModestProgressObject *self);
};

GType     modest_progress_object_get_type            (void) G_GNUC_CONST;

void      modest_progress_object_add_operation       (ModestProgressObject *self,
						      ModestMailOperation  *mail_op);

void      modest_progress_object_remove_operation    (ModestProgressObject *self,
						      ModestMailOperation  *mail_op);
void      modest_progress_object_cancel_current_operation (ModestProgressObject *self);

G_END_DECLS

#endif /* __MODEST_PROGRESS_OBJECT_H__ */
