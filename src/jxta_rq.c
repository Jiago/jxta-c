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
 * $Id: jxta_rq.c,v 1.54 2005/02/02 02:58:32 exocetrick Exp $
 */

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_rq.h"
#include "jxta_xml_util.h"
#include "jxta_routea.h"
#include "jxta_apa.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/** Each of these corresponds to a tag in the
 * xml ad.
 */
enum tokentype {
    Null_,
    ResolverQuery_,
    Credential_,
    SrcPeerID_,
    SrcRoute_,
    HandlerName_,
    QueryID_,
    Query_
};


/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _ResolverQuery {
    Jxta_advertisement jxta_advertisement;
    char    * ResolverQuery;
    JString * Credential;
    Jxta_id * SrcPeerID;
    Jxta_RouteAdvertisement* route;
    JString * HandlerName;
    long      QueryID;
    JString * Query;

};

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void
handleResolverQuery(void * userdata, const XML_Char * cd, int len) {
    /*ResolverQuery * ad = (ResolverQuery*)userdata;*/
}

static void
handleCredential(void * userdata, const XML_Char * cd, int len) {
    ResolverQuery * ad = (ResolverQuery*)userdata;
    jstring_append_0(((struct _ResolverQuery *)ad)->Credential,cd,len);
}

static void
handleSrcPeerID(void * userdata, const XML_Char * cd, int len) {

    ResolverQuery * ad = (ResolverQuery*)userdata;
    char *tok;

    if (len > 0) {
        tok = malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd,len,tok);
        /* XXXX hamada@jxta.org it is possible to get called many times, some of which are
         * calls on white-space, we only care for the id therefore
         * we create the id only when we have data, this not an elagant way
         * of dealing with this.
         */
        if (*tok != '\0') {
            JString * tmps = jstring_new_2(tok);
            /* hamada@jxta.org it would be desireable to have a fucntion that
            takes a "char *" we would not need to create a JString to create 
            the ID
            */
            ad->SrcPeerID = NULL;
            jxta_id_from_jstring( &ad->SrcPeerID, tmps);
            JXTA_OBJECT_RELEASE(tmps);
        }
        free(tok);
    }
}


static void
handleSrcRoute(void * userdata, const XML_Char * cd, int len) {
    ResolverQuery* ad = (ResolverQuery *) userdata;

    Jxta_RouteAdvertisement * ra =
        jxta_RouteAdvertisement_new();

    jxta_resolver_query_set_src_peer_route(ad, ra);

    jxta_advertisement_set_handlers((Jxta_advertisement *)ra,
                                    ((Jxta_advertisement *)ad)->parser,
                                    (void*)ad);
}

static void
handleHandlerName(void * userdata, const XML_Char * cd, int len) {
    ResolverQuery * ad = (ResolverQuery*)userdata;
    jstring_append_0((JString *) ad->HandlerName,cd,len);
}


static void
handleQueryID(void * userdata, const XML_Char * cd, int len) {
    ResolverQuery * ad = (ResolverQuery*)userdata;
    char *tok;
    /* XXXX hamada@jxta.org this can be cleaned up once parsing is corrected */
    if (len > 0) {
        tok = malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd,len,tok);
        if (*tok != '\0') {
            ad->QueryID =(int) strtol(cd, NULL, 0);
        }
        free(tok);
    }
}



static void
handleQuery(void * userdata, const XML_Char * cd, int len) {
    ResolverQuery * ad = (ResolverQuery*)userdata;
    jstring_append_0(ad->Query,(char *)cd,len);
}

static void
trim_elements( ResolverQuery * adv ) {
    jstring_trim(adv->Credential);
    jstring_trim(adv->HandlerName);
    jstring_trim(adv->Query);
}
/**
 * The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */


/**
 * return a JString represntation of the advertisement
 * it is the responsiblity of the caller to release the JString object
 * @param adv a pointer to the advertisement.
 * @param doc  a pointer to the JString object representing the document.
 * @return Jxta_status 
 */

