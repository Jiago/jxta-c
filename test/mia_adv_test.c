/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>

#include <jxta.h>
#include <jxta_mia.h>
#include <jxta_id.h>




Jxta_boolean mia_test(int argc, char **argv)
{
    Jxta_MIA *ad;
    unsigned int i = 0;
    Jxta_advertisement *parm_adv;
    Jxta_vector *parm_vector;
    Jxta_id *msid;
    JString *field;

    FILE *testfile;
    JString *js;

    if (argc != 2) {

        printf("usage: ad <filename>\n");
        return FALSE;
    }

    jxta_advertisement_register_global_handler("jxta:MIA", (JxtaAdvertisementNewFunc) jxta_MIA_new);

    ad = jxta_MIA_new();

    testfile = fopen(argv[1], "r");

    jxta_MIA_parse_file(ad, testfile);

    fclose(testfile);


    jxta_MIA_get_xml(ad, &js);

    fprintf(stdout, "%s\n", jstring_get_string(js));


    JXTA_OBJECT_RELEASE(js);

    /*
     * Check accessors.
     */
    msid = jxta_MIA_get_MSID(ad);
    jxta_id_to_jstring(msid, &field);

    printf("MSID: %s\n", jstring_get_string(field));
    JXTA_OBJECT_RELEASE(field);

    field = jxta_MIA_get_Comp(ad);
    printf("Comp: %s\n", jstring_get_string(field));
    JXTA_OBJECT_RELEASE(field);

    field = jxta_MIA_get_Code(ad);
    printf("Code: %s\n", jstring_get_string(field));
    JXTA_OBJECT_RELEASE(field);

    field = jxta_MIA_get_PURI(ad);
    printf("PURI: %s\n", jstring_get_string(field));
    JXTA_OBJECT_RELEASE(field);

    field = jxta_MIA_get_Prov(ad);
    printf("Prov: %s\n", jstring_get_string(field));
    JXTA_OBJECT_RELEASE(field);

    field = jxta_MIA_get_Desc(ad);
    printf("Desc: %s\n", jstring_get_string(field));
    JXTA_OBJECT_RELEASE(field);

    field = jxta_MIA_get_Parm(ad);
    printf("Parm: %s\n", jstring_get_string(field));

    parm_adv = jxta_advertisement_new();
    jxta_advertisement_parse_charbuffer(parm_adv, jstring_get_string(field), jstring_length(field));
    jxta_advertisement_get_advs(parm_adv, &parm_vector);

    for (i = 0; i < jxta_vector_size(parm_vector); i++) {
      Jxta_MIA * parm_mia;
      JString * parm_mia_jstring;

      jxta_vector_get_object_at(parm_vector, JXTA_OBJECT_PPTR(&parm_mia), i);
      jxta_MIA_get_xml(parm_mia, &parm_mia_jstring);
      printf("Embbeded adv %i in PARM section is: %s\n", i, jstring_get_string(parm_mia_jstring)); 

      JXTA_OBJECT_RELEASE(parm_mia);
      JXTA_OBJECT_RELEASE(parm_mia_jstring);
    }
    
    JXTA_OBJECT_RELEASE(parm_vector);
    JXTA_OBJECT_RELEASE(field);
    JXTA_OBJECT_RELEASE(ad);
    JXTA_OBJECT_RELEASE(parm_adv);

    /* FIXME: Figure out a sensible way to test xml processing. */
    return TRUE;
}

int main(int argc, char **argv)
{
    int rv;

    jxta_initialize();
    rv = mia_test(argc, argv);
    jxta_terminate();
    if (TRUE == rv)
        rv = 0;
    else
        rv = -1;
    return rv;
}
