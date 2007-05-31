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
 * $Id: jxta_cred.c,v 1.3 2005/01/12 21:46:56 bondolo Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_cred.h"
#include "jxta_cred_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

    Jxta_status
    jxta_credential_get_xml( Jxta_advertisement *ad , Jxta_write_func func, void* stream ) {
        Jxta_credential_mutable* self = (Jxta_credential_mutable*) ad;
        
        if( !JXTA_OBJECT_CHECK_VALID(self) )
            return JXTA_INVALID_ARGUMENT;
            
        return (self->credfuncs->cred_getxml)( self, func, stream );
        }

    Jxta_status
    jxta_credential_get_xml_1( Jxta_advertisement *ad, Jxta_write_func func, void* stream ) {
        Jxta_credential_mutable* self = (Jxta_credential_mutable*) ad;
        
        if( !JXTA_OBJECT_CHECK_VALID(self) )
            return JXTA_INVALID_ARGUMENT;
            
        return (self->credfuncs->cred_getxml_1)( self, func, stream );
        }

    Jxta_status
    jxta_credential_parse_charbuffer( Jxta_credential *cred , const char * buff, int len ) {
        Jxta_credential_mutable* self = (Jxta_credential_mutable*) cred;
        
        if( !JXTA_OBJECT_CHECK_VALID(self) )
            return JXTA_INVALID_ARGUMENT;
            
        return JXTA_NOTIMP;
        }

    
    Jxta_status
    jxta_credential_parse_file( Jxta_credential * cred , FILE * stream ) {
        Jxta_credential_mutable* self = (Jxta_credential_mutable*) cred;
        
        if( !JXTA_OBJECT_CHECK_VALID(self) )
            return JXTA_INVALID_ARGUMENT;
            
        return JXTA_NOTIMP;
        }

    Jxta_status
    jxta_credential_get_peergroupid( Jxta_credential * cred , Jxta_id** pg ) {
        Jxta_credential_mutable* self = (Jxta_credential_mutable*) cred;
        
        if( !JXTA_OBJECT_CHECK_VALID(self) )
            return JXTA_INVALID_ARGUMENT;
        
        return (self->credfuncs->cred_getpeergroup)( cred, pg );
        }

    Jxta_status
    jxta_credential_get_peerid( Jxta_credential * cred , Jxta_id** peer ) {
        Jxta_credential_mutable* self = (Jxta_credential_mutable*) cred;
        
        if( !JXTA_OBJECT_CHECK_VALID(self) )
            return JXTA_INVALID_ARGUMENT;
            
        return (self->credfuncs->cred_getpeer)( cred, peer );
        }

    Jxta_status
    jxta_credential_get_source( Jxta_credential * cred, Jxta_service** svc ) {
        Jxta_credential_mutable* self = (Jxta_credential_mutable*) cred;
        
        if( !JXTA_OBJECT_CHECK_VALID(self) )
            return JXTA_INVALID_ARGUMENT;
            
        return (self->credfuncs->cred_getsource)( cred, svc );
        }

#ifdef __cplusplus
}
#endif