Jxta_status
jxta_resolver_query_get_xml(ResolverQuery * adv, JString** document) {

    JString * doc ;
    JString * tmps = NULL;
    char * buf ;
    Jxta_status status;
    Jxta_AccessPointAdvertisement* dest;

    if (adv == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    buf = malloc(128);
    memset(buf, 0, 128);
    trim_elements(adv);
    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    jstring_append_2(doc, "<!DOCTYPE jxta:ResolverQuery>");
    jstring_append_2(doc,"<jxta:ResolverQuery>\n");

    if (jstring_length(adv->Credential) != 0) {
        jstring_append_2(doc,"<jxta:Cred>");
        jstring_append_1(doc,adv->Credential);
        jstring_append_2(doc,"</jxta:Cred>\n");
    }

    jstring_append_2(doc,"<SrcPeerRoute>");
    dest = jxta_RouteAdvertisement_get_Dest(adv->route);
    jxta_RouteAdvertisement_get_xml(adv->route, &tmps);
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    JXTA_OBJECT_RELEASE(dest);
    jstring_append_2(doc,"</SrcPeerRoute>\n");

    jstring_append_2(doc,"<SrcPeerID>");
    jxta_id_to_jstring(adv->SrcPeerID, &tmps);
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc,"</SrcPeerID>\n");

    jstring_append_2(doc,"<HandlerName>");
    jstring_append_1(doc,adv->HandlerName);
    jstring_append_2(doc,"</HandlerName>\n");

    sprintf(buf,"%ld\n",adv->QueryID);
    jstring_append_2(doc,"<QueryID>");
    jstring_append_2(doc,buf);
    jstring_append_2(doc,"</QueryID>\n");
    /* done with buf, free it */
    free(buf);

    jstring_append_2(doc,"<Query>");
    status = jxta_xml_util_encode_jstring(adv->Query,& tmps);
    if (status != JXTA_SUCCESS ) {
        JXTA_LOG("error encoding the query, retrun status :%d\n",status);
        JXTA_OBJECT_RELEASE(doc);
        return status;
    }
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc,"</Query>\n");

    jstring_append_2(doc,"</jxta:ResolverQuery>\n");

    *document = doc;

    return JXTA_SUCCESS;
}

JString *
jxta_resolver_query_get_credential(ResolverQuery * ad) {
    if (ad->Credential) {
        jstring_trim(ad->Credential);
        JXTA_OBJECT_SHARE(ad->Credential);
    }
    return ad->Credential;
}

void
jxta_resolver_query_set_credential(ResolverQuery * ad, JString *credential) {

    if (ad == NULL) {
        return;
    }
    if(ad->Credential) {
        JXTA_OBJECT_RELEASE(ad->Credential);
        ad->Credential = NULL;
    }
    JXTA_OBJECT_SHARE(credential);
    ad->Credential = credential;
}

Jxta_id *
jxta_resolver_query_get_src_peer_id(ResolverQuery * ad) {
    if( ad->SrcPeerID ) {
        JXTA_OBJECT_SHARE( ad->SrcPeerID );
    }
    return  ad->SrcPeerID;
}

void
jxta_resolver_query_set_src_peer_id(ResolverQuery * ad, Jxta_id * id) {
    if (ad == NULL) {
        return;
    }
    if(ad->SrcPeerID) {
        JXTA_OBJECT_RELEASE(ad->SrcPeerID);
        ad->SrcPeerID = NULL;
    }
    JXTA_OBJECT_SHARE( id );
    ad->SrcPeerID = id;
}

Jxta_RouteAdvertisement *
jxta_resolver_query_get_src_peer_route(ResolverQuery * ad) {
    if( ad->route != NULL) {
        JXTA_OBJECT_SHARE( ad->route );
    }
    return  ad->route;
}

void
jxta_resolver_query_set_src_peer_route(ResolverQuery * ad, Jxta_RouteAdvertisement * route) {
    if (route == NULL) {
        return;
    }
    if(ad->route != NULL) {
        JXTA_OBJECT_RELEASE(ad->route);
    }

    ad->route = route;
    JXTA_OBJECT_SHARE(ad->route);
}

void
jxta_resolver_query_get_handlername(ResolverQuery * ad, JString ** str) {
    if( ad->HandlerName ) {
        jstring_trim(ad->HandlerName);
        JXTA_OBJECT_SHARE( ad->HandlerName );
    }
    *str = ad->HandlerName;
}

void
jxta_resolver_query_set_handlername(ResolverQuery * ad, JString *handlerName ) {
    if (ad == NULL) {
        return;
    }
    if(ad->HandlerName) {
        JXTA_OBJECT_RELEASE(ad->HandlerName);
        ad->HandlerName = NULL;
    }
    JXTA_OBJECT_SHARE(handlerName);
    ad->HandlerName = handlerName;
}

long
jxta_resolver_query_get_queryid(ResolverQuery * ad) {
    return ad->QueryID;
}

void
jxta_resolver_query_set_queryid(ResolverQuery * ad, long qid) {
    if (ad == NULL) {
        return;
    }
    ad->QueryID = qid;
}

void
jxta_resolver_query_get_query(ResolverQuery * ad, JString ** query) {

    if( ad->Query ) {
        jstring_trim(ad->Query);
        JXTA_OBJECT_SHARE( ad->Query);
    }
    *query = ad->Query;
}


void
jxta_resolver_query_set_query(ResolverQuery * ad, JString * query) {

    if (ad == NULL) {
        return;
    }
    if (ad->Query != NULL) {
        JXTA_OBJECT_RELEASE(ad->Query);
        ad->Query = NULL;
    }
    if (query != NULL) {
        ad->Query = query;
        JXTA_OBJECT_SHARE(ad->Query);
    }
}



/** Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
const static Kwdtab ResolverQuery_tags[] = {
            {"Null",              Null_,          NULL,               NULL
            },
            {"jxta:ResolverQuery",ResolverQuery_,*handleResolverQuery,NULL},
            {"jxta:Cred",        Credential_,   *handleCredential,   NULL},
            {"SrcPeerID",         SrcPeerID_,    *handleSrcPeerID,    NULL},
            {"SrcPeerRoute",      SrcRoute_,     *handleSrcRoute,     NULL},
            {"HandlerName",       HandlerName_,  *handleHandlerName,  NULL},
            {"QueryID",           QueryID_,      *handleQueryID,      NULL},
            {"Query",             Query_,        *handleQuery,        NULL},
            {NULL,                0,              0,                  NULL}
        };


/** Get a new instance of the ad.
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
ResolverQuery *
jxta_resolver_query_new(void) {

    ResolverQuery * ad;
    ad = (ResolverQuery *) malloc(sizeof(ResolverQuery));
    memset(ad, 0xda, sizeof(ResolverQuery));
    jxta_advertisement_initialize((Jxta_advertisement*)ad,
                                  "jxta:ResolverQuery",
                                  ResolverQuery_tags,
                                  (JxtaAdvertisementGetXMLFunc)jxta_resolver_query_get_xml,
                                  NULL, NULL,
                                  (FreeFunc)jxta_resolver_query_free);

    /* Fill in the required initialization code here. */
    ad->SrcPeerID   = jxta_id_nullID;
    ad->Credential  = jstring_new_0();
    ad->HandlerName = jstring_new_0();
    ad->QueryID     = -1;
    ad->Query       = jstring_new_0();
    ad->route       = NULL;

    /*
     * in theory we're supposed to share even nullID, although, normally
     * it is never freed, nomatter the ref-count, but let's not assume too much
     */
    JXTA_OBJECT_SHARE(jxta_id_nullID);
    ad->SrcPeerID = jxta_id_nullID;

    return ad;
}


ResolverQuery *
jxta_resolver_query_new_1(JString * handlername, JString * query, Jxta_id * src_pid, Jxta_RouteAdvertisement* route) {

    ResolverQuery * ad;
    JString *temps = NULL;

    ad = (ResolverQuery *) malloc(sizeof(ResolverQuery));
    memset(ad, 0xda, sizeof(ResolverQuery));
    jxta_advertisement_initialize((Jxta_advertisement*)ad,
                                  "jxta:ResolverQuery",
                                  ResolverQuery_tags,
                                  (JxtaAdvertisementGetXMLFunc)jxta_resolver_query_get_xml,
                                  NULL, NULL,
                                  (FreeFunc)jxta_resolver_query_free);

    /* Fill in the required initialization code here. */

    ad->Credential = jstring_new_0();
    jxta_id_to_jstring(src_pid, &temps);
    jxta_id_from_jstring(&ad->SrcPeerID, temps);
    JXTA_OBJECT_RELEASE(temps);
    ad->QueryID     = -1;
    ad->Query       = query;
    if (ad->Query != NULL)
        JXTA_OBJECT_SHARE(ad->Query);
    ad->HandlerName = jstring_new_2(jstring_get_string(handlername));
    ad->route = route;
    if (route != NULL)
        JXTA_OBJECT_SHARE(ad->route);
    return ad;
}


 /** 
  * Shred the memory going out.  Again,
  * if there ever was a segfault (unlikely,
  * of course), the hex value dddddddd will 
  * pop right out as a piece of memory accessed
  * after it was freed...
  */
void
jxta_resolver_query_free(ResolverQuery * ad) {

    if (ad->Credential) {
        JXTA_OBJECT_RELEASE(ad->Credential);
    }
    if (ad->SrcPeerID) {
        JXTA_OBJECT_RELEASE(ad->SrcPeerID);
    }
    if (ad->HandlerName) {
        JXTA_OBJECT_RELEASE(ad->HandlerName);
    }
    if (ad->Query) {
        JXTA_OBJECT_RELEASE(ad->Query);
    }
    if (ad->route != NULL) {
        JXTA_OBJECT_RELEASE(ad->route);
    }

    jxta_advertisement_delete((Jxta_advertisement*)ad);
    memset(ad, 0xdd, sizeof(ResolverQuery));
    free(ad);
}


void
jxta_resolver_query_parse_charbuffer(ResolverQuery * ad, const char * buf, int len) {

    jxta_advertisement_parse_charbuffer((Jxta_advertisement*)ad,buf,len);
}

void
jxta_resolver_query_parse_file(ResolverQuery * ad, FILE * stream) {
    jxta_advertisement_parse_file((Jxta_advertisement*)ad, stream);
}

#ifdef __cplusplus
}
#endif