/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * $Id$
 */

#ifndef JXTA_STDPG_PRIVATE_H
#define JXTA_STDPG_PRIVATE_H

#include "jxta_peergroup_private.h"
#include "jxta_hashtable.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/*
 * Note: Jxta_stdpg does not normally need to be public, unless the
 * group has some extra features available to applications. That's why
 * there is no separate incomplete type declaration in a public
 * header file. Here we only export this type to subclassers.
 */
typedef struct _jxta_stdpg Jxta_stdpg;

struct _jxta_stdpg {

    Extends(Jxta_PG);

    /* Add data members here */

    Jxta_advertisement *impl_adv;
    Jxta_PG *home_group;
    Jxta_PA *config_adv;
    Jxta_PA *peer_adv;
    Jxta_PGA *group_adv;
    JString *name;
    JString *desc;

    Jxta_vector *resource_groups;

    /*
     * Shortcuts to well known services.
     */
    Jxta_endpoint_service *endpoint;
    Jxta_resolver_service *resolver;
    Jxta_discovery_service *discovery;
    Jxta_pipe_service *pipe;
    Jxta_membership_service *membership;
    Jxta_rdv_service *rendezvous;
    Jxta_peerinfo_service *peerinfo;
    Jxta_srdi_service *srdi;
    Jxta_cm *cm;

    /**
     * The tables of all custom-services 
     */
    Jxta_hashtable *services;        /** This hashtable contains (MSID, Jxta_module) */
    Jxta_hashtable *services_name;   /** This hashtable contains (MSID, code_to_load as jstring) */ 
    apr_hash_t     *services_handle; /** This hashtable contains (MSID, apr_handle to the lib) */ 
    apr_pool_t *pool;
};

/*
 * We do not need to subtype the base methods table, we're not adding to it.
 * However, typedef it to be nice to subclassers that expect the naming
 * to follow the regular scheme. Hopefully we should be able to get away
 * with just that and not be forced to always subclass the vtbl.
 *
 */
typedef Jxta_PG_methods Jxta_stdpg_methods;

/*
 * export our methods table to subclassers so that they can copy it
 * at run-time if useful.
 */
extern const Jxta_stdpg_methods jxta_stdpg_methods;

/*
 * export some "protected" methods to subclassers...
 */

/*
 * the constructor
 */
extern void jxta_stdpg_construct(Jxta_stdpg * self, Jxta_stdpg_methods const *methods);

/*
 * the destructor
 */
extern void jxta_stdpg_destruct(Jxta_stdpg * self);

/*
 * This routine inits the group but not its modules.
 * This is not part of the public API, this is for subclasses which may
 * need a finer grain control on the init sequence: specifically, they may
 * need to init additional services before the lot of them are started.
 */
void jxta_stdpg_init_group(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);

/*
 */
Jxta_status stdpg_ld_mod(Jxta_PG * self, Jxta_id * class_id, const char *name, Jxta_MIA * impl_adv, Jxta_module **);

/*
 * This routine just init the modules.
 * This is not part of the public API, this is for subclasses which may
 * need a finer grain control on the init sequence: specifically, they may
 * need to init additional services before the lot of them are started.
 *
 * Error-returning variant.
 */
Jxta_status jxta_stdpg_init_modules(Jxta_module * self);

/*
 * start the modules. This is not part of the public API, this is
 * for subclasses which may need a finer grain control on the init sequence.
 * specifically, the opportunity to init additional services before starting
 * ours, and then, their own.
 */
Jxta_status jxta_stdpg_start_modules(Jxta_module * self);

/**
 * Set the config adv for this peer group.
 * This is mainly used by net peer group
 * to give the stdpg group, access to the 
 * PlatformConfig file.
 */
void jxta_stdpg_set_configadv(Jxta_module * self, Jxta_PA * config_adv);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_STDPG_PRIVATE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
