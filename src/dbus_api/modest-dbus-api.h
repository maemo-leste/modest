/* Copyright (c) 2007, Nokia Corporation
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


#ifndef __MODEST_DBUS_API__
#define __MODEST_DBUS_API__


/* Note that the com.nokia service name and /com/nokia object name 
 * are what is assumed by the, bizarrely named, osso_rpc_run_with_defaults() function, 
 * so they are probably a good choice. */
#define MODEST_DBUS_NAME    "modestemail"
#define MODEST_DBUS_EXAMPLE_SERVICE "com.nokia."MODEST_DBUS_NAME
#define MODEST_DBUS_EXAMPLE_OBJECT  "/com/nokia/"MODEST_DBUS_NAME
#define MODEST_DBUS_EXAMPLE_IFACE   "com.nokia."MODEST_DBUS_NAME

#define MODEST_DBUS_EXAMPLE_MESSAGE "HelloWorld"

#define MODEST_DBUS_METHOD_SEND_MAIL "SendMail"
enum ModestDbusSendMailArguments
{
	MODEST_DEBUS_SEND_MAIL_ARG_TO,
	MODEST_DEBUS_SEND_MAIL_ARG_CC,
	MODEST_DEBUS_SEND_MAIL_ARG_BCC,
	MODEST_DEBUS_SEND_MAIL_ARG_SUBJECT,
	MODEST_DEBUS_SEND_MAIL_ARG_BODY,
	/* TODO: MODEST_DEBUS_SEND_MAIL_ARG_ATTACHMENTS, */
	MODEST_DEBUS_SEND_MAIL_ARGS_COUNT
};

#endif /* __MODEST_DBUS_API__ */

