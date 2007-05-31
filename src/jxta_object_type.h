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
 * $Id: jxta_object_type.h,v 1.5 2005/02/14 01:23:15 bondolo Exp $
 */

#ifndef JXTA_OBJECT_TYPE_H
#define JXTA_OBJECT_TYPE_H


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
/**
    Change the following to <tt>#define NO_RUNTIME_TYPE_CHECK</tt> to disable the
    runtime type checking of Jxta objects provided by the PTValid macro.
**/
#undef NO_RUNTIME_TYPE_CHECK
#ifndef name2
#define _name2(a, b) a##b
#define name2(a, b) _name2(a, b)
#endif
/**
 * Add Extends(type); at the top of the list of fields in a structure in order
 * to:
 * 1 - make that structure derive from "type", that is make this structure
 * compatible with "type".
 * 2 - add the "thisType" fields which the structure initializer should set to
 * a string that's the name of this structure's type if runtime type checking is
 * to be used by the routines that manipulate that structure.
 * If runtime type checking is not to be used then one is not obliged to use
 * "Extends" in order to derive from another struct; it can be replaced by
 * simply "type somename".
 */
#define Extends(type) type name2(_jxta_base_, type); const char* thisType
/**
 * Use this macro to add the runtime type checking field without deriving from
 * anything.
 * This is provided for syntactic uniformity mostly and to ensure consistency
 * with the runtime type checking macro (see PTValid). It expands as
 * "const char* thisType".
 */
#define Extends_nothing const char* thisType
/**
 * Checks that the two supplied strings (both reflecting a type name) are
 * identical. If not, aborts with a message emphasizing that a runtime
 * type checking caught an inconsistency.
 *
 * @param object the object
 * @param object_type the type of the object.
 * @param want_type the type that is desired.
 * @param file the source file in which the test is occuring.
 * @param file the source file line at which the test is occuring.
 * @return a pointer to the object. If object type does not match then function does not return.
 **/ extern void *jxta_assert_same_type(const void *object, const char *object_type, const char *want_type, const char *file, int line);

/**
 * This macro checks that the given object is of the given type.
 * Any object that has a pointer to a string version of the symbolic type name
 * located at the same offset than the "thisType" field of this type
 * will be deemed to be of that type. Objects that actualy are of the said type
 * will verify positively only if the thisType field was properly initialized.
 *
 * If the object does not meet these conditions, an error message is output and
 * excution is aborted.
 *
 * @param thing Pointer to the object which type is to be verified.
 * @param type The actual symbol for the type to be verified.
 **/
/* type * PTValid(type* thing, type ); */

#ifndef NO_RUNTIME_TYPE_CHECK
#define PTValid(thing, type) (type*) jxta_assert_same_type((thing), (((type*) (thing))->thisType), #type, __FILE__, __LINE__)
#else
#define PTValid(thing, type) ((type*)(thing))
#endif

#ifdef __cplusplus
}
#endif

#endif /* JXTA_OBJECT_TYPE_H */
