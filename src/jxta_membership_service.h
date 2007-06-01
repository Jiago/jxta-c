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
 * $Id: jxta_membership_service.h,v 1.7 2005/09/21 21:16:47 slowhog Exp $
 */


#ifndef __Jxta_Membership_Service_H__
#define __Jxta_Membership_Service_H__

#include "jxta_service.h"

#include "jxta_vector.h"
#include "jxta_cred.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_membership_service const Jxta_membership_service;

typedef struct _jxta_membership_authenticator const Jxta_membership_authenticator;

JXTA_DECLARE(Jxta_status) jxta_membership_service_apply(Jxta_membership_service * self, Jxta_credential * authCred,
                                                        Jxta_membership_authenticator ** auth);

JXTA_DECLARE(Jxta_status) jxta_membership_service_join(Jxta_membership_service * self, Jxta_membership_authenticator * auth,
                                                       Jxta_credential ** newcred);

JXTA_DECLARE(Jxta_status) jxta_membership_service_resign(Jxta_membership_service * self);

JXTA_DECLARE(Jxta_status) jxta_membership_service_get_currentcreds(Jxta_membership_service * self, Jxta_vector ** creds);

JXTA_DECLARE(Jxta_status) jxta_membership_service_makecred(Jxta_membership_service * self, JString * somecred,
                                                           Jxta_credential ** cred);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __Jxta_Membership_Service_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
