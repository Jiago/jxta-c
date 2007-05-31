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
 * $Id: jxta_object.c,v 1.32 2005/03/30 00:49:08 slowhog Exp $
 */

static const char *__log_cat = "OBJECT";

#include "jxtaapr.h"

#include "jxta_log.h"
#include "jxta_object.h"

#define MAX_REF_COUNT 10000

/**
 * Forward definition. Makes compilers happier.
 **/
static int jxta_object_increment_refcount(Jxta_object * obj);

static int jxta_object_decrement_refcount(Jxta_object * obj);

static void jxta_object_set_refcount(Jxta_object * obj, int count);

static Jxta_boolean jxta_object_check_initialized(Jxta_object *, const char *, const int);

static Jxta_object *jxta_object_set_initialized(Jxta_object *, const char *, const int);

static Jxta_object *jxta_object_set_uninitialized(Jxta_object *, const char *, const int);

/**
    Change this to #define to enable the KDB Server. The KDB server
    allows you to remotely see what JXTA objects are being tracked.
**/
#undef KDB_SERVER

#ifdef KDB_SERVER
void start_kdb_server(void);
#endif

/**
 * This module uses a global mutex in order to protect its
 * critical section, such as changing the reference count of an
 * object. In therory, there should be a mutex per object, but since
 * incrementing or decrementing the reference count is very fast,
 * it is better to save some memory (only one mutex allocated versus
 * one per object).
 **/

static apr_thread_mutex_t *jxta_object_mutex = NULL;
static apr_pool_t *jxta_object_pool = NULL;
static Jxta_boolean jxta_object_initialized = FALSE;

/**
 * Initialize the global mutex. Since we do want this module
 * to be initialized "by itself", in other words, when it's first
 * used, this function must be idempotent.
 *
 * Note that this function cannot be called concurrently. However,
 * there is a risk to have it invoked concurrentely. The final test
 * on jxta_object_initialized should catch most of the problems.
 * lomax@jxta.org: to be revisited. The proper way to initialize this
 * module would be to have a static initializer invoked at boot time.
 * jice@jxta.org - 20020204 : Actualy, the POSIX way is to use thread-once
 * because static initialization order is practicaly impossible to control.
 **/
void jxta_object_initialize(void)
{
    apr_status_t res;

    if (jxta_object_initialized) {
        /* Already initialized. Nothing to do. */
        return;
    }

    /* Initialize the global mutex. */
    res = apr_pool_create(&jxta_object_pool, NULL);
    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot allocate pool for jxta_object_mutex\n");
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Allocated pool fot jxta_object_mutex\n");

    /* Create the mutex */
    res = apr_thread_mutex_create(&jxta_object_mutex, APR_THREAD_MUTEX_NESTED, jxta_object_pool);
    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot jxta_object_mutex\n");
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Allocated the global jxta_object_mutex\n");

    if (jxta_object_initialized) {
        /* This should not be the case */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_object_initialized invoked concurrently !!!\n");
    }
#ifdef KDB_SERVER
    start_kdb_server();
#endif

    jxta_object_initialized = TRUE;
}

/**
 * Get the mutex.
 **/
static void jxta_object_mutexGet(void)
{
    apr_status_t res;

    jxta_object_initialize();
    res = apr_thread_mutex_lock(jxta_object_mutex);
    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Error getting jxta_object_mutex\n");
    }
}

/**
 * Release the mutex.
 **/
static void jxta_object_mutexRel(void)
{
    apr_status_t res;

    jxta_object_initialize();
    res = apr_thread_mutex_unlock(jxta_object_mutex);
    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Error getting jxta_object_mutex\n");
    }
}

static int jxta_object_increment_refcount(Jxta_object * obj)
{
    int count;

    jxta_object_mutexGet();
    count = ++obj->_refCount;
    jxta_object_mutexRel();
    return count;
}

static int jxta_object_decrement_refcount(Jxta_object * obj)
{
    int count;

    jxta_object_mutexGet();
    count = --obj->_refCount;
    jxta_object_mutexRel();
    return count;
}

int _jxta_object_get_refcount(Jxta_object * obj)
{
    int count;

    jxta_object_mutexGet();
    count = obj->_refCount;
    jxta_object_mutexRel();
    return count;
}

static void jxta_object_set_refcount(Jxta_object * obj, int count)
{
    jxta_object_mutexGet();
    obj->_refCount = count;
    jxta_object_mutexRel();
}

Jxta_boolean _jxta_object_check_valid(Jxta_object * obj, const char *file, int line)
{
    Jxta_boolean res;

    if (obj == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object is NULL\n", file, line);
        return FALSE;
    }
    jxta_object_mutexGet();

    res = jxta_object_check_initialized(obj, file, line);
    if (!res)
        goto Common_Exit;

    res = ((obj->_refCount > 0) && (obj->_refCount < MAX_REF_COUNT));
    if (!res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                        FILEANDLINE "from [%s:%d] *********** Object [%p] is not valid (ref count is %d)\n", file, line, obj,
                        obj->_refCount);
    }

  Common_Exit:
    jxta_object_mutexRel();

    return res;
}

/*
 * Just in case someone tries to free a static object.
 * We need to make sure that the reference count is back to one.
 */
void jxta_object_freemenot(Jxta_object * addr)
{
    if (NULL == addr)
        return;

    jxta_object_set_initialized(addr, __FILE__, __LINE__);
    JXTA_OBJECT_SHARE(addr);
}

static Jxta_boolean jxta_object_check_initialized(Jxta_object * obj, const char *file, int line)
{
    if (NULL == obj) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object is NULL\n", file, line);
        return FALSE;
    }

    if (0 == (obj->_bitset & JXTA_OBJECT_INITED_BIT)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object [%p] not initialized\n",
                        file, line, obj);
        return FALSE;
    }
#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
    if (obj->_initialized != JXTA_OBJECT_MAGIC_NUMBER) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object [%p] not initialized\n",
                        file, line, obj);
        return FALSE;
    }
#endif

    return TRUE;
}

static Jxta_boolean jxta_object_check_uninitialized(Jxta_object * obj, const char *file, int line)
{
    if (NULL == obj) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object is NULL\n", file, line);
        return FALSE;
    }
#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
    if (obj->_initialized == JXTA_OBJECT_MAGIC_NUMBER) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object [%p] already initialized\n",
                        file, line, obj);
        return FALSE;
    }
#endif

    return TRUE;
}

static Jxta_object *jxta_object_set_initialized(Jxta_object * obj, const char *file, int line)
{
    if (NULL == obj) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object is NULL\n", file, line);
    } else {
        obj->_bitset |= JXTA_OBJECT_INITED_BIT;
#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
        obj->_initialized = JXTA_OBJECT_MAGIC_NUMBER;
#endif
    }

    return obj;
}

static Jxta_object *jxta_object_set_uninitialized(Jxta_object * obj, const char *file, int line)
{
    if (NULL == obj) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "from [%s:%d] *********** Object is NULL\n", file, line);
    } else {
        obj->_bitset &= ~JXTA_OBJECT_INITED_BIT;
#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
        obj->_initialized = 0;
#endif
    }

    return obj;
}

#ifdef JXTA_OBJECT_TRACKER
static void count_freed(Jxta_object *);
static void count_init(Jxta_object *);
#endif

void *_jxta_object_init(Jxta_object * obj, unsigned int flags, JXTA_OBJECT_FREE_FUNC freefunc, void *cookie, const char *file,
                        int line)
{
#ifdef JXTA_OBJECT_TRACKER
    char *pt = malloc(strlen(file) + 20);
    sprintf(pt, "%s:%d", file, line);
#endif

    jxta_object_check_uninitialized(obj, file, line);

    obj->_bitset = flags;
    obj->_free = freefunc;
    obj->_freeCookie = cookie;
#ifdef JXTA_OBJECT_TRACKER
    obj->_name = pt;
    count_init(obj);
#endif
    obj->_refCount = 1;

    jxta_object_set_initialized(obj, file, line);

    return obj;
}

void *_jxta_object_share(Jxta_object * obj, const char *file, int line)
{
    jxta_object_mutexGet();

    if (jxta_object_check_initialized(obj, file, line)) {
        int newRefCount = 1;

        if (0 == (obj->_bitset & JXTA_OBJECT_IMMUTABLE_FLAG)) {
            newRefCount = jxta_object_increment_refcount(JXTA_OBJECT(obj));
        } else {
            newRefCount = 1;
        }

        if (0 != (obj->_bitset & JXTA_OBJECT_SHARE_TRACK)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "[%s:%d] SHARE obj[%p]=%d\n", file, line, obj, newRefCount);
        }
    }

    jxta_object_mutexRel();

    return obj;
}

int _jxta_object_release(Jxta_object * obj, const char *file, int line)
{
    int newRefCount = -1;

    jxta_object_mutexGet();

    if (_jxta_object_check_valid(obj, file, line)) {
        if (0 == (obj->_bitset & JXTA_OBJECT_IMMUTABLE_FLAG)) {
            newRefCount = jxta_object_decrement_refcount(JXTA_OBJECT(obj));

            if (newRefCount == 0) {
#ifdef JXTA_OBJECT_TRACKER
                count_freed(obj);
#endif

                jxta_object_set_uninitialized(obj, file, line);
            }
        } else {
            newRefCount = 1;
        }

        if (0 != (obj->_bitset & JXTA_OBJECT_SHARE_TRACK)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "[%s:%d] RELEASE obj[%p]=%d\n", file, line, obj, newRefCount);
        }
    }

    jxta_object_mutexRel();

    /* Call freefunc from outside of the mutex */
    if (0 == newRefCount) {
        if (NULL != JXTA_OBJECT_GET_FREE_FUNC(obj)) {
            JXTA_OBJECT_GET_FREE_FUNC(obj) ((void *) (obj));
        }
    }

    return newRefCount;
}

#ifdef JXTA_OBJECT_TRACKER

/****************************************************
 ** Object tracker for debug purposes
 **
 ** We cannot use any of the tools (vector, hashtable,
 ** jstring...) since we want to use this tool in order
 ** to monitor them as well.
 ****************************************************/

#define MAX_NB_OF_ENTRIES 5000

typedef struct {
    char *name;
    int created;
    int freed;
    int current;
} _object_entry;

_object_entry *_jxta_object_table[MAX_NB_OF_ENTRIES];
int _nbOfObjectEntries = 0;

static void add_new_entry(char *name)
{
    _object_entry *o = NULL;

    jxta_object_mutexGet();

    if (_nbOfObjectEntries >= MAX_NB_OF_ENTRIES) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "********** JXTA OBJECT TRACKER: cannot allocate new object\n");
        jxta_object_mutexRel();
        return;
    }
    o = malloc(sizeof(_object_entry));
    o->name = malloc(strlen(name) + 1);
    strcpy(o->name, name);
    o->created = 0;
    o->freed = 0;
    o->current = 0;

    _jxta_object_table[_nbOfObjectEntries++] = o;
    jxta_object_mutexRel();
}

static _object_entry *get_object_entry(Jxta_object * obj)
{
    _object_entry *o = NULL;
    int i = 0;

    jxta_object_mutexGet();
    /**
     ** 3/16/2002 FIXME lomax@jxta.org
     ** I know: the following code is not efficient. Since this code runs
     ** only when JXTA_OBJECT_TRACKER is enabled, which is not the default,
     ** this is not a huge issue.
     ** However, something smarter could be done (any volonteer is welcome),
     ** like, for instance, sorting the table in an alphabitical order: the
     ** overhead is when creating a new type of object, which is a rare case,
     ** the typical case, updating an existing type of object, being then optimized
     ** by a dichotomic search... or whatever is smarter to do anyway :-)
     **/
    for (i = 0; i < _nbOfObjectEntries; ++i) {
        o = _jxta_object_table[i];
        if (strcmp(o->name, obj->_name) == 0) {
            jxta_object_mutexRel();
            return o;
        }
    }
    jxta_object_mutexRel();
    return NULL;
}

static void count_freed(Jxta_object * obj)
{
    _object_entry *o = get_object_entry(obj);

    if (o == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "********** JXTA OBJECT TRACKER: cannot find object\n");
        return;
    }

    jxta_object_mutexGet();
    o->current--;
    o->freed++;
    jxta_object_mutexRel();

}

static void count_init(Jxta_object * obj)
{
    _object_entry *o = get_object_entry(obj);

    if (o == NULL) {
        add_new_entry(obj->_name);
        o = get_object_entry(obj);
        if (o == NULL) {
            return;
        }
    }

    jxta_object_mutexGet();
    o->current++;
    o->created++;
    jxta_object_mutexRel();
}

void _print_object_table(void)
{
    _object_entry *o = NULL;
    int i = 0;

    printf("\nJXTA-C Object Tracker: %d types of objects\n", _nbOfObjectEntries);
    for (i = 0; i < _nbOfObjectEntries; ++i) {
        o = _jxta_object_table[i];
        printf("%-40screated=%8d freed=%8d current=%8d\n", o->name, o->created, o->freed, o->current);
    }
    printf("\n");

}

#endif

/*****************************************************************************
 ** 3/17/2002 FIXME lomax@jxta.org
 **
 ** The following code is temporary. It should be in its own file, and also
 ** be more sophisticated. But for the time being, it's useful:
 **
 ** telnet 127.0.0.1 10000  would make the jxta-c peer display its objects.
 *****************************************************************************/

#ifdef KDB_SERVER

#include "jpr/jpr_thread.h"
#include "jpr/jpr_excep.h"

#define KDB_SERVER_HOST  "127.0.0.1"
#define KDB_SERVER_PORT  10000


static void *APR_THREAD_FUNC kdb_server_main(apr_thread_t * thread, void *arg)
{
    apr_status_t status;
    apr_pool_t *pool;
    apr_socket_t *socket;
    apr_socket_t *sock_in;
    apr_sockaddr_t *sockaddr;

    apr_pool_create(&pool, 0);

    status = apr_sockaddr_info_get(&sockaddr, KDB_SERVER_HOST, APR_INET, KDB_SERVER_PORT, SOCK_STREAM, pool);

    if (APR_STATUS_IS_SUCCESS(status)) {

        status = apr_socket_create(&socket, APR_INET, SOCK_STREAM, pool);

        apr_setsocketopt(socket, APR_SO_REUSEADDR, 1);

        if (APR_STATUS_IS_SUCCESS(status)) {

            status = apr_bind(socket, sockaddr);
            if (APR_STATUS_IS_SUCCESS(status)) {
                status = apr_listen(socket, 50);
                if (APR_STATUS_IS_SUCCESS(status)) {

                    for (;;) {
                        printf("KDB Server: listening on 127.0.0.1 port 10000\n");
                        status = apr_accept(&sock_in, socket, pool);
                        if (APR_STATUS_IS_SUCCESS(status)) {
#ifdef JXTA_OBJECT_TRACKER
                            _print_object_table();
#endif

                            apr_socket_close(sock_in);
                        }
                    }
                }
            }
        }
    }
    apr_thread_exit(thread, 0);

    return NULL;
}


void start_kdb_server(void)
{

    apr_status_t status;
    apr_pool_t *pool;
    apr_thread_t *thread;

#ifdef JXTA_OBJECT_TRACKER

    apr_pool_create(&pool, 0);

    apr_thread_create(&thread, NULL,    /* no attr */
                      kdb_server_main, (void *) NULL, pool);
#else

    printf("************* Cannot start KDB server: JXTA_OBJECT_TRACKER is not enabled\n");
#endif
}
#endif