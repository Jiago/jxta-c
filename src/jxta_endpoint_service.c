/* 
 * Copyright (c) 2002-2006 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id$
 */

static const char *__log_cat = "ENDPOINT";

#include <stdlib.h> /* malloc, free */
#include <assert.h>

#include "jxta_apr.h"

#include "jpr/jpr_excep_proto.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "queue.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_service_private.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_endpoint_service.h"
#include "jxta_endpoint_address.h"
#include "jxta_router_service.h"
#include "jxta_listener.h"
#include "jxta_peergroup.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_stdpg_private.h"
#include "jxta_routea.h"
#include "jxta_log.h"
#include "jxta_apr.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_util_priv.h"

/* negative cache entry */
typedef struct _nc_entry {
    char *addr;
    Jxta_time_diff timeout;
    Jxta_time expiration;
    Jxta_message **rq;
    int rq_head;
    int rq_tail;
    int rq_capacity;
} Nc_entry;

/* outgoing message task for thread pool */
typedef struct _msg_task {
    Jxta_endpoint_service *ep_svc;
    Jxta_message *msg;
    struct _msg_task *recycle;
} Msg_task;

/* recipient callback */
typedef struct _cb_elt {
    Jxta_endpoint_service *ep_svc;
    Jxta_callback_func func;
    void *arg;
    char *recipient;
    struct _cb_elt *recycle;
} Cb_elt;

/* transport connection */
typedef struct tc_elt {
    apr_pollfd_t fd;
    Jxta_callback_fn fn;
    void * arg;
} Tc_elt;

typedef struct peer_route_elt {
    char * ta;
    JxtaEndpointMessenger * msgr;
} Peer_route_elt;

struct jxta_endpoint_service {
    Extends(Jxta_service);

    apr_thread_mutex_t *mutex;

    Jxta_PG *my_group;
    JString *my_groupid;
    Jxta_advertisement *my_impl_adv;
    Jxta_EndPointConfigAdvertisement *config;
    Jxta_RouteAdvertisement *myRoute;
    char *relay_addr;
    char *relay_proto;
    Jxta_transport *router_transport;
    Jxta_hashtable *transport_table;

    /* Current messengers */
    /* Each messenger could have two entries, one for peer ID, the other with transport addr */
    apr_hash_t *messengers;

    /* Outgoing messages */
    Msg_task *recycled_tasks;
    volatile apr_uint32_t msg_task_cnt;

    /* Negatice cache */
    apr_thread_mutex_t *nc_wlock; /* write lock for nc, use mutex for read lock */
    apr_hash_t *nc;             /* negative cache */

    /* Endpoint demux */
    apr_thread_mutex_t *demux_mutex;
    apr_hash_t *cb_table;       /* callback table for recipients */
    Cb_elt *recycled_cbs;
    Dlist *filter_list;
    apr_hash_t *listener_table;

    /* Nonblocking I/O */
    volatile apr_size_t pollfd_cnt;
    apr_pollset_t *pollset;
};

static Jxta_listener *lookup_listener(Jxta_endpoint_service * me, Jxta_endpoint_address * addr);
static void *APR_THREAD_FUNC outgoing_message_thread(apr_thread_t * t, void *arg);
static Jxta_status outgoing_message_process(Jxta_endpoint_service * service, Jxta_message * msg);
static Jxta_status send_message(Jxta_endpoint_service * me, Jxta_message * msg, Jxta_endpoint_address * dest);

/* negative cache table operations */
static Nc_entry *nc_add_peer(Jxta_endpoint_service * me, const char *addr, Jxta_message * msg);
static void nc_remove_peer(Jxta_endpoint_service * me, Nc_entry * ptr);
static void nc_destroy(Jxta_endpoint_service * me);
static Jxta_status nc_peer_queue_msg(Jxta_endpoint_service * me, Nc_entry * ptr, Jxta_message * msg);
static void nc_review_all(Jxta_endpoint_service * me);

/* transport connection ops */
static Jxta_status tc_new(Tc_elt ** me, Jxta_callback_fn fn, apr_socket_t * s, void * arg, apr_pool_t * pool);
static Jxta_status tc_destroy(Tc_elt * me);

static void messenger_add(Jxta_endpoint_service * me, Jxta_endpoint_address *ea, JxtaEndpointMessenger * msgr);
static void messenger_remove(Jxta_endpoint_service * me, Jxta_endpoint_address *ea);
static void messengers_destroy(Jxta_endpoint_service * me);
static Jxta_status try_existing_messenger(Jxta_endpoint_service * me, Jxta_message * msg, Jxta_endpoint_address * dest);
    
/* event operations */
static Jxta_status event_new(Jxta_endpoint_event ** me, Jxta_endpoint_service * ep, Jxta_endpoint_event_type type,
                             size_t extra_size);
static void event_free(Jxta_object * me);
static Jxta_status emit_local_route_change_event(Jxta_endpoint_service * me);

static void event_free(Jxta_object * me)
{
    Jxta_endpoint_event *myself = (Jxta_endpoint_event *) me;
    if (myself->extra) {
        free(myself->extra);
    }
    free(myself);
}

static Jxta_status event_new(Jxta_endpoint_event ** me, Jxta_endpoint_service * ep, Jxta_endpoint_event_type type,
                             size_t extra_size)
{
    Jxta_endpoint_event *event;

    event = calloc(1, sizeof(Jxta_endpoint_event));
    if (!event) {
        *me = NULL;
        return JXTA_NOMEM;
    }
    if (extra_size) {
        event->extra = calloc(1, extra_size);
        if (!event->extra) {
            free(event);
            *me = NULL;
            return JXTA_NOMEM;
        }
    }

    event->ep_svc = ep;
    event->type = type;
    JXTA_OBJECT_INIT(event, event_free, NULL);
    *me = event;
    return JXTA_SUCCESS;
}

static Jxta_status emit_local_route_change_event(Jxta_endpoint_service * me)
{
    Jxta_status rv;
    Jxta_endpoint_event *event;

    rv = event_new(&event, me, JXTA_EP_LOCAL_ROUTE_CHANGE, 0);
    if (!event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to allocate endpoint event.\n");
        return rv;
    }
    jxta_service_event_emit((Jxta_service *) me, event);
    JXTA_OBJECT_RELEASE(event);
    return JXTA_SUCCESS;
}

static Nc_entry *nc_add_peer(Jxta_endpoint_service * me, const char *addr, Jxta_message * msg)
{
    Nc_entry *ptr = NULL;

    assert(NULL != addr);
    assert(NULL != me);

    apr_thread_mutex_lock(me->nc_wlock);
    ptr = calloc(1, sizeof(*ptr));
    if (NULL == ptr) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to allocate negative cache entry, out of memory?\n");
        apr_thread_mutex_unlock(me->nc_wlock);
        return NULL;
    }
    ptr->addr = strdup(addr);
    if (NULL == ptr->addr) {
        free(ptr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to duplicate address, out of memory?\n");
        apr_thread_mutex_unlock(me->nc_wlock);
        return NULL;
    }
    ptr->timeout = jxta_epcfg_get_nc_timeout_init(me->config);
    ptr->expiration = jpr_time_now() + ptr->timeout;
    ptr->rq_capacity = jxta_epcfg_get_ncrq_size(me->config);
    if (0 != ptr->rq_capacity) {
        ptr->rq = calloc(ptr->rq_capacity, sizeof(*ptr->rq));
        if (NULL == ptr->rq) {
            free(ptr->addr);
            free(ptr);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to allocate retransmit queue, out of memory?\n");
            apr_thread_mutex_unlock(me->nc_wlock);
            return NULL;
        }
        ptr->rq[0] = JXTA_OBJECT_SHARE(msg);
        ptr->rq_tail = 1;
    }
    apr_thread_mutex_lock(me->mutex);
    apr_hash_set(me->nc, ptr->addr, APR_HASH_KEY_STRING, ptr);
    apr_thread_mutex_unlock(me->mutex);
    apr_thread_mutex_unlock(me->nc_wlock);
    return ptr;
}

static void nc_remove_peer(Jxta_endpoint_service * me, Nc_entry * ptr)
{
    assert(NULL == ptr->rq || -1 == ptr->rq_tail);

    apr_thread_mutex_lock(me->nc_wlock);
    apr_thread_mutex_lock(me->mutex);
    apr_hash_set(me->nc, ptr->addr, APR_HASH_KEY_STRING, NULL);
    apr_thread_mutex_unlock(me->mutex);
    apr_thread_mutex_unlock(me->nc_wlock);
    free(ptr->addr);
    free(ptr->rq);
    free(ptr);
}

static void nc_peer_drop_retransmit_queue(Nc_entry * me)
{
    if (NULL == me->rq) {
        return;
    }

    if (-1 != me->rq_tail) {
        do {
            Jxta_message *msg = NULL;

            assert(me->rq_capacity > me->rq_head);
            msg = me->rq[me->rq_head++];
            assert(NULL != msg);
            JXTA_OBJECT_RELEASE(msg);
            if (me->rq_head == me->rq_capacity) {
                me->rq_head = 0;
            }
        } while (me->rq_head != me->rq_tail);
    }

    free(me->rq);
    me->rq = NULL;
}

static void nc_destroy(Jxta_endpoint_service * me)
{
    apr_hash_index_t *hi = NULL;
    Nc_entry *ptr = NULL;

    for (hi = apr_hash_first(NULL, me->nc); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi, NULL, NULL, (void **) &ptr);
        assert(NULL != ptr);

        nc_peer_drop_retransmit_queue(ptr);
        free(ptr->addr);
        free(ptr);
    }
}

/**
 * Review the negative cache entry.
 * Try to send messages in the retransmit queue after timeout
 * In case failed, double the timeout period. If the timeout period is already maximum, drop the queue completely
 * @return JXTA_SUCCESS if the peer is out of timeout period and no msg remains in the queue. JXTA_UNREACHABLE_DEST if the peer
 * still is unreachable.
 */
static Jxta_status nc_peer_process_queue(Jxta_endpoint_service * me, Nc_entry * ptr)
{
    Jxta_message *msg = NULL;
    Jxta_status res = JXTA_SUCCESS;
    int sent = 0;

    if (jpr_time_now() < ptr->expiration) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is still in timeout for another %" APR_INT64_T_FMT "ms.\n",
                        ptr->addr, ptr->expiration - jpr_time_now());
        return JXTA_UNREACHABLE_DEST;
    }

    if (!ptr->rq || -1 == ptr->rq_tail) {
        return JXTA_SUCCESS;
    }

    do {
        msg = ptr->rq[ptr->rq_head];
        res = send_message(me, msg, NULL);
        if (JXTA_SUCCESS != res) {
            break;
        }
        JXTA_OBJECT_RELEASE(msg);
        ++sent;
        if (++ptr->rq_head >= ptr->rq_capacity) {
            ptr->rq_head = 0;
        }
    } while (ptr->rq_head != ptr->rq_tail);

    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is now reachable after timeout %" APR_INT64_T_FMT
                        "ms, retransmit queue is flushed.\n", ptr->addr, ptr->timeout);
        ptr->rq_tail = -1;
    } else if (!sent && JXTA_UNREACHABLE_DEST == res) {
        Jxta_time_diff timeout_max;

        timeout_max = jxta_epcfg_get_nc_timeout_max(me->config);
        if (ptr->timeout >= timeout_max) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is still unreachable after timeout %" APR_INT64_T_FMT
                            "ms over maximum %" APR_INT64_T_FMT "ms, drop retransmit queue.\n", ptr->addr, ptr->timeout,
                            timeout_max);
            nc_peer_drop_retransmit_queue(ptr);
            ptr->timeout = timeout_max;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is still unreachable after timeout %" APR_INT64_T_FMT
                            "ms, double it(limit to a maximum of %" APR_INT64_T_FMT "ms).\n", ptr->addr, ptr->timeout,
                            timeout_max);
            ptr->timeout *= 2;
            if (ptr->timeout > timeout_max) {
                ptr->timeout = timeout_max;
            }
        }
        ptr->expiration = jpr_time_now() + ptr->timeout;
    }
    return res;
}

static Jxta_status nc_peer_queue_msg(Jxta_endpoint_service * me, Nc_entry * ptr, Jxta_message * msg)
{
    Jxta_status res;

    apr_thread_mutex_lock(me->nc_wlock);
    res = nc_peer_process_queue(me, ptr);
    if (JXTA_SUCCESS == res) {
        if (JXTA_UNREACHABLE_DEST != send_message(me, msg, NULL)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is back online.\n", ptr->addr);
            nc_remove_peer(me, ptr);
            apr_thread_mutex_unlock(me->nc_wlock);
            return res;
        }
    }

    if (NULL == ptr->rq) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is still unreachable with maximum timeout %" APR_INT64_T_FMT
                        "ms, drop message[%pp]\n", ptr->addr, ptr->timeout, msg);
        apr_thread_mutex_unlock(me->nc_wlock);
        return JXTA_UNREACHABLE_DEST;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Queueing msg[%pp] for peer at %s for later retransmission.\n", msg,
                    ptr->addr);
    if (ptr->rq_head == ptr->rq_tail) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                        "Peer at %s has filled up(%d) retransmit queue, drop oldest message[%pp]\n", ptr->addr, ptr->rq_capacity,
                        ptr->rq[ptr->rq_head]);
        JXTA_OBJECT_RELEASE(ptr->rq[ptr->rq_head]);
        ptr->rq[ptr->rq_head++] = JXTA_OBJECT_SHARE(msg);
        if (ptr->rq_head >= ptr->rq_capacity) {
            ptr->rq_head = 0;
        }
    } else {
        if (-1 == ptr->rq_tail) {
            ptr->rq_tail = ptr->rq_head;
        }
        ptr->rq[ptr->rq_tail++] = JXTA_OBJECT_SHARE(msg);
        if (ptr->rq_tail >= ptr->rq_capacity) {
            ptr->rq_tail = 0;
        }
    }
    apr_thread_mutex_unlock(me->nc_wlock);
    return JXTA_UNREACHABLE_DEST;
}

static void nc_review_all(Jxta_endpoint_service * me)
{
    apr_hash_index_t *hi = NULL;
    Nc_entry *ptr = NULL;
    Jxta_status res = JXTA_SUCCESS;

    apr_thread_mutex_lock(me->nc_wlock);
    for (hi = apr_hash_first(NULL, me->nc); hi;) {
        apr_hash_this(hi, NULL, NULL, (void **) &ptr);
        /* move index before calling nc_remove_peer which removes entry from cache table and could screw the index */
        hi = apr_hash_next(hi);
        assert(NULL != ptr);

        res = nc_peer_process_queue(me, ptr);
        if (JXTA_SUCCESS == res && NULL != ptr->rq) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is back online.\n", ptr->addr);
            nc_remove_peer(me, ptr);
        }
    }
    apr_thread_mutex_unlock(me->nc_wlock);
}

/* transport connection ops */
static Jxta_status tc_new(Tc_elt ** me, Jxta_callback_fn fn, apr_socket_t * s, void * arg, apr_pool_t * pool)
{
    apr_pool_t * p;

    *me = calloc(1, sizeof(**me));
    if (! *me) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        return JXTA_NOMEM;
    }
    if (APR_SUCCESS != apr_pool_create(&p, pool)) {
        free(*me);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        return JXTA_NOMEM;
    }

    (*me)->fn = fn;
    (*me)->arg = arg;
    (*me)->fd.p = p;
    (*me)->fd.desc.s = s;
    (*me)->fd.desc_type = APR_POLL_SOCKET;
    (*me)->fd.client_data = *me;
    (*me)->fd.reqevents = APR_POLLIN | APR_POLLPRI;
    return JXTA_SUCCESS;
}

static Jxta_status tc_destroy(Tc_elt * me)
{
    apr_pool_destroy(me->fd.p);
    free(me);
    return JXTA_SUCCESS;
}

static void messenger_add(Jxta_endpoint_service * me, Jxta_endpoint_address *ea, JxtaEndpointMessenger *msgr)
{
    char * ta;
    Peer_route_elt *ptr;

    ta = jxta_endpoint_address_get_transport_addr(ea);

    ptr = apr_hash_get(me->messengers, ta, APR_HASH_KEY_STRING);
    if (ptr) {
        /* hardcode TCP preference */
        if (0 == strncasecmp("tcp://", ptr->ta, 6) && strcasecmp("tcp", jxta_endpoint_address_get_protocol_name(ea))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                            "Keep current messenger[%pp] which is preferred than new one[%pp] for peer at %s.\n", 
                            ptr->msgr, msgr, ta);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE 
                            "The old messenger[%pp] will be replaced with new one[%pp] for peer at %s.\n", 
                            ptr->msgr, msgr, ta);
            JXTA_OBJECT_RELEASE(ptr->msgr);
            ptr->msgr = JXTA_OBJECT_SHARE(msgr);
        }
        free(ta);
        return;
    }

    ptr = calloc(1, sizeof(*ptr));
    if (!ptr) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
        free(ta);
        return;
    }

    ptr->ta = ta;
    ptr->msgr = JXTA_OBJECT_SHARE(msgr);
    apr_hash_set(me->messengers, ta, APR_HASH_KEY_STRING, ptr);
}

