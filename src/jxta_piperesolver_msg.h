/* 
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact Project JXTA at http://www.jxta.org.
 *
 * 5. Products derived from this software may not be called "JXTA",
 *    nor may "JXTA" appear in their name, without prior written
 *    permission of Sun.
 *
 * THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of Project JXTA.  For more
 * information on Project JXTA, please see
 * <http://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id: jxta_piperesolver_msg.h,v 1.3 2005/01/12 21:46:59 bondolo Exp $
 */

#ifndef __Jxta_piperesolver_msg_H__
#define __Jxta_piperesolver_msg_H__

#include "jxta_advertisement.h"
#include "jstring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _jxta_piperesolver_msg Jxta_piperesolver_msg;
   

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
char * jxta_piperesolver_msg_get_PipeResolver(Jxta_piperesolver_msg * ad);
 
void jxta_piperesolver_msg_set_PipeResolver(Jxta_piperesolver_msg * ad, char * name);

Jxta_id * jxta_piperesolver_msg_get_pipeid (Jxta_piperesolver_msg * ad);

char * jxta_piperesolver_msg_get_MsgType(Jxta_piperesolver_msg * ad);
 
Jxta_status jxta_piperesolver_msg_set_MsgType(Jxta_piperesolver_msg * ad, char* val);

char * jxta_piperesolver_msg_get_Type(Jxta_piperesolver_msg * ad);
 
Jxta_status jxta_piperesolver_msg_set_Type(Jxta_piperesolver_msg * ad, char * val);
 
char * jxta_piperesolver_msg_get_PipeId(Jxta_piperesolver_msg * ad);

Jxta_status jxta_piperesolver_msg_set_PipeId(Jxta_piperesolver_msg * ad, char* val);

boolean jxta_piperesolver_msg_get_Found(Jxta_piperesolver_msg * ad);

Jxta_status jxta_piperesolver_msg_set_Found(Jxta_piperesolver_msg * ad, boolean val);

boolean jxta_piperesolver_msg_get_Cached(Jxta_piperesolver_msg * ad);

Jxta_status jxta_piperesolver_msg_set_Cached(Jxta_piperesolver_msg * ad, boolean val);

char * jxta_piperesolver_msg_get_Peer(Jxta_piperesolver_msg * ad);

Jxta_status jxta_piperesolver_msg_set_Peer(Jxta_piperesolver_msg * ad, char* val);

void jxta_piperesolver_msg_get_PeerAdv(Jxta_piperesolver_msg * ad, JString** val);

Jxta_status jxta_piperesolver_msg_set_PeerAdv(Jxta_piperesolver_msg * ad, JString* val);
 
Jxta_status jxta_piperesolver_msg_get_xml(Jxta_piperesolver_msg * ad, JString** xml);

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
Jxta_piperesolver_msg * jxta_piperesolver_msg_new(void);

void  jxta_piperesolver_msg_parse_charbuffer(Jxta_piperesolver_msg * ad, const char * buf, int len);
  
void jxta_piperesolver_msg_parse_file(Jxta_piperesolver_msg * ad, FILE * stream);

#endif /* __Jxta_pipe_adv_H__  */
#ifdef __cplusplus
}
#endif
