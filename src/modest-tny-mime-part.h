/* Copyright (c) 2006, 2007, 2008 Nokia Corporation
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

#ifndef __MODEST_TNY_MIME_PART_H__
#define __MODEST_TNY_MIME_PART_H__

#include <tny-mime-part.h>

/**
 * modest_tny_mime_part_get_header_value:
 * @self: some #TnyMimePart 
 * @header: the header to get
 * 
 * gets the mail header for a #TnyMimePart as a newly allocated string,
 * or NULL if it cannot be found
 * 
 * Returns: the header
 **/
gchar* modest_tny_mime_part_get_header_value (TnyMimePart *part, const gchar *header);


/**
 * modest_tny_mime_part_is_attachment_for_modest
 * @self: some #TnyMimePart 
 * 
 * determines whether some MIME part is an attachment, from Modest's PoV.
 *
 * Returns: TRUE if it's an attachment, FALSE otherwise.
 **/

gboolean modest_tny_mime_part_is_attachment_for_modest (TnyMimePart *part);

#endif /*__MODEST_TNY_MIME_PART_H__*/
