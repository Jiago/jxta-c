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
 * $Id: jxta_peerinfo_service.h,v 1.4 2005/06/16 23:11:46 slowhog Exp $
 */



#ifndef JXTA_PEERINFO_SERVICE_H
#define JXTA_PEERINFO_SERVICE_H

#include "jxta_service.h"

#include "jxta_object.h"
#include "jxta_id.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
typedef struct _jxta_peerinfo_service Jxta_peerinfo_service;

typedef struct _PeerinfoEvent PeerinfoEvent;
typedef struct _Jxta_peerinfo_listener Jxta_peerinfo_listener;

/**
 * Get PeerInfoService from a remote peer.
 * @param  Jxta_peerinfo_service the service 
 * @param  peer Address of a peer, or portal to connect to, if address is null
 * @return  Jxta_status
 */
Jxta_status
peerinfo_service_get_remote_peerinfo(Jxta_peerinfo_service * service, Jxta_id * peerid, Jxta_peerinfo_listener * listener);

/**
 * Get a PeerInfo from local cache.
 * @param  Jxta_peerinfo_service the service 
 * @param  peer peer ID of a peer
 * @param  adv pointer to the adv to return
 * @return  Jxta_status
 */

Jxta_status peerinfo_service_get_local_peerinfo(Jxta_peerinfo_service * service, Jxta_id * peerid, Jxta_object ** adv);

/**
 * Retrieve this peer's PeerInfo Advertisement
 * @param  Jxta_peerinfo_service the service 
 * @return Jxta_status
 * @see Jxta_status
 */
Jxta_status peerinfo_service_get_my_peerinfo(Jxta_peerinfo_service * service, Jxta_object ** adv);

/**
 * flush s stored Document
 * @param  Jxta_peerinfo_service the service 
 * @param  peer peer ID of a peerinfo adv to flush
 * @return Jxta_status
 * @see Jxta_status
 */
Jxta_status peerinfo_service_flush_advertisement(Jxta_peerinfo_service * service, Jxta_id * peerid);

/**
 * register a peerinfo listener, to notified on peerinfo events
 * @param  Jxta_peerinfo_service the service 
 * @param  Jxta_peerinfo_listener the peer info listener to register
 * @return Jxta_status
 * @see Jxta_status
 */
Jxta_status peerinfo_service_add_peerinfo_listener(Jxta_peerinfo_service * service, Jxta_peerinfo_listener * listener);

/**
 * remove a peerinfo listener
 * @param  Jxta_peerinfo_service the service 
 * @param  Jxta_peerinfo_listener the peer info listener to remove
 * @return Jxta_status
 * @see Jxta_status
 */
Jxta_status peerinfo_service_remove_peerinfo_listener(Jxta_peerinfo_service * service, Jxta_peerinfo_listener * listener);
#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif

/* vi: set ts=4 sw=4 tw=130 et: */
