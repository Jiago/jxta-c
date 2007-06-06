/*
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: DiscoveryEvent.cs,v 1.1 2006/08/04 10:33:20 lankes Exp $
 */
using System;
using System.Text;
using System.Runtime.InteropServices;

namespace JxtaNET
{
    /// <summary>
    /// Container for DiscoveryService events. The source of the
    /// event is the Endpoint address of the responding peer
    /// </summary>
    public class DiscoveryEvent : JxtaObject
    {
        #region import of jxta-c functions
        [DllImport("jxta.dll")]
        private static extern Int32 jxta_discovery_response_get_query_id(IntPtr ad);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_discovery_response_get_responses(IntPtr dr, ref IntPtr responses);
        #endregion

        /// <summary>
        /// Response associated with the event
        /// </summary>
        public DiscoveryResponseMsg Response
        {
            get
            {
                IntPtr ptr = IntPtr.Zero;
                Errors.check(jxta_discovery_response_get_responses(this.self, ref ptr));
                return new DiscoveryResponseMsg(ptr);
            }
        }

        /// <summary>
        /// Query id associated with the response returned in this event
        /// </summary>
        public int QueryID
        {
            get
            {
                return jxta_discovery_response_get_query_id(this.self);
            }
        }

        internal DiscoveryEvent(IntPtr i) : base(i) { }
    }

    internal class DiscoveryEventImpl : DiscoveryEvent
    {
        public DiscoveryEventImpl(IntPtr i) : base(i) { }
    }
}