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
 * $Id: queue.h,v 1.3 2005/01/28 02:09:04 slowhog Exp $
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "jxta_types.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif /* __cplusplus */

typedef struct _Queue Queue;

Queue* queue_new (apr_pool_t *pool);
void   queue_free (Queue *q);
void   queue_enqueue (Queue *q, void *item);
int    queue_size (Queue* q);

/**
 * Wait up to a certain amount of time until an item is available
 * on the queue.
 *
 * @return an item from the queue or NULL if the function waited
 * for the maximum amount of time and there still wasn't anything enqueued.
 */
void*  queue_dequeue (Queue *q, apr_time_t max_time_to_wait_in_microsecs);

/**
 * Wait up to a certain amount of time until an item is available
 * on the queue.
 *
 * @return an item from the queue or NULL if the function waited
 * for the maximum amount of time and there still wasn't anything enqueued.
 */
Jxta_status queue_dequeue_1 (Queue *q, void **obj, apr_time_t max_time_to_wait_in_microsecs);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif /* __cplusplus */
#endif /* __QUEUE_H__ */
