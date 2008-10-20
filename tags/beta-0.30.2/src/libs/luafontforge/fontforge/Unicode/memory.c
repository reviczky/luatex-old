/* Copyright (C) 2000-2007 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ustring.h"

/* wrappers around standard memory routines so we can trap errors. */
static void default_trap(void) {
    fprintf(stderr, "Attempt to allocate memory failed.\n" );
    abort();
}

static void (*trap)(void) = default_trap;

void galloc_set_trap(void (*newtrap)(void)) {
    if ( newtrap==NULL ) newtrap = default_trap;
    trap = newtrap;
}

void *galloc(long size) {
    void *ret;
    while (( ret = malloc(size))==NULL )
	trap();
    memset(ret,0x3c,size);		/* fill with random junk for debugging */
return( ret );
}

void *gcalloc(int cnt,long size) {
    void *ret;
    while (( ret = calloc(cnt,size))==NULL )
	trap();
return( ret );
}

void *grealloc(void *old,long size) {
    void *ret;
    while (( ret = realloc(old,size))==NULL )
	trap();
return( ret );
}

void gfree(void *old) {
    free(old);
}

char *copy(const char *str) {
    char *ret;

    if ( str==NULL )
return( NULL );
    ret = galloc(strlen(str)+1);
    strcpy(ret,str);
return( ret );
}

char *copyn(const char *str,long n) {
    char *ret;

    if ( str==NULL )
return( NULL );
    ret = galloc(n+1);
    memcpy(ret,str,n);
    ret[n]='\0';
return( ret );
}
