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

#include "modest-progress-object.h"

static void modest_progress_object_base_init (gpointer g_class);

void
modest_progress_object_add_operation (ModestProgressObject *self, 
				      ModestMailOperation *mail_op)
{
	g_return_if_fail (MODEST_IS_PROGRESS_OBJECT(self));
	
	return MODEST_PROGRESS_OBJECT_GET_IFACE (self)->add_operation_func (self, mail_op);
}

void
modest_progress_object_remove_operation (ModestProgressObject *self, 
					 ModestMailOperation *mail_op)
{
	g_return_if_fail (MODEST_IS_PROGRESS_OBJECT(self));

	return MODEST_PROGRESS_OBJECT_GET_IFACE (self)->remove_operation_func (self, mail_op);
}

void
modest_progress_object_cancel_current_operation (ModestProgressObject *self) 
{
	g_return_if_fail (MODEST_IS_PROGRESS_OBJECT(self));
		
	return MODEST_PROGRESS_OBJECT_GET_IFACE (self)->cancel_current_operation_func (self);
}

void 
modest_progress_object_cancel_all_operations (ModestProgressObject *self)
{
	g_return_if_fail (MODEST_IS_PROGRESS_OBJECT(self));

	return MODEST_PROGRESS_OBJECT_GET_IFACE (self)->cancel_all_operations_func (self);
}

guint
modest_progress_object_num_pending_operations (ModestProgressObject *self) 
{
	g_return_val_if_fail (MODEST_IS_PROGRESS_OBJECT(self), 0);
	
	return MODEST_PROGRESS_OBJECT_GET_IFACE (self)->num_pending_operations_func (self);
}


static void
modest_progress_object_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
	/* create interface signals here */
		initialized = TRUE;
	}
}
GType
modest_progress_object_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestProgressObjectIface),
			modest_progress_object_base_init,		/* base init */
			NULL,		/* base finalize */
			NULL,		/* class_init */
			NULL,		/* class finalize */
			NULL,		/* class data */
			0,
			0,		/* n_preallocs */
			NULL,		/* instance init */
		};
		my_type = g_type_register_static (G_TYPE_INTERFACE,
		                                  "ModestProgressObject",
		                                  &my_info, 0);
		g_type_interface_add_prerequisite (my_type, G_TYPE_OBJECT);
	}
	return my_type;
}