static void messenger_remove(Jxta_endpoint_service * me, Jxta_endpoint_address *ea)
{
    char * ta;
    Peer_route_elt *ptr;

    ta = jxta_endpoint_address_get_transport_addr(ea);
    ptr = apr_hash_get(me->messengers, ta, APR_HASH_KEY_STRING);
    free(ta);
    if (ptr) {
        apr_hash_set(me->messengers, ptr->ta, APR_HASH_KEY_STRING, NULL);
        free(ptr->ta);
        JXTA_OBJECT_RELEASE(ptr->msgr);
        free(ptr);
    }
}

static void messengers_destroy(Jxta_endpoint_service * me)
{
    apr_hash_index_t *hi = NULL;
    Peer_route_elt *ptr;
    const char *ta;

    for (hi = apr_hash_first(NULL, me->messengers); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi, (const void **) &ta, NULL, (void **) &ptr);
        assert(NULL != ptr);
        assert(ta == ptr->ta);

        free(ptr->ta);
        JXTA_OBJECT_RELEASE(ptr->msgr);
        free(ptr);
    }
}

/*
 * Implementations for module methods
 */
static Jxta_status endpoint_init(Jxta_module * it, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_id *gid;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    apr_pool_t *pool;
    Jxta_PG *parentgroup;

    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing ...\n");

    jxta_service_init((Jxta_service *) self, group, assigned_id, impl_adv);
    pool = jxta_PG_pool_get(group);
    self->listener_table = apr_hash_make(pool);
    self->nc = apr_hash_make(pool);
    self->cb_table = apr_hash_make(pool);
    self->messengers = apr_hash_make(pool);
    self->filter_list = dl_make();
    self->transport_table = jxta_hashtable_new(2);
    if (self->transport_table == NULL || self->filter_list == NULL) {
        return JXTA_FAILED;
    }

    apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, pool);
    apr_thread_mutex_create(&self->nc_wlock, APR_THREAD_MUTEX_NESTED, pool);
    apr_thread_mutex_create(&self->demux_mutex, APR_THREAD_MUTEX_NESTED, pool);

    self->msg_task_cnt = 0;

    /* advs and groups are jxta_objects that we share */
    if (impl_adv != NULL) {
        JXTA_OBJECT_SHARE(impl_adv);
    }

    self->my_impl_adv = impl_adv;
    self->my_group = group;
    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &(self->my_groupid));
    JXTA_OBJECT_RELEASE(gid);
    self->relay_addr = NULL;
    self->relay_proto = NULL;
    self->myRoute = NULL;
    jxta_PG_get_configadv(group, &conf_adv);

    if (!conf_adv) {
        jxta_PG_get_parentgroup(group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }

    if (conf_adv != NULL) {
        jxta_PA_get_Svc_with_id(conf_adv, jxta_endpoint_classid, &svc);
        if (NULL != svc) {
            self->config = jxta_svc_get_EndPointConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    if (NULL == self->config) {
        self->config = jxta_EndPointConfigAdvertisement_new();
        if (self->config == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
            return JXTA_NOMEM;
        }
    }

    apr_pollset_create(&self->pollset, 100, pool, 0);
    self->pollfd_cnt = 0;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initialized\n");
    return JXTA_SUCCESS;
}

static void* APR_THREAD_FUNC do_poll(apr_thread_t * thd, void * arg)
{
    Jxta_endpoint_service * me = arg;
    Jxta_status rv;
    Jxta_module_state running;

    /* Use 1 second timeout instead of blocking because:
     * a. poll won't return when close local listening socket as 9/1/2006
     * b. give this thread a chance to serve other tasks in thread pool
     */
    rv = endpoint_service_poll(me, 1000000L);
    running = jxta_module_state((Jxta_module*) me);
    if ((JXTA_MODULE_STARTED == running || JXTA_MODULE_STARTING == running) && me->pollfd_cnt) {
        apr_thread_pool_push(jxta_PG_thread_pool_get(me->my_group), do_poll, me, APR_THREAD_TASK_PRIORITY_HIGHEST, me);
    }
    return JXTA_SUCCESS;
}

static Jxta_status endpoint_start(Jxta_module * me, const char *args[])
{
    Jxta_endpoint_service * myself = (Jxta_endpoint_service *) me;

    /* construct + init have done everything already */
    PTValid(myself, Jxta_endpoint_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Starting ...\n");
    if (myself->pollfd_cnt) {
        apr_thread_pool_push(jxta_PG_thread_pool_get(myself->my_group), do_poll, myself, APR_THREAD_TASK_PRIORITY_HIGHEST, myself);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started\n");
    return JXTA_SUCCESS;
}

static void endpoint_stop(Jxta_module * self)
{
    Jxta_endpoint_service *me = PTValid(self, Jxta_endpoint_service);

    /* stop the thread that processes outgoing messages */
    apr_thread_pool_tasks_cancel(jxta_PG_thread_pool_get(me->my_group), me);
    if (NULL != me->router_transport) {
        JXTA_OBJECT_RELEASE(me->router_transport);
        me->router_transport = NULL;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}

/*
 * implementations for service methods
 */
static void endpoint_get_MIA(Jxta_service * it, Jxta_advertisement ** mia)
{
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    *mia = JXTA_OBJECT_SHARE(self->my_impl_adv);
}

static void endpoint_get_interface(Jxta_service * it, Jxta_service ** serv)
{
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    JXTA_OBJECT_SHARE(self);
    *serv = (Jxta_service *) self;
}

/*
 * Endpoint service is not virtual yet : there is no way to provide
 * more than one implementation. Therefore its methods table is just
 * the same than Jxta_service's.
 */
typedef Jxta_service_methods Jxta_endpoint_service_methods;

static const Jxta_endpoint_service_methods jxta_endpoint_service_methods = {
    {
     "Jxta_module_methods",
     endpoint_init,
     endpoint_start,
     endpoint_stop},
    "Jxta_service_methods",
    endpoint_get_MIA,
    endpoint_get_interface,
    service_on_option_set
};

typedef struct _Filter Filter;

struct _Filter {
    char *str;
    JxtaEndpointFilter func;
    void *arg;
};

void jxta_endpoint_service_destruct(Jxta_endpoint_service * service)
{
    PTValid(service, Jxta_endpoint_service);

    /* delete tables and stuff */
    nc_destroy(service);
    messengers_destroy(service);
    JXTA_OBJECT_RELEASE(service->transport_table);

    if (service->relay_addr)
        free(service->relay_addr);
    if (service->relay_proto)
        free(service->relay_proto);
    dl_free(service->filter_list, free);

    apr_thread_mutex_destroy(service->nc_wlock);
    apr_thread_mutex_destroy(service->demux_mutex);
    apr_thread_mutex_destroy(service->mutex);

    /* un-ref our impl_adv, and delete assigned_id */
    service->my_group = NULL;

    if (service->my_groupid) {
        JXTA_OBJECT_RELEASE(service->my_groupid);
        service->my_groupid = NULL;
    }
    if (service->my_impl_adv) {
        JXTA_OBJECT_RELEASE(service->my_impl_adv);
        service->my_impl_adv = NULL;
    }

    if (service->myRoute != NULL) {
        JXTA_OBJECT_RELEASE(service->myRoute);
        service->myRoute = NULL;
    }

    if (service->config != NULL) {
        JXTA_OBJECT_RELEASE(service->config);
        service->config = NULL;
    }

    service->thisType = NULL;

    jxta_service_destruct((Jxta_service *) service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
}

Jxta_endpoint_service *jxta_endpoint_service_construct(Jxta_endpoint_service * service,
                                                       const Jxta_endpoint_service_methods * methods)
{
    PTValid(methods, Jxta_service_methods);

    jxta_service_construct((Jxta_service *) service, (const Jxta_service_methods *) methods);
    service->thisType = "Jxta_endpoint_service";

    service->my_impl_adv = NULL;
    service->my_group = NULL;
    service->my_groupid = NULL;
    service->router_transport = NULL;

    return service;
}

static void jxta_endpoint_service_free(Jxta_object * obj)
{
    jxta_endpoint_service_destruct((Jxta_endpoint_service *) obj);

    memset((void *) obj, 0xdd, sizeof(Jxta_endpoint_service));
    free((void *) obj);
}

Jxta_endpoint_service *jxta_endpoint_service_new_instance(void)
{
    Jxta_endpoint_service *service = (Jxta_endpoint_service *) calloc(1, sizeof(Jxta_endpoint_service));
    if (service == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }
    JXTA_OBJECT_INIT(service, jxta_endpoint_service_free, 0);

    if (NULL == jxta_endpoint_service_construct(service, &jxta_endpoint_service_methods)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        free(service);
        service = NULL;
    }

    return service;
}

Jxta_status endpoint_service_demux(Jxta_endpoint_service * me, const char *name, const char *param, Jxta_message * msg)
{
    char *key;
    Jxta_status rv;
    Cb_elt *cb;

    key = get_service_key(name, param);
    apr_thread_mutex_lock(me->demux_mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Looking up callback for %s\n", key);
    cb = apr_hash_get(me->cb_table, key, APR_HASH_KEY_STRING);
    if (!cb && param) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Looking up callback for fallback %s\n", name);
        cb = apr_hash_get(me->cb_table, name, APR_HASH_KEY_STRING);
    }
    apr_thread_mutex_unlock(me->demux_mutex);
    if (cb) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Calling found callback.\n");
        rv = cb->func((Jxta_object *) msg, cb->arg);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Demux: No callback found for %s.\n", key);
        rv = JXTA_ITEM_NOTFOUND;
    }
    free(key);
    return rv;
}

JXTA_DECLARE(void) jxta_endpoint_service_demux(Jxta_endpoint_service * service, Jxta_message * msg)
{
    Jxta_endpoint_address *dest;

    PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);

    dest = jxta_message_get_destination(msg);
    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to get message destination\n");
        return;
    }

    jxta_endpoint_service_demux_addr(service, dest, msg);

    JXTA_OBJECT_RELEASE(dest);
}

JXTA_DECLARE(void) jxta_endpoint_service_demux_addr(Jxta_endpoint_service * service, Jxta_endpoint_address * dest,
                                                    Jxta_message * msg)
{
    Dlist *cur;
    Filter *cur_filter;
    Jxta_vector *vector = NULL;
    char *destStr;
    Jxta_listener *listener;

    PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);

    if (JXTA_MODULE_STARTED != jxta_module_state((Jxta_module*)service)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: Endpoint stopped \n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Demux of message [%pp]\n", msg);

    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: Destination of message [%pp] is NULL\n", msg);
        return;
    }

    JXTA_OBJECT_CHECK_VALID(dest);

  /**
   * FIXME 4/11/2002 lomax@jxta.org
   *
   * The following code needs to be revisited:
   *    - a same message can go several times through demux. Filters must
   *      be invoked only once.
   **/

    /* pass message through appropriate filters */
    apr_thread_mutex_lock(service->demux_mutex);

    dl_traverse(cur, service->filter_list) {
        Jxta_message_element *el = NULL;
        cur_filter = cur->val;

        if ((vector = jxta_message_get_elements_of_namespace(msg, cur_filter->str)) != NULL
            || (JXTA_SUCCESS == jxta_message_get_element_1(msg, cur_filter->str, &el))) {

            JXTA_OBJECT_RELEASE(el);
            /* discard the message if the filter returned false */
            if (!cur_filter->func(msg, cur_filter->arg)) {
                apr_thread_mutex_unlock(service->demux_mutex);
                return;
            }

        }
        if (vector != NULL) {
            JXTA_OBJECT_RELEASE(vector);
        }
    }

    apr_thread_mutex_unlock(service->demux_mutex);

    destStr = jxta_endpoint_address_to_string(dest);

    if (endpoint_service_demux(service, jxta_endpoint_address_get_service_name(dest),
                               jxta_endpoint_address_get_service_params(dest), msg) != JXTA_ITEM_NOTFOUND) {
        goto FINAL_EXIT;
    }

    /* Todo: remove listener interface once transition to callback completed */
    listener = lookup_listener(service, dest);

    if (listener != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Calling listener for %s\n", destStr);
        jxta_listener_process_object(listener, (Jxta_object *) msg);

        JXTA_OBJECT_CHECK_VALID(msg);
        goto FINAL_EXIT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: No demux listener for %s\n", destStr);
    /* end of listener section to be deprecated */
  FINAL_EXIT:
    free(destStr);
}

Jxta_status endpoint_service_poll(Jxta_endpoint_service * me, apr_interval_time_t timeout)
{
    apr_status_t rv;
    const apr_pollfd_t * fds;
    int i;
    apr_int32_t num_sockets;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "before the pollset poll\n");
    assert(me->pollfd_cnt > 0);
    rv = apr_pollset_poll(me->pollset, timeout, &num_sockets, &fds);

/* Not sure if we can safely ignore when endpoint is stopping. Transport should still get a chance to handle it.
    if (! me->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint service is stopped, skip poll.\n");
        return JXTA_SUCCESS;
    }
*/

    if (APR_SUCCESS != rv) {
        jxta_log_append(__log_cat, APR_STATUS_IS_TIMEUP(rv) ? JXTA_LOG_LEVEL_PARANOID : JXTA_LOG_LEVEL_DEBUG, 
                        "polling func return with status %d\n", rv);
        return rv;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "num sockets = %d\n", num_sockets);
    for (i = 0; i < num_sockets; i++) {
        int ev;
        Tc_elt * tc;

        ev = fds->rtnevents;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, 
                        "APR_POLLIN %d APR_POLLOUT %d APR_POLLPRI %d APR_POLLERR %d APR_POLLHUP %d APR_POLLNVAL %d\n", 
                        ev & APR_POLLIN ? 1:0, ev & APR_POLLOUT ? 1:0, ev & APR_POLLPRI ? 1:0, ev & APR_POLLERR  ? 1:0,
                        ev & APR_POLLHUP ? 1:0, ev & APR_POLLNVAL ? 1:0);
        tc = fds->client_data;
        rv = tc->fn(&tc->fd, tc->arg);
        fds++;
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_endpoint_service_get_route_from_PA(Jxta_PA * padv, Jxta_RouteAdvertisement ** route)
{
    Jxta_svc *svc = NULL;

    jxta_PA_get_Svc_with_id(padv, jxta_endpoint_classid_get(), &svc);

    if (svc != NULL) {
        *route = jxta_svc_get_RouteAdvertisement(svc);
        JXTA_OBJECT_RELEASE(svc);
    }
}

/**
 * Add a transport to the endpoint service, if the transport supports inbound messages, update the router advertisement to the
 * local peer.
 */
JXTA_DECLARE(void) jxta_endpoint_service_add_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
{
    Jxta_PA *pa;
    Jxta_vector *svcs;
    Jxta_RouteAdvertisement *route;
    Jxta_AccessPointAdvertisement *dest;
    Jxta_endpoint_address *address;
    Jxta_vector *addresses;
    Jxta_id *pid = NULL;
    JString *jaddress;
    char *caddress;
    Jxta_id *assigned_id;

    Jxta_svc *svc = NULL;
    JString *name;

    PTValid(service, Jxta_endpoint_service);
    if (jxta_transport_allow_inbound(transport)) {
        /* Add the public address to the endpoint section of the PA */
        jxta_PG_get_PA(service->my_group, &pa);
        assigned_id = jxta_service_get_assigned_ID_priv((Jxta_service *) service);
        jxta_PA_get_Svc_with_id(pa, assigned_id, &svc);
        if (svc == NULL) {
            svc = jxta_svc_new();
            if (svc == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_svc_set_MCID(svc, assigned_id);
            svcs = jxta_PA_get_Svc(pa);
            jxta_vector_add_object_last(svcs, (Jxta_object *) svc);
            JXTA_OBJECT_RELEASE(svcs);
        }

        route = jxta_svc_get_RouteAdvertisement(svc);
        if (route == NULL) {
            route = jxta_RouteAdvertisement_new();
            if (route == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_svc_set_RouteAdvertisement(svc, route);
        }
        address = jxta_transport_publicaddr_get(transport);
        caddress = jxta_endpoint_address_to_string(address);
        jaddress = jstring_new_2(caddress);
        if (jaddress == NULL || caddress == NULL || address == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
            JXTA_OBJECT_RELEASE(address);
            free(caddress);
            JXTA_OBJECT_RELEASE(pa);
            JXTA_OBJECT_RELEASE(svc);
            return;
        }
        dest = jxta_RouteAdvertisement_get_Dest(route);
        if (dest == NULL) {
            dest = jxta_AccessPointAdvertisement_new();
            if (dest == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(address);
                free(caddress);
                JXTA_OBJECT_RELEASE(jaddress);
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_PG_get_PID(service->my_group, &pid);
            jxta_AccessPointAdvertisement_set_PID(dest, pid);
            jxta_RouteAdvertisement_set_Dest(route, dest);
            JXTA_OBJECT_RELEASE(pid);
        }

        addresses = jxta_AccessPointAdvertisement_get_EndpointAddresses(dest);
        if (addresses == NULL) {
            addresses = jxta_vector_new(2);
            if (addresses == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(addresses);
                free(caddress);
                JXTA_OBJECT_RELEASE(jaddress);
                JXTA_OBJECT_RELEASE(dest);
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_AccessPointAdvertisement_set_EndpointAddresses(dest, addresses);
        }

        jxta_vector_add_object_last(addresses, (Jxta_object *) jaddress);

        if (service->myRoute != NULL)
            JXTA_OBJECT_RELEASE(service->myRoute);

        service->myRoute = route;
        free(caddress);

        JXTA_OBJECT_RELEASE(addresses);
        JXTA_OBJECT_RELEASE(dest);
        JXTA_OBJECT_RELEASE(jaddress);
        JXTA_OBJECT_RELEASE(address);
        JXTA_OBJECT_RELEASE(svc);
        JXTA_OBJECT_RELEASE(pa);
    }

    /* add the transport to our table */
    /* FIXME: should check allow_overload etc. */
    name = jxta_transport_name_get(transport);
    jxta_hashtable_put(service->transport_table, jstring_get_string(name),  /* string gets copied */
                       jstring_length(name), (Jxta_object *) transport);

    /* String copied. Not needed anymore. */
    JXTA_OBJECT_RELEASE(name);
    if (jxta_transport_allow_inbound(transport)) {
        emit_local_route_change_event(service);
    }
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_add_poll(Jxta_endpoint_service * me, apr_socket_t * s, Jxta_callback_fn fn,
                                                         void * arg, void ** cookie)
{
    Jxta_status rv;
    Tc_elt * tc;
    Jxta_module_state running;
    
    rv = tc_new(&tc, fn, s, arg, jxta_PG_pool_get(me->my_group));
    if (rv != JXTA_SUCCESS) {
        *cookie = NULL;
        return rv;
    }

    apr_thread_mutex_lock(me->mutex);
    rv = apr_pollset_add(me->pollset, &tc->fd);
    running = jxta_module_state((Jxta_module*) me);
    if (0 == me->pollfd_cnt++ && (JXTA_MODULE_STARTED == running || JXTA_MODULE_STARTING == running)) {
        apr_thread_pool_push(jxta_PG_thread_pool_get(me->my_group), do_poll, me, APR_THREAD_TASK_PRIORITY_HIGHEST, me);
    }
    apr_thread_mutex_unlock(me->mutex);
    if (rv != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to add poll[%pp] with error %d\n", tc, rv);
        *cookie = NULL;
        tc_destroy(tc);
        return rv;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Add poll[%pp] with socket[%pp], total %d sockets to poll\n", tc, s,
                    me->pollfd_cnt);

    *cookie = tc;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_remove_poll(Jxta_endpoint_service * me, void * cookie)
{
    Jxta_status rv;
    Tc_elt *tc = cookie;

    apr_thread_mutex_lock(me->mutex);
    assert(me->pollfd_cnt > 0);
    rv = apr_pollset_remove (me->pollset, &tc->fd);
    if (APR_SUCCESS == rv) {
        --me->pollfd_cnt;
    }
    apr_thread_mutex_unlock(me->mutex);
    if (APR_SUCCESS != rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to remove poll[%pp] with error %d\n", tc, rv);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Removed poll[%pp] with socket[%pp], remaining %d sockets to poll\n", 
                        tc, tc->fd.desc.s, me->pollfd_cnt);
    }
    tc_destroy(tc);
    return (APR_SUCCESS == rv) ? JXTA_SUCCESS : JXTA_FAILED;
}

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_endpoint_service_get_local_route(Jxta_endpoint_service * service)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->myRoute != NULL)
        JXTA_OBJECT_SHARE(service->myRoute);
    return service->myRoute;
}

JXTA_DECLARE(void) jxta_endpoint_service_set_local_route(Jxta_endpoint_service * service, Jxta_RouteAdvertisement * route)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->myRoute != NULL)
        JXTA_OBJECT_RELEASE(service->myRoute);

    service->myRoute = route;
    JXTA_OBJECT_SHARE(service->myRoute);
    emit_local_route_change_event(service);
}

JXTA_DECLARE(void) jxta_endpoint_service_remove_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
{
    JString *name = jxta_transport_name_get(transport);
    Jxta_object *rmd = NULL;

    PTValid(service, Jxta_endpoint_service);

    jxta_hashtable_del(service->transport_table, jstring_get_string(name), jstring_length(name), &rmd);

    /* String copied. Not needed anymore. */
    JXTA_OBJECT_RELEASE(name);

    /*
     * FIXME 20040516 tra
     * We should update our route info here removing
     * the corresponding endpoint address
     */

    /* If trsp existed, release it */
    if (rmd != NULL)
        JXTA_OBJECT_RELEASE(rmd);
}

/* FIXME: transport_name should be a JString for consistency */
JXTA_DECLARE(Jxta_transport *) jxta_endpoint_service_lookup_transport(Jxta_endpoint_service * service, const char *transport_name)
{
    Jxta_transport *t = NULL;

    PTValid(service, Jxta_endpoint_service);

    jxta_hashtable_get(service->transport_table, transport_name, strlen(transport_name), JXTA_OBJECT_PPTR(&t));

    /*
     * This routine is allowed to return NULL if the requested transport
     * was not found. (Until we convert this code the convention of always
     * returning a status.
     */
    if (t != NULL) {
        JXTA_OBJECT_CHECK_VALID(t);
    }

    return t;
}

JXTA_DECLARE(void) jxta_endpoint_service_add_filter(Jxta_endpoint_service * service, char const *str, JxtaEndpointFilter f,
                                                    void *arg)
{
    Filter *filter = (Filter *) malloc(sizeof(Filter));
    if (filter == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return;
    }
    PTValid(service, Jxta_endpoint_service);

    filter->func = f;
    filter->arg = arg;

    /* insert the filter into the list */
    apr_thread_mutex_lock(service->demux_mutex);
    dl_insert_b(service->filter_list, filter);
    apr_thread_mutex_unlock(service->demux_mutex);
}

JXTA_DECLARE(void) jxta_endpoint_service_remove_filter(Jxta_endpoint_service * service, JxtaEndpointFilter filter)
{
    Dlist *cur;

    PTValid(service, Jxta_endpoint_service);

    apr_thread_mutex_lock(service->demux_mutex);

    /* find the filter registered under this handle */

    dl_traverse(cur, service->filter_list) {

        if (((Filter *) cur->val)->func == filter)
            break;

    }

    /* remove this filter from the list */
    if (cur != NULL) {
        free(cur->val);
        dl_delete_node(cur);
    }

    apr_thread_mutex_unlock(service->demux_mutex);
}

static apr_status_t cb_elt_cleanup(void *me)
{
    Cb_elt *_self = me;

    if (_self->recipient) {
        free(_self->recipient);
    }

    return APR_SUCCESS;
}

static Cb_elt *cb_elt_create(Jxta_endpoint_service * me, const char *name, const char *param, Jxta_callback_func f, void *arg)
{
    Cb_elt *cb;
    apr_pool_t *pool;

    pool = jxta_PG_pool_get(me->my_group);
    apr_thread_mutex_lock(me->mutex);
    if (NULL == me->recycled_cbs) {
        cb = apr_pcalloc(pool, sizeof(*cb));
        cb->ep_svc = me;
        apr_pool_cleanup_register(pool, cb, cb_elt_cleanup, apr_pool_cleanup_null);
    } else {
        cb = me->recycled_cbs;
        if (cb->recipient) {
            free(cb->recipient);
        }
        me->recycled_cbs = cb->recycle;
    }
    apr_thread_mutex_unlock(me->mutex);
    cb->func = f;
    cb->arg = arg;
    cb->recipient = get_service_key(name, param);

    return cb;
}

static void cb_elt_recycle(Cb_elt * me)
{
    Jxta_endpoint_service *ep = me->ep_svc;

    apr_thread_mutex_lock(ep->mutex);
    me->recycle = ep->recycled_cbs;
    ep->recycled_cbs = me;
    apr_thread_mutex_unlock(ep->mutex);
}

Jxta_status endpoint_service_add_recipient(Jxta_endpoint_service * me, void **cookie, const char *name, const char *param,
                                           Jxta_callback_func f, void *arg)
{
    Cb_elt *cb, *old_cb;
    Jxta_status rv;

    cb = cb_elt_create(me, name, param, f, arg);
    apr_thread_mutex_lock(me->demux_mutex);
    old_cb = apr_hash_get(me->cb_table, cb->recipient, APR_HASH_KEY_STRING);
    if (old_cb) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "There is a recipient for %s already, ignor request.\n",
                        cb->recipient);
        cb_elt_recycle(cb);
        *cookie = NULL;
        rv = JXTA_BUSY;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding recipient[%pp] for %s.\n", cb->func, cb->recipient);
        apr_hash_set(me->cb_table, cb->recipient, APR_HASH_KEY_STRING, cb);
        *cookie = cb->recipient;
        rv = JXTA_SUCCESS;
    }
    apr_thread_mutex_unlock(me->demux_mutex);
    return rv;
}

Jxta_status endpoint_service_remove_recipient(Jxta_endpoint_service * me, void *cookie)
{
    Jxta_status rv;
    Cb_elt *old_cb;

    apr_thread_mutex_lock(me->demux_mutex);
    old_cb = apr_hash_get(me->cb_table, cookie, APR_HASH_KEY_STRING);
    if (!old_cb) {
        rv = JXTA_ITEM_NOTFOUND;
    } else {
        apr_hash_set(me->cb_table, cookie, APR_HASH_KEY_STRING, NULL);
        cb_elt_recycle(old_cb);
        rv = JXTA_SUCCESS;
    }
    apr_thread_mutex_unlock(me->demux_mutex);
    return rv;
}

Jxta_status endpoint_service_remove_recipient_by_addr(Jxta_endpoint_service * me, const char *name, const char *param)
{
    char *key;
    Jxta_status rv;

    key = get_service_key(name, param);
    rv = endpoint_service_remove_recipient(me, key);
    free(key);
    return rv;
}

JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_add_listener(Jxta_endpoint_service * service,
                                   char const *serviceName, char const *serviceParam, Jxta_listener * listener)
{
    int str_length = (serviceName != NULL ? strlen(serviceName) : 0) + (serviceParam != NULL ? strlen(serviceParam) : 0) + 1;
    apr_pool_t *pool = jxta_PG_pool_get(service->my_group);
    char *str = apr_pcalloc(pool, str_length);

    PTValid(service, Jxta_endpoint_service);

    JXTA_DEPRECATED_API();
    apr_snprintf(str, str_length, "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Register listener for %s\n", str);

    apr_thread_mutex_lock(service->demux_mutex);

    if (apr_hash_get(service->listener_table, str, APR_HASH_KEY_STRING)) {
        apr_thread_mutex_unlock(service->demux_mutex);
        return JXTA_BUSY;
    }

    JXTA_OBJECT_SHARE(listener);
    apr_hash_set(service->listener_table, str, APR_HASH_KEY_STRING, (void *) listener);

    apr_thread_mutex_unlock(service->demux_mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_remove_listener(Jxta_endpoint_service * service, char const *serviceName, char const *serviceParam)
{
    Jxta_listener *listener;
    char str[256];

    PTValid(service, Jxta_endpoint_service);

    JXTA_DEPRECATED_API();
    apr_snprintf(str, sizeof(str), "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    apr_thread_mutex_lock(service->demux_mutex);
    listener = (Jxta_listener *) apr_hash_get(service->listener_table, str, APR_HASH_KEY_STRING);
    if (listener != NULL) {
        apr_hash_set(service->listener_table, str, APR_HASH_KEY_STRING, NULL);
        JXTA_OBJECT_RELEASE(listener);
    }
    apr_thread_mutex_unlock(service->demux_mutex);
    return JXTA_SUCCESS;
}

static Jxta_listener *lookup_listener(Jxta_endpoint_service * service, Jxta_endpoint_address * addr)
{
    Jxta_listener *listener = NULL;
    unsigned int i;
    int index;
    int pt = 0;
    int j;
    int str_length;
    char *str;
    char *str1;
    const char *ea_svc_name;
    const char *ea_svc_params;

    PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(addr);

    ea_svc_name = jxta_endpoint_address_get_service_name(addr);
    ea_svc_params = jxta_endpoint_address_get_service_params(addr);
    if (ea_svc_name == NULL) {
        str = jxta_endpoint_address_to_string(addr);
        if (str == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
            return NULL;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No listener associated with this address %s\n", str);

        if (str != NULL)
            free(str);
        return NULL;
    }

    str_length = strlen(ea_svc_name) + (ea_svc_params != NULL ? strlen(ea_svc_params) : 0) + 1;
    str = calloc(str_length, sizeof(char));
    if (str == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return NULL;
    }
  
    strcpy( str, ea_svc_name );

    if( NULL != ea_svc_params ) {
        strcat( str, ea_svc_params );
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Lookup for listener : %s\n", str );

    apr_thread_mutex_lock(service->demux_mutex);
    listener = (Jxta_listener *) apr_hash_get(service->listener_table, str, APR_HASH_KEY_STRING);

    /*
     * Note: 20040510 tra
     * If we did not find a listener, we should check if the message
     * if from a cross-group endpoint messenger. Cross-group messengers
     * are registered only with the service param of the address.
     * The service name is just the name of the Endpoint cross-messenger
     * service:
     *    EndpointService:uuid-groupId/"real address"
     * We need to remove the beginning of the address
     */
    if (listener == NULL) {

        str1 = calloc(strlen(ea_svc_params) + 1, sizeof(char));
        if (str1 == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
            free(str);
            return NULL;
        }
        index = strlen(ea_svc_name);

        /* 
         * copy the address from the param position, skipping the service name
         */
        j = 0;
        pt = 0;
        for (i = 0; i < strlen(ea_svc_params); i++) {
            *(str1 + j) = *(str + index);

            /*
             * remove the extra '/'
             */
            if (*(str1 + j) == '/') {
                index++;
                pt = i; /* save location of end of first parameter */
            } else {
                index++;
                j++;
            }
        }
        *(str1 + j) = '\0';

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "looking for subgroup endpoint %s\n", str1);
        listener = (Jxta_listener *) apr_hash_get(service->listener_table, str1, APR_HASH_KEY_STRING);

        /*
         * FIXME: 20040706 tra
         * The relay transport is using an endpoint address that
         * contain the relay server peerId. For the time being
         * just use the service id for demuxing to the relay as the
         * relay just registered with this address.
         * we are dropping the service parameter. This used to be used
         * when multiple relay connections were supported. This should
         * have been removed.
         */
        if (listener == NULL && pt != 0) {
            strncpy(str1, ea_svc_params, pt - 1);
            *(str1 + pt) = '\0';
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "looking for relay endpoint %s\n", str1);
            listener = (Jxta_listener *) apr_hash_get(service->listener_table, str1, APR_HASH_KEY_STRING);
        }

        free(str1);
    }

    apr_thread_mutex_unlock(service->demux_mutex);
    free(str);
    return listener;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_propagate(Jxta_endpoint_service * service,
                                                          Jxta_message * msg, const char *service_name, const char *service_param)
{
    Jxta_vector *tps = NULL;
    Jxta_transport *transport = NULL;
    JString *svc = NULL;
    JString *param = NULL;
    unsigned int idx;
    Jxta_status rv = JXTA_SUCCESS;

    PTValid(service, Jxta_endpoint_service);

    if (service_name == NULL && service_param == NULL)
        return JXTA_INVALID_ARGUMENT;

    tps = jxta_hashtable_values_get(service->transport_table);
    if (!tps) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to enumerate available transports!\n");
        return JXTA_FAILED;
    }

    /*
     * Correctly set the endpoint address for cross-group
     * demuxing via the NetPeerGroup endpoint service
     */
    svc = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    if (svc == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return JXTA_NOMEM;
    }
    jstring_append_2(svc, ":");
    jstring_append_1(svc, service->my_groupid);

    param = jstring_new_2(service_name);
    jstring_append_2(param, "/");
    jstring_append_2(param, service_param);

    for (idx = 0; idx < jxta_vector_size(tps); ++idx) {
        rv = jxta_vector_get_object_at(tps, JXTA_OBJECT_PPTR(&transport), idx);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to get transport %d of %d!\n", idx, jxta_vector_size(tps));
            break;
        }

        assert(PTValid(transport, Jxta_transport));
        jxta_transport_propagate(transport, msg, jstring_get_string(svc), jstring_get_string(param));
        JXTA_OBJECT_RELEASE(transport);
    }

    JXTA_OBJECT_RELEASE(tps);
    JXTA_OBJECT_RELEASE(param);
    JXTA_OBJECT_RELEASE(svc);
    return JXTA_SUCCESS;
}

static apr_status_t msg_task_cleanup(void *me)
{
    Msg_task *_self = me;

    if (_self->msg) {
        JXTA_OBJECT_RELEASE(_self->msg);
    }

    return APR_SUCCESS;
}

static Msg_task *msg_task_create(Jxta_endpoint_service * me, Jxta_message * msg)
{
    apr_pool_t *pool;
    Msg_task *task;

    pool = jxta_PG_pool_get(me->my_group);
    apr_thread_mutex_lock(me->mutex);
    if (NULL == me->recycled_tasks) {
        task = apr_pcalloc(pool, sizeof(*task));
        task->ep_svc = me;
        apr_pool_cleanup_register(pool, task, msg_task_cleanup, apr_pool_cleanup_null);
    } else {
        task = me->recycled_tasks;
        me->recycled_tasks = task->recycle;
    }
    apr_thread_mutex_unlock(me->mutex);
    /* FIXME: Currently we clone to ensure msg won't be altered */
    /* this is for backward compatibility, we should deprecate this approach to avoid copy */
    task->msg = jxta_message_clone(msg);
    return task;
}

static void msg_task_recycle(Msg_task * me)
{
    Jxta_endpoint_service *ep = me->ep_svc;

    if (me->msg) {
        JXTA_OBJECT_RELEASE(me->msg);
        me->msg = NULL;
    }
    apr_thread_mutex_lock(ep->mutex);
    me->recycle = ep->recycled_tasks;
    ep->recycled_tasks = me;
    apr_thread_mutex_unlock(ep->mutex);
}

static long task_priority(Jxta_message * msg)
{
    apr_uint16_t pri;
    Jxta_status rv;

    rv = jxta_message_get_priority(msg, &pri);
    if (JXTA_SUCCESS != rv) {
        return APR_THREAD_TASK_PRIORITY_NORMAL;
    }
    return (pri >> 8) & APR_THREAD_TASK_PRIORITY_HIGHEST;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_ex(Jxta_endpoint_service * me, Jxta_message * msg,
                                                        Jxta_endpoint_address * dest_addr, Jxta_boolean sync)
{
    Jxta_status res = JXTA_SUCCESS;

    PTValid(me, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);

    if (NULL == jxta_endpoint_address_get_service_name(dest_addr)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        FILEANDLINE "No destination service specified. Discarding message [%pp]\n", msg);
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_message_set_destination(msg, dest_addr);

    if (sync) {
        res = outgoing_message_process(me, msg);
    } else {
        char *baseAddrStr = NULL;
        Nc_entry *ptr = NULL;
        Msg_task *task;

        task = msg_task_create(me, msg);
        apr_thread_pool_push(jxta_PG_thread_pool_get(me->my_group), outgoing_message_thread, task, task_priority(msg), me);
        apr_atomic_inc32(&me->msg_task_cnt);

        baseAddrStr = jxta_endpoint_address_get_transport_addr(dest_addr);
        apr_thread_mutex_lock(me->mutex);
        ptr = apr_hash_get(me->nc, baseAddrStr, APR_HASH_KEY_STRING);
        apr_thread_mutex_unlock(me->mutex);
        free(baseAddrStr);
        res = (NULL == ptr) ? JXTA_SUCCESS : JXTA_UNREACHABLE_DEST;
    }

    return res;
}

/* Deprecated, use jxta_PG_get_recipient_addr to get appropriate rewrited address. */
static Jxta_status do_crossgroup_send(Jxta_PG * obj, Jxta_endpoint_service * me, Jxta_message * msg,
                                      Jxta_endpoint_address * dest_addr, Jxta_boolean sync)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_stdpg *pg = PTValid(obj, Jxta_stdpg);
    Jxta_endpoint_address *new_addr = NULL;

    /*
     * Rewrite actually required for NetGroup as well, because JSE has the concept of WPG
     */
    if (strncmp(jxta_endpoint_address_get_service_name(dest_addr), JXTA_ENDPOINT_SERVICE_NAME, 
                strlen(JXTA_ENDPOINT_SERVICE_NAME))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PeerGroup cross-messenger need to rewrite dest address\n");
        res = jxta_PG_get_recipient_addr((Jxta_PG *)pg, jxta_endpoint_address_get_protocol_name(dest_addr),
                                         jxta_endpoint_address_get_protocol_address(dest_addr),
                                         jxta_endpoint_address_get_service_name(dest_addr),
                                         jxta_endpoint_address_get_service_params(dest_addr), &new_addr);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Rewrite message address failed\n");
            return JXTA_FAILED;
        }
        res = jxta_endpoint_service_send_ex(me, msg, new_addr, sync);
        JXTA_OBJECT_RELEASE(new_addr);
    } else {
        res = jxta_endpoint_service_send_ex(me, msg, dest_addr, sync);
    }
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_sync(Jxta_PG * obj, Jxta_endpoint_service * service, Jxta_message * msg,
                                                          Jxta_endpoint_address * dest_addr)
{
    return do_crossgroup_send(obj, service, msg, dest_addr, JXTA_TRUE);
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_async(Jxta_PG * obj, Jxta_endpoint_service * service, Jxta_message * msg,
                                                           Jxta_endpoint_address * dest_addr)
{
    return do_crossgroup_send(obj, service, msg, dest_addr, JXTA_FALSE);
}

JXTA_DECLARE(void) jxta_endpoint_service_set_relay(Jxta_endpoint_service * service, const char *proto, const char *host)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->relay_addr != NULL) {
        free(service->relay_addr);
    }

    if (NULL != service->relay_proto) {
        free(service->relay_proto);
    }

    if (host != NULL) {
        service->relay_addr = strdup(host);
        service->relay_proto = strdup(proto);
    } else {
        service->relay_addr = NULL;
        service->relay_proto = NULL;
    }
}

JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_addr(Jxta_endpoint_service * service)
{
    char *addr = NULL;

    PTValid(service, Jxta_endpoint_service);

    if (service->relay_addr != NULL) {
        addr = strdup(service->relay_addr);
    }

    return addr;
}

JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_proto(Jxta_endpoint_service * service)
{
    char *proto = NULL;

    PTValid(service, Jxta_endpoint_service);

    if (service->relay_proto != NULL) {
        proto = strdup(service->relay_proto);
        if (proto == NULL)
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");

    }

    return proto;
}

static void *APR_THREAD_FUNC outgoing_message_thread(apr_thread_t * thread, void *arg)
{
    Msg_task *task = arg;
    Jxta_endpoint_service *me = PTValid(task->ep_svc, Jxta_endpoint_service);
    Jxta_message *msg;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint outgoing message handler awake.\n");

    if (JXTA_MODULE_STARTED != jxta_module_state((Jxta_module*)me)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint service is stopped, interrupt handler.\n");
        return NULL;
    }

    apr_atomic_dec32(&me->msg_task_cnt);
    msg = task->msg;
    JXTA_OBJECT_CHECK_VALID(msg);
    outgoing_message_process(me, msg);
    msg_task_recycle(task);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint outgoing message handler stopped.\n");
    return NULL;
}

static Jxta_status try_existing_messenger(Jxta_endpoint_service * me, Jxta_message * msg, Jxta_endpoint_address * dest)
{
    Peer_route_elt *ptr;
    char * ta;
    JxtaEndpointMessenger *messenger;
    Jxta_status res;

    ta = jxta_endpoint_address_get_transport_addr(dest);
    apr_thread_mutex_lock(me->mutex);
    ptr = apr_hash_get(me->messengers, ta, APR_HASH_KEY_STRING);
    free(ta);
    if (!ptr) {
        apr_thread_mutex_unlock(me->mutex);
        return JXTA_ITEM_NOTFOUND;
    }

    messenger = JXTA_OBJECT_SHARE(ptr->msgr);
    apr_thread_mutex_unlock(me->mutex);

    JXTA_OBJECT_CHECK_VALID(messenger);
    res = messenger->jxta_send(messenger, msg);
    if (JXTA_SUCCESS != res) {
        messenger_remove(me, dest);
    }
    JXTA_OBJECT_RELEASE(messenger);
    return res;
}

static Jxta_status send_message(Jxta_endpoint_service * me, Jxta_message * msg, Jxta_endpoint_address * dest)
{
    JxtaEndpointMessenger *messenger;
    Jxta_status res;

    if (NULL == dest) {
        dest = jxta_message_get_destination(msg);
        if (dest == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "Failed to get destination. Message [%pp] is discarded.\n", msg);
            return JXTA_FAILED;
        }
        assert(NULL != jxta_endpoint_address_get_service_name(dest));
    }

    if (JXTA_SUCCESS == try_existing_messenger(me, msg, dest)) {
        return JXTA_SUCCESS;
    }

    /*
     * We have the router transport go ahead and demux to it for sending our message
     */
    messenger = jxta_transport_messenger_get(me->router_transport, dest);
    if (messenger != NULL) {
        JXTA_OBJECT_CHECK_VALID(messenger);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending message [%pp] via router transport.\n", msg);
        res = messenger->jxta_send(messenger, msg);
        JXTA_OBJECT_RELEASE(messenger);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not get router messenger\n");
        res = JXTA_UNREACHABLE_DEST;
    }
    return res;
}

static Jxta_status outgoing_message_process(Jxta_endpoint_service * me, Jxta_message * msg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_endpoint_address *dest;
    char *baseAddrStr = NULL;
    Nc_entry *ptr;
    static volatile apr_uint32_t cnt = 0;
    apr_uint16_t ttl;
    time_t lifespan;

    if (me->router_transport == NULL) {
        me->router_transport = jxta_endpoint_service_lookup_transport(me, "jxta");
        if (me->router_transport == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "Router transport not available. Discarding message [%pp]\n", msg);
            return JXTA_BUSY;
        }
    }

    JXTA_OBJECT_CHECK_VALID(msg);

    if (JXTA_SUCCESS == jxta_message_get_ttl(msg, &ttl) && 0 == ttl) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%pp] is discarded with TTL 0\n", msg);
        return JXTA_SUCCESS;
    }

    if (JXTA_SUCCESS == jxta_message_get_lifespan(msg, &lifespan) && time(NULL) >= lifespan) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%pp] is discarded with lifespan %ld\n", msg, lifespan);
        return JXTA_SUCCESS;
    }

    dest = jxta_message_get_destination(msg);
    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "No destination. Message [%pp] is discarded.\n", msg);
        return JXTA_ITEM_NOTFOUND;
    }
    assert(NULL != jxta_endpoint_address_get_service_name(dest));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Outgoing message [%pp] -> %s://%s/%s/%s\n",
                    msg,
                    jxta_endpoint_address_get_protocol_name(dest),
                    jxta_endpoint_address_get_protocol_address(dest),
                    jxta_endpoint_address_get_service_name(dest), jxta_endpoint_address_get_service_params(dest));

    baseAddrStr = jxta_endpoint_address_get_transport_addr(dest);
    apr_thread_mutex_lock(me->nc_wlock);
    apr_thread_mutex_lock(me->mutex);
    ptr = apr_hash_get(me->nc, baseAddrStr, APR_HASH_KEY_STRING);
    apr_thread_mutex_unlock(me->mutex);
    if (NULL != ptr) {
        res = nc_peer_queue_msg(me, ptr, msg);
        apr_thread_mutex_unlock(me->nc_wlock);
    } else {
        apr_thread_mutex_unlock(me->nc_wlock);
        res = send_message(me, msg, dest);
        if (JXTA_UNREACHABLE_DEST == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is unreachable, queueing msg[%pp]\n", baseAddrStr, msg);
            ptr = nc_add_peer(me, baseAddrStr, msg);
        }
    }
    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Message [%pp] send successful.\n", msg);
    }
    free(baseAddrStr);
    JXTA_OBJECT_RELEASE(dest);

    if (apr_atomic_inc32(&cnt) >= jxta_epcfg_get_ncrq_retry(me->config) || 0 == apr_atomic_read32(&me->msg_task_cnt)) {
        apr_atomic_set32(&cnt, 0);
        nc_review_all(me);
    }

    return res;
}

static void check_nc_entry(Jxta_endpoint_service *me, const char *ta)
{
    Nc_entry *ptr = NULL;
    Jxta_status status;

    apr_thread_mutex_lock(me->nc_wlock);
    apr_thread_mutex_lock(me->mutex);
    ptr = apr_hash_get(me->nc, ta, APR_HASH_KEY_STRING);
    apr_thread_mutex_unlock(me->mutex);
    if (NULL != ptr) {
        ptr->expiration = 0;
        status = nc_peer_process_queue(me, ptr);
        if (JXTA_SUCCESS == status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is back online.\n", ptr->addr);
            nc_remove_peer(me, ptr);
        }
    }
    apr_thread_mutex_unlock(me->nc_wlock);
}

void jxta_endpoint_service_transport_event(Jxta_endpoint_service * me, Jxta_transport_event * e)
{
    char *addr = NULL;
    Jxta_endpoint_address *ea = NULL;
    Nc_entry *ptr = NULL;

    switch (e->type) {
    case JXTA_TRANSPORT_INBOUND_CONNECTED:
        assert(e->dest_addr);
        assert(e->peer_id);
        assert(e->msgr);

        addr = jxta_endpoint_address_get_transport_addr(e->dest_addr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Inbound connection from %s.\n", addr);

        ea = jxta_endpoint_address_new_3(e->peer_id, NULL, NULL);

        apr_thread_mutex_lock(me->mutex);
        messenger_add(me, e->dest_addr, e->msgr);
        if (ea) {
            messenger_add(me, ea, e->msgr);
        }
        apr_thread_mutex_unlock(me->mutex);

        check_nc_entry(me, addr);
        free(addr);
        if (ea) {
            addr = jxta_endpoint_address_get_transport_addr(ea);
            check_nc_entry(me, addr);
            free(addr);
            JXTA_OBJECT_RELEASE(ea);
        }
        break;
        
    case JXTA_TRANSPORT_OUTBOUND_CONNECTED:
        assert(e->dest_addr);
        assert(e->peer_id);
        assert(e->msgr);

        addr = jxta_endpoint_address_get_transport_addr(e->dest_addr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Outbound connection to %s.\n", addr);
        free(addr);

        ea = jxta_endpoint_address_new_3(e->peer_id, NULL, NULL);

        apr_thread_mutex_lock(me->mutex);
        messenger_add(me, e->dest_addr, e->msgr);
        if (ea) {
            messenger_add(me, ea, e->msgr);
            JXTA_OBJECT_RELEASE(ea);
        }
        apr_thread_mutex_unlock(me->mutex);
        break;

    case JXTA_TRANSPORT_CONNECTION_CLOSED:
        assert(e->dest_addr);
        assert(e->peer_id);

        addr = jxta_endpoint_address_get_transport_addr(e->dest_addr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Close connection to %s.\n", addr);
        free(addr);

        ea = jxta_endpoint_address_new_3(e->peer_id, NULL, NULL);
        apr_thread_mutex_lock(me->mutex);
        messenger_remove(me, e->dest_addr);
        if (ea) {
            addr = jxta_endpoint_address_get_transport_addr(ea);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Close connection with %s.\n", addr);
            free(addr);
            messenger_remove(me, ea);
            JXTA_OBJECT_RELEASE(ea);
        }
        apr_thread_mutex_unlock(me->mutex);
        break;

    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Ignore unknow transport event type %d.\n", e);
        break;
    }
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_get_thread_pool(Jxta_endpoint_service *me, apr_thread_pool_t **tp)
{
    *tp = jxta_PG_thread_pool_get(me->my_group);
    return JXTA_SUCCESS;
}

/* vim: set ts=4 sw=4 tw=130 et: */
