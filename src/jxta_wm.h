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
 * $Id: jxta_wm.h,v 1.4 2005/01/12 21:47:01 bondolo Exp $
 */

   
#ifndef __JxtaWire_H__
#define __JxtaWire_H__

#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JxtaWire JxtaWire;

JxtaWire * JxtaWire_new(void);
void JxtaWire_set_handlers(JxtaWire *, XML_Parser, void *);
void JxtaWire_delete(JxtaWire *);
Jxta_status JxtaWire_get_xml(JxtaWire *, JString** xml);
void JxtaWire_parse_charbuffer(JxtaWire *, const char *, int len); 
void JxtaWire_parse_file(JxtaWire *, FILE * stream);
 
char * JxtaWire_get_JxtaWire(JxtaWire *);
void JxtaWire_set_JxtaWire(JxtaWire *, char *);
 
char* JxtaWire_get_SrcPeer(JxtaWire *);
void JxtaWire_set_SrcPeer(JxtaWire *, char *);

char* JxtaWire_get_PipeId(JxtaWire *);
void JxtaWire_set_PipeId(JxtaWire *, char *);

char* JxtaWire_get_MsgId(JxtaWire *);
void JxtaWire_set_MsgId(JxtaWire *, char *);
 
Jxta_vector * JxtaWire_get_VisitedPeer(JxtaWire *);
void JxtaWire_set_VisitedPeer(JxtaWire *, Jxta_vector *);

int JxtaWire_get_TTL(JxtaWire *);
void JxtaWire_set_TTL(JxtaWire *, int);
 
 
#endif /* __JxtaWire_H__  */
#ifdef __cplusplus
}
#endif