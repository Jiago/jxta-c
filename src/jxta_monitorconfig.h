/* 
 * Copyright (c) 2008 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_platformconfig.h 118 2007-08-16 00:18:01Z slowhog $
 */

#ifndef JXTA_MONITORCONFIG_H
#define JXTA_MONITORCONFIG_H

#include "jxta_types.h"
#include "jxta_pa.h"
#include "jxta_log.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
};
#endif

/**
 * Create the default MonitorConfig file
 * 
 * @return The peer advertisement 
 */
JXTA_DECLARE(Jxta_PA *) jxta_MonitorConfig_create_default(void);

/**
 * Read the monitorConfig file and returns it as a peer advertisement. 
 * 
 * @param fname Path of the platformConfig file
 * @return The peer advertisement 
 */
JXTA_DECLARE(Jxta_PA *) jxta_MonitorConfig_read(const char *fname);

/**
 * Write the monitorConfig file
 *
 * @param ad Config to be saved
 * @param fname Path path where to save
 */
JXTA_DECLARE(Jxta_status) jxta_MonitorConfig_write(Jxta_PA * ad, const char *fname);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_CONFIG_H */

/* vi: set ts=4 sw=4 tw=130 et: */