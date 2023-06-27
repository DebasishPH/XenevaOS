/**
* BSD 2-Clause License
*
* Copyright (c) 2022-2023, Manas Kamal Choudhury
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**/

#ifndef _KMALLOC_H
#define _KMALLOC_H

#include <stdint.h>
#include <aurora.h>


#ifdef __cplusplus
extern "C" {
#endif
/* Meta data magic */
#define MAGIC_USED  0x16062002
#define MAGIC_FREE  0x05212023   /* the year to remember */

#pragma pack(push,1)
typedef struct _meta_data_ {
	int magic;
	int size;
	struct _meta_data_ *next;
	struct _meta_data_ *prev;
}meta_data_t;
#pragma pack(pop)
    /*
     * x86_64_kmalloc_initialize -- initialize
     * kernel malloc library with two pages
	 */
    void AuHeapInitialize();

	AU_EXTERN AU_EXPORT void     *kmalloc(unsigned int);				//< The standard function.
	AU_EXTERN AU_EXPORT void     *krealloc(void *, unsigned int);		//< The standard function.
	AU_EXTERN AU_EXPORT void     *kcalloc(unsigned long long, unsigned long long);		//< The standard function.
	AU_EXTERN AU_EXPORT void      kfree(void *);					//< The standard function.
	extern void kheap_debug();
	extern void kmalloc_debug_on(bool bit);

#ifdef __cplusplus
}
#endif

#endif


