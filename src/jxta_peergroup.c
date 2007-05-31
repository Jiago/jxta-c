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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: jxta_peergroup.c,v 1.30 2005/03/24 19:43:42 slowhog Exp $
 */

#include "apr_thread_proc.h"
#include "jpr/jpr_excep.h"
#include "jpr/jpr_threadonce.h"

#include "jxta_defloader_private.h"
#include "jxta_types.h"

/*
 * For now, jxta_peergroup.h fakes many of the types we use here.
 * Need to include appropriate headers in the future.
 */

#include "jxta_errno.h"
#include "jxta_id.h"
#include "jxta_peergroup_private.h"
#include "jxta_objecthashtable.h"

/*
 * This implementation supports only one instance of each group; the peer ID
 * is the same in each group that a given instance of Jxta joins.
 * To enforce that we need a global registry. This is probably the only
 * "global" (as in "not provided by a group") API in Jxta-C besides new_netpg.
 */
static Jxta_objecthashtable* groups_global_registry;

/*
 * Thread private data initializer control.
 */

static void
registry_init(void) {
    groups_global_registry = jxta_objecthashtable_new(4,
                             (Jxta_object_hash_func) jxta_id_hashcode,
                             (Jxta_object_equals_func) jxta_id_equals);
}

/*
 * To be clean, groups should register before they complete
 * initialization, but they should have their get_interface routine
 * block until intialization is complete. That way we avoid letting
 * two redundant instances init in parallel and then have one to abort late.
 * If a group attempts to register and gets kicked-out, it should
 * abort initialization and return JXTA_ITEM_EXISTS.
 */
Jxta_status jxta_register_group_instance(Jxta_id* gid, Jxta_PG* pg) {
    return jxta_objecthashtable_putnoreplace(groups_global_registry,
            (Jxta_object*) gid,
            (Jxta_object*) pg);
}

/*
 * Will remove the given GID/group pair from the global group registry
 * if such a pair is found.
 * In the future, unprivileged applications may obtain an interface
 * object to each group that is private and distinct from the group
 * object itself. Checking the object being removed will prevent such
 * applications from removing groups from the registry.
 */
Jxta_status jxta_unregister_group_instance(Jxta_id* gid, Jxta_PG* pg) {
    return jxta_objecthashtable_delcheck(groups_global_registry,
                                         (Jxta_object*) gid,
                                         (Jxta_object*) pg);
}

/*
 * Lookup a group by ID.
 */
Jxta_status jxta_lookup_group_instance(Jxta_id* gid, Jxta_PG** pg) {
    Jxta_service* tmp;
    Jxta_status res;

    res = jxta_objecthashtable_get(groups_global_registry,
                                   (Jxta_object*) gid,
                                   (Jxta_object**) &tmp);

    if (res != JXTA_SUCCESS)
        return res;
    jxta_service_get_interface(tmp, (Jxta_service**) pg);
    JXTA_OBJECT_RELEASE(tmp);

    return res;
}

/**
 * Well known module class identifier: peer group
 */
static Jxta_MCID* _peergroup_classid;

/**
 * Well known module class identifier: resolver service
 */
static Jxta_MCID* _resolver_classid;

/**
 * Well known module class identifier: discovery service
 */
static Jxta_MCID* _discovery_classid;

/**
 * Well known module class identifier: pipe service
 */
static Jxta_MCID* _pipe_classid;

/**
 * Well known module class identifier: membership service
 */
static Jxta_MCID* _membership_classid;

/**
 * Well known module class identifier: rendezvous service
 */
static Jxta_MCID* _rendezvous_classid;

/**
 * Well known module class identifier: peerinfo service
 */
static Jxta_MCID* _peerinfo_classid;

/**
 * Well known module class identifier: endpoint service
 */
static Jxta_MCID* _endpoint_classid;

/*
 * fixme: endpointprotocols should probably all be of the same class
 * and of different specs and roles... but we'll take a shortcut for now.
 */

/**
 * Well known module class identifier: tcp protocol
 */
static Jxta_MCID* _tcpproto_classid;

/**
 * Well known module class identifier: http protocol
 */
static Jxta_MCID* _httpproto_classid;

/**
 * Well known module class identifier: router protocol
 */
static Jxta_MCID* _routerproto_classid;

/**
 * Well known module class identifier: relay protocol
 */
static Jxta_MCID* _relayproto_classid;

/**
 * Well known module class identifier: application
 */
static Jxta_MCID* _application_classid;

/**
 * Well known module class identifier: tlsprotocol
 */
static Jxta_MCID* _tlsproto_classid;

/**
 * Well known group specification identifier: the platform
 */
static Jxta_MSID* _ref_platform_specid;

/**
 * Well known group specification identifier: the network peer group
 */
static Jxta_MSID* _ref_netpeergroup_specid;

/**
 * Well known service specification identifier: the standard resolver
 */
static Jxta_MSID* _ref_resolver_specid;

/**
 * Well known service specification identifier: the standard discovery
 */
static Jxta_MSID* _ref_discovery_specid;

/**
 * Well known service specification identifier: the standard pipe
 */
static Jxta_MSID* _ref_pipe_specid;

/**
 * Well known service specification identifier: the standard membership
 */
static Jxta_MSID* _ref_membership_specid;

/**
 * Well known service specification identifier: the standard rendezvous
 */
static Jxta_MSID* _ref_rendezvous_specid;

/**
 * Well known service specification identifier: the standard peerinfo
 */
static Jxta_MSID* _ref_peerinfo_specid;

/**
 * Well known service specification identifier: the standard endpoint
 */
static Jxta_MSID* _ref_endpoint_specid;

/**
 * Well known TCP/IP protocol specification identifier: the standard
 * tcp endpoint protocol
 */
static Jxta_MSID* _ref_tcpproto_specid;

/**
 * Well known HTTP protocol specification identifier: the standard
 * http endpoint protocol
 */
static Jxta_MSID* _ref_httpproto_specid;

/**
 * Well known router protocol specification identifier: the standard
 * router
 */
static Jxta_MSID* _ref_routerproto_specid;

/**
 * Well known relay protocol specification identifier: the standard
 * router
 */
static Jxta_MSID* _ref_relayproto_specid;

/**
 * Well known tls protocol specification identifier: the standard
 * tls endpoint protocol
 */
static Jxta_MSID* _ref_tlsproto_specid;

/**
 * Well known group specification identifier: an all purpose peer group
 * specification. the java reference implementation implements it with
 * the stdpeergroup class and all the standard platform services and no
 * endpoint protocols.
 */
static Jxta_MSID* _genericpeergroup_specid;

/**
 * Well known main application of the platform: startnetpeergroup.
 */
static Jxta_MSID* _ref_startnetpeergroup_specid;

/**
 * Well known application: the shell
 */
static Jxta_MSID* _ref_shell_specid;

/**
 * Used to create static well-known identifiers.
 */
static Jxta_id* buildWellKnownID(const char* s, Throws) {
    Jxta_id* result = NULL;
    JString* tmp = jstring_new_2("urn:jxta:uuid-");
    jstring_append_2(tmp, s);
    jxta_id_from_jstring( &result, tmp);
    JXTA_OBJECT_RELEASE(tmp);
    if (result == NULL)
        Throw(JXTA_INVALID_ARGUMENT);
    return result;
}

/**
 * Well known classes for the basic services.
 * FIXME: we should make a "well-known ID" encoding implementation that
 * has its own little name space of human readable names...later.
 * To keep their string representation shorter, we put our small spec
 * or role pseudo unique ID at the front of the second UUID string.
 * Base classes do not need an explicit second UUID string because it is
 * all 0.
 * The type is always the last two characters, nomatter the total length.
 */

static void static_id_init(void) {

    static int done = 0;

    if (done)
        return;

    Try {

        /**
         * Well known module class identifier: peer group
         */
        _peergroup_classid = (Jxta_MCID*)
                             buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000001"
                                               "05", MayThrow);

        /**
         * Well known module class identifier: resolver service
         */
        _resolver_classid = (Jxta_MCID*)
                            buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000002"
                                              "05", MayThrow);

        /**
         * Well known module class identifier: discovery service
         */
        _discovery_classid = (Jxta_MCID*)
                             buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000003"
                                               "05", MayThrow);

        /**
         * Well known module class identifier: pipe service
         */
        _pipe_classid = (Jxta_MCID*)
                        buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000004"
                                          "05", MayThrow);

        /**
         * Well known module class identifier: membership service
         */
        _membership_classid = (Jxta_MCID*)
                              buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000005"
                                                "05", MayThrow);

        /**
         * Well known module class identifier: rendezvous service
         */
        _rendezvous_classid = (Jxta_MCID*)
                              buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000006"
                                                "05", MayThrow);

        /**
         * Well known module class identifier: peerinfo service
         */
        _peerinfo_classid = (Jxta_MCID*)
                            buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000007"
                                              "05", MayThrow);

        /**
         * Well known module class identifier: endpoint service
         */
        _endpoint_classid = (Jxta_MCID*)
                            buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000008"
                                              "05", MayThrow);

        /*
         * FIXME: EndpointProtocols should probably all be of the same class
         * and of different specs and roles... But we'll take a shortcut for now.
         */

        /**
         * Well known module class identifier: tcp protocol
         */
        _tcpproto_classid = (Jxta_MCID*)
                            buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000009"
                                              "05", MayThrow);

        /**
         * Well known module class identifier: http protocol
         */
        _httpproto_classid = (Jxta_MCID*)
                             buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000A"
                                               "05", MayThrow);
        /**
         * Well known module class identifier: relay protocol
         */
        _relayproto_classid = (Jxta_MCID*)
                             buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000F"
                                               "05", MayThrow);
        /**
         * Well known module class identifier: router protocol
         */
        _routerproto_classid = (Jxta_MCID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000B"
                                                 "05", MayThrow);

        /**
         * Well known module class identifier: application
         */
        _application_classid = (Jxta_MCID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000C"
                                                 "05", MayThrow);

        /**
         * Well known module class identifier: tlsProtocol
         */
        _tlsproto_classid = (Jxta_MCID*)
                            buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000D"
                                              "05", MayThrow);

        /**
         * Well known group specification identifier: the platform
         */
        _ref_platform_specid = (Jxta_MSID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000001"
                                                 "01"
                                                 "06", MayThrow);

        /**
         * Well known group specification identifier: the Network Peer Group
         */
        _ref_netpeergroup_specid = (Jxta_MSID*)
                                   buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000001"
                                                     "02"
                                                     "06", MayThrow);

        /**
         * Well known service specification identifier: the standard resolver
         */
        _ref_resolver_specid = (Jxta_MSID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000002"
                                                 "01"
                                                 "06", MayThrow);

        /**
         * Well known service specification identifier: the standard discovery
         */
        _ref_discovery_specid = (Jxta_MSID*)
                                buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000003"
                                                  "01"
                                                  "06", MayThrow);

        /**
         * Well known service specification identifier: the standard pipe
         */
        _ref_pipe_specid = (Jxta_MSID*)
                           buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000004"
                                             "01"
                                             "06", MayThrow);

        /**
         * Well known service specification identifier: the standard membership
         */
        _ref_membership_specid = (Jxta_MSID*)
                                 buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000005"
                                                   "01"
                                                   "06", MayThrow);

        /**
         * Well known service specification identifier: the standard rendezvous
         */
        _ref_rendezvous_specid = (Jxta_MSID*)
                                 buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000006"
                                                   "01"
                                                   "06", MayThrow);

        /**
         * Well known service specification identifier: the standard peerinfo
         */
        _ref_peerinfo_specid = (Jxta_MSID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000007"
                                                 "01"
                                                 "06", MayThrow);

        /**
         * Well known service specification identifier: the standard endpoint
         */
        _ref_endpoint_specid = (Jxta_MSID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000008"
                                                 "01"
                                                 "06", MayThrow);

        /**
         * Well known endpoint protocol specification identifier: the standard
         * tcp endpoint protocol
         */
        _ref_tcpproto_specid = (Jxta_MSID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000009"
                                                 "01"
                                                 "06", MayThrow);

        /**
         * Well known endpoint protocol specification identifier: the standard
         * http endpoint protocol
         */
        _ref_httpproto_specid = (Jxta_MSID*)
                                buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000A"
                                                  "01"
                                                  "06", MayThrow);

        /**
         * Well known endpoint protocol specification identifier: the standard
         * router
         */
        _ref_routerproto_specid = (Jxta_MSID*)
                                  buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000B"
                                                    "01"
                                                    "06", MayThrow);

        /**
         * Well known endpoint protocol specification identifier: the standard
         * tls endpoint protocol
         */
        _ref_tlsproto_specid = (Jxta_MSID*)
                               buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000D"
                                                 "01"
                                                 "06", MayThrow);

        /**
         * Well known main application of the platform: startNetPeerGroup.
         */
        _ref_startnetpeergroup_specid = (Jxta_MSID*)
                                        buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000C"
                                                          "01"
                                                          "06", MayThrow);

        /**
         * Well known application: the shell
         */
        _ref_shell_specid = (Jxta_MSID*)
                            buildWellKnownID( "DeadBeefDeafBabaFeedBabe0000000C"
                                              "02"
                                              "06", MayThrow);

        /**
         * Well known group specification identifier: an all purpose peer group
         * specification. The java reference implementation implements it with
         * the StdPeerGroup class and all the standard platform services and no
         * endpoint protocols.
         */
        _genericpeergroup_specid = (Jxta_MSID*)
                                   buildWellKnownID( "DeadBeefDeafBabaFeedBabe00000001"
                                                     "03"
                                                     "06", MayThrow);

        done = 1;
    } Catch {
        printf("Malformed Hardcoded well-known IDs. Error %lu\n",
               jpr_lasterror_get());
        abort();
    }
}


/**
 * get each well known ID.
 */
Jxta_MCID* jxta_peergroup_classid_get           (void) {
    return _peergroup_classid;
}
Jxta_MCID* jxta_resolver_classid_get            (void) {
    return _resolver_classid;
}
Jxta_MCID* jxta_discovery_classid_get           (void) {
    return _discovery_classid;
}
Jxta_MCID* jxta_pipe_classid_get                (void) {
    return _pipe_classid;
}
Jxta_MCID* jxta_membership_classid_get          (void) {
    return _membership_classid;
}
Jxta_MCID* jxta_rendezvous_classid_get          (void) {
    return _rendezvous_classid;
}
Jxta_MCID* jxta_peerinfo_classid_get            (void) {
    return _peerinfo_classid;
}
Jxta_MCID* jxta_endpoint_classid_get            (void) {
    return _endpoint_classid;
}
Jxta_MCID* jxta_tcpproto_classid_get            (void) {
    return _tcpproto_classid;
}
Jxta_MCID* jxta_httpproto_classid_get           (void) {
    return _httpproto_classid;
}
Jxta_MCID* jxta_relayproto_classid_get          (void) {
    return _relayproto_classid;
}
Jxta_MCID* jxta_routerproto_classid_get         (void) {
    return _routerproto_classid;
}
Jxta_MCID* jxta_application_classid_get         (void) {
    return _application_classid;
}
Jxta_MCID* jxta_tlsproto_classid_get            (void) {
    return _tlsproto_classid;
}
Jxta_MSID* jxta_ref_platform_specid_get         (void) {
    return _ref_platform_specid;
}
Jxta_MSID* jxta_ref_netpeergroup_specid_get     (void) {
    return _ref_netpeergroup_specid;
}
Jxta_MSID* jxta_ref_resolver_specid_get         (void) {
    return _ref_resolver_specid;
}
Jxta_MSID* jxta_ref_discovery_specid_get        (void) {
    return _ref_discovery_specid;
}
Jxta_MSID* jxta_ref_pipe_specid_get             (void) {
    return _ref_pipe_specid;
}
Jxta_MSID* jxta_ref_membership_specid_get       (void) {
    return _ref_membership_specid;
}
Jxta_MSID* jxta_ref_rendezvous_specid_get       (void) {
    return _ref_rendezvous_specid;
}
Jxta_MSID* jxta_ref_peerinfo_specid_get         (void) {
    return _ref_peerinfo_specid;
}
Jxta_MSID* jxta_ref_endpoint_specid_get         (void) {
    return _ref_endpoint_specid;
}
Jxta_MSID* jxta_ref_tcpproto_specid_get         (void) {
    return _ref_tcpproto_specid;
}
Jxta_MSID* jxta_ref_httpproto_specid_get        (void) {
    return _ref_httpproto_specid;
}
Jxta_MSID* jxta_ref_relayproto_specid_get       (void) {
    return _ref_relayproto_specid;
}
Jxta_MSID* jxta_ref_routerproto_specid_get      (void) {
    return _ref_routerproto_specid;
}
Jxta_MSID* jxta_ref_tlsproto_specid_get         (void) {
    return _ref_tlsproto_specid;
}
Jxta_MSID* jxta_ref_startnetpeergroup_specid_get(void) {
    return _ref_startnetpeergroup_specid;
}
Jxta_MSID* jxta_ref_shell_specid_get            (void) {
    return _ref_shell_specid;
}
Jxta_MSID* jxta_genericpeergroup_specid_get     (void) {
    return _genericpeergroup_specid;
}


/**
 * The base PG ctor (not public: the only public way to make a new pg 
 * is to instantiate one of the derived types).
 */
void jxta_PG_construct(Jxta_PG* self, Jxta_PG_methods* methods) {

    PTValid(methods, Jxta_PG_methods);
    jxta_service_construct((Jxta_service*) self,
                           (Jxta_service_methods*) methods);
    self->thisType = "Jxta_PG";
}

/**
 * The base PG dtor (Not public, not virtual. Only called by subclassers).
 * We just pass it along.
 */
void jxta_PG_destruct(Jxta_PG* self) {
    jxta_service_destruct((Jxta_service*) self);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_PG_methods*) JXTA_MODULE_VTBL(self))

void jxta_PG_get_loader(Jxta_PG* self, Jxta_loader** loader ) {
    PTValid(self, Jxta_PG);
    (VTBL->get_loader)(self, loader);
}

void jxta_PG_get_PGA(Jxta_PG* self, Jxta_PGA** pga) {
    PTValid(self, Jxta_PG);
    (VTBL->get_PGA)(self, pga);
}

void jxta_PG_get_PA(Jxta_PG* self, Jxta_PA** pa) {
    PTValid(self, Jxta_PG);
    (VTBL->get_PA)(self, pa);
}

Jxta_status jxta_PG_lookup_service(Jxta_PG* self, Jxta_id* name,
                                   Jxta_service** result) {
    PTValid(self, Jxta_PG);
    return (VTBL->lookup_service)(self, name, result);
}

void jxta_PG_lookup_service_e(Jxta_PG* self, Jxta_id* name,
                              Jxta_service** result, Throws) {
    ThrowThrough();
    PTValid(self, Jxta_PG);
    (VTBL->lookup_service_e)(self, name, result, MayThrow);
}

Jxta_boolean jxta_PG_is_compatible(Jxta_PG* self, JString* compat) {
    PTValid(self, Jxta_PG);
    return (VTBL->is_compatible)(self, compat);
}

Jxta_status jxta_PG_loadfromimpl_module(Jxta_PG* self, Jxta_id* assigned_id,
                                        Jxta_advertisement* impl,
                                        Jxta_module** result) {
    PTValid(self, Jxta_PG);
    return (VTBL->loadfromimpl_module)(self, assigned_id, impl,
                                       result);
}

void jxta_PG_loadfromimpl_module_e(Jxta_PG* self, Jxta_id* assigned_id,
                                   Jxta_advertisement* impl,
                                   Jxta_module** mod, Throws) {
    ThrowThrough();
    PTValid(self, Jxta_PG);
    (VTBL->loadfromimpl_module_e)(self, assigned_id, impl, mod, MayThrow);
}

Jxta_status jxta_PG_loadfromid_module(Jxta_PG* self, Jxta_id* assigned_id,
                                      Jxta_MSID* spec_id, int where,
                                      Jxta_module** result) {
    PTValid(self, Jxta_PG);
    return (VTBL->loadfromid_module)(self, assigned_id,
                                     spec_id, where, result);
}

void jxta_PG_loadfromid_module_e(Jxta_PG* self, Jxta_id* assigned_id,
                                 Jxta_MSID* spec_id, int where,
                                 Jxta_module** mod, Throws) {
    ThrowThrough();
    PTValid(self, Jxta_PG);
    (VTBL->loadfromid_module_e)(self, assigned_id, spec_id, where,
                                mod, MayThrow);

}

void jxta_PG_set_labels(Jxta_PG* self, JString* name, JString* description) {
    PTValid(self, Jxta_PG);
    (VTBL->set_labels)(self, name, description);
}

Jxta_status jxta_PG_newfromadv(Jxta_PG* self, Jxta_advertisement* pgAdv,
                               Jxta_vector* resource_groups,
                               Jxta_PG** result) {
    PTValid(self, Jxta_PG);
    return (VTBL->newfromadv)(self, pgAdv, resource_groups, result);
}

void jxta_PG_newfromadv_e(Jxta_PG* self,
                          Jxta_advertisement* pgAdv,
                          Jxta_vector* resource_groups,
                          Jxta_PG** pg,
                          Throws) {
    ThrowThrough();
    PTValid(self, Jxta_PG);
    (VTBL->newfromadv_e)(self, pgAdv, resource_groups, pg, MayThrow);
}

Jxta_status jxta_PG_newfromimpl(Jxta_PG* self, Jxta_PGID* gid,
                                Jxta_advertisement* impl, JString* name,
                                JString* description,
                                Jxta_vector* resource_groups,
                                Jxta_PG** result) {
    PTValid(self, Jxta_PG);
    return (VTBL->newfromimpl)(self, gid, impl, name,
                               description, resource_groups, result);
}

void jxta_PG_newfromimpl_e(Jxta_PG* self, Jxta_PGID* gid,
                           Jxta_advertisement* impl, JString* name,
                           JString* description,
                           Jxta_vector* resource_groups,
                           Jxta_PG** pg,
                           Throws) {
    ThrowThrough();
    PTValid(self, Jxta_PG);
    (VTBL->newfromimpl_e)(self, gid, impl, name,description, resource_groups, pg,
                          MayThrow);
}

Jxta_status jxta_PG_newfromid(Jxta_PG* self, Jxta_PGID* gid,
                              Jxta_vector* resource_groups,
                              Jxta_PG** result) {
    PTValid(self, Jxta_PG);
    return (VTBL->newfromid)(self, gid, resource_groups, result);
}

void jxta_PG_newfromid_e(Jxta_PG* self, Jxta_PGID* gid,
                         Jxta_vector* resource_groups,Jxta_PG** pg,
                         Throws) {
    ThrowThrough();
    PTValid(self, Jxta_PG);
    (VTBL->newfromid_e)(self, gid, resource_groups, pg, MayThrow);
}

void jxta_PG_get_rendezvous_service(Jxta_PG* self, Jxta_rdv_service** rdv) {
    PTValid(self, Jxta_PG);
    (VTBL->get_rendezvous_service)(self, rdv);
}

void jxta_PG_get_endpoint_service(Jxta_PG* self, Jxta_endpoint_service** endp) {
    PTValid(self, Jxta_PG);
    (VTBL->get_endpoint_service)(self, endp);
}

void jxta_PG_get_resolver_service(Jxta_PG* self, Jxta_resolver_service** res) {
    PTValid(self, Jxta_PG);
    (VTBL->get_resolver_service)(self, res);
}

void jxta_PG_get_discovery_service(Jxta_PG* self, Jxta_discovery_service** disco) {
    PTValid(self, Jxta_PG);
    (VTBL->get_discovery_service)(self, disco);
}

void jxta_PG_get_peerinfo_service(Jxta_PG* self, Jxta_peerinfo_service** peerinfo) {
    PTValid(self, Jxta_PG);
    (VTBL->get_peerinfo_service)(self, peerinfo);
}

void jxta_PG_get_membership_service(Jxta_PG* self, Jxta_membership_service** memb) {
    PTValid(self, Jxta_PG);
    (VTBL->get_membership_service)(self, memb);
}

void jxta_PG_get_pipe_service(Jxta_PG* self, Jxta_pipe_service** pipe) {
    PTValid(self, Jxta_PG);
    (VTBL->get_pipe_service)(self, pipe);
}

void jxta_PG_get_GID(Jxta_PG* self, Jxta_PGID** gid) {
    PTValid(self, Jxta_PG);
    (VTBL->get_GID)(self, gid);
}

void jxta_PG_get_PID(Jxta_PG* self, Jxta_PID** pid) {
    PTValid(self, Jxta_PG);
    (VTBL->get_PID)(self, pid);
}

void jxta_PG_get_groupname(Jxta_PG* self, JString** nm) {
    PTValid(self, Jxta_PG);
    (VTBL->get_groupname)(self, nm);
}

void jxta_PG_get_peername(Jxta_PG* self, JString** nm) {
    PTValid(self, Jxta_PG);
    (VTBL->get_peername)(self, nm);
}

void jxta_PG_get_configadv(Jxta_PG* self, Jxta_PA** adv) {
    PTValid(self, Jxta_PG);
    (VTBL->get_configadv)(self, adv);
}

void jxta_PG_get_genericpeergroupMIA(Jxta_PG* self, Jxta_MIA** mia) {
    PTValid(self, Jxta_PG);
    (VTBL->get_genericpeergroupMIA)(self, mia);
}

void jxta_PG_set_resourcegroups(Jxta_PG* self,
                                Jxta_vector* resource_groups) {
    PTValid(self, Jxta_PG);
    (VTBL->set_resourcegroups)(self, resource_groups);
}

void jxta_PG_get_resourcegroups(Jxta_PG* self,
                                Jxta_vector** resource_groups) {
    PTValid(self, Jxta_PG);
    (VTBL->get_resourcegroups)(self, resource_groups);
}

void jxta_PG_get_parentgroup(Jxta_PG* self,
                             Jxta_PG** parent_group) {
    PTValid(self, Jxta_PG);
    (VTBL->get_parentgroup)(self, parent_group);
}

void jxta_PG_get_compatstatement(Jxta_PG* self,
                                 JString** compat) {
    PTValid(self, Jxta_PG);
    (VTBL->get_compatstatement)(self, compat);
}

Jxta_status jxta_PG_new_netpg(Jxta_PG** new_netpg) {
    Jxta_PG* npg = NULL;
    Jxta_status res = JXTA_SUCCESS;
    char* noargs[] = { NULL };

    Try {
        npg = (Jxta_PG*)
              jxta_defloader_instantiate_e("builtin:netpg", MayThrow);

        jxta_module_init_e((Jxta_module*) npg,  (Jxta_PG*) 0,
                           (Jxta_id*) 0, 0, MayThrow);

        res = jxta_module_start((Jxta_module*) npg, noargs );

        if( JXTA_SUCCESS != res )
        Throw(res);
    } Catch {
        if (npg != NULL )
            JXTA_OBJECT_RELEASE(npg);
            return jpr_lasterror_get();
        }

        *new_netpg = npg;
return res;
}

Jxta_status jxta_PG_add_relay_address(Jxta_PG* self, Jxta_RdvAdvertisement* relay) {
    PTValid(self, Jxta_PG);
    return (VTBL->add_relay_address)(self, relay);
}


Jxta_status jxta_PG_remove_relay_address(Jxta_PG* self, Jxta_id* relayid) {
    PTValid(self, Jxta_PG);
    return (VTBL->remove_relay_address)(self, relayid);
}

Jxta_status jxta_PG_module_initialize(void)
{
    registry_init();
    static_id_init();

    return JXTA_SUCCESS;
}

void jxta_PG_module_terminate(void)
{
    JXTA_OBJECT_RELEASE(groups_global_registry);
    groups_global_registry = NULL;

    JXTA_OBJECT_RELEASE(_peergroup_classid);
    JXTA_OBJECT_RELEASE(_resolver_classid);
    JXTA_OBJECT_RELEASE(_discovery_classid);
    JXTA_OBJECT_RELEASE(_pipe_classid);
    JXTA_OBJECT_RELEASE(_membership_classid);
    JXTA_OBJECT_RELEASE(_rendezvous_classid);
    JXTA_OBJECT_RELEASE(_peerinfo_classid);
    JXTA_OBJECT_RELEASE(_endpoint_classid);
    JXTA_OBJECT_RELEASE(_tcpproto_classid);
    JXTA_OBJECT_RELEASE(_httpproto_classid);
    JXTA_OBJECT_RELEASE(_relayproto_classid);
    JXTA_OBJECT_RELEASE(_routerproto_classid);
    JXTA_OBJECT_RELEASE(_application_classid);
    JXTA_OBJECT_RELEASE(_tlsproto_classid);
    JXTA_OBJECT_RELEASE(_ref_platform_specid);
    JXTA_OBJECT_RELEASE(_ref_netpeergroup_specid);
    JXTA_OBJECT_RELEASE(_ref_resolver_specid);
    JXTA_OBJECT_RELEASE(_ref_discovery_specid);
    JXTA_OBJECT_RELEASE(_ref_pipe_specid);
    JXTA_OBJECT_RELEASE(_ref_membership_specid);
    JXTA_OBJECT_RELEASE(_ref_rendezvous_specid);
    JXTA_OBJECT_RELEASE(_ref_peerinfo_specid);
    JXTA_OBJECT_RELEASE(_ref_endpoint_specid);
    JXTA_OBJECT_RELEASE(_ref_tcpproto_specid);
    JXTA_OBJECT_RELEASE(_ref_httpproto_specid);
    JXTA_OBJECT_RELEASE(_ref_routerproto_specid);
    JXTA_OBJECT_RELEASE(_ref_tlsproto_specid);
    JXTA_OBJECT_RELEASE(_ref_startnetpeergroup_specid);
    JXTA_OBJECT_RELEASE(_ref_shell_specid);
    JXTA_OBJECT_RELEASE(_genericpeergroup_specid);
}

/* vim: set ts=4 sw=4 tw=130 et: */
