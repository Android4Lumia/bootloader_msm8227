/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <reg.h>
#include <sys/types.h>
#include <platform/iomap.h>

#include "smem.h"

static struct smem *smem;

/* DYNAMIC SMEM REGION feature enables LK to dynamically
 * read the SMEM addr info from TCSR register or IMEM location.
 * The first word read, if indicates a MAGIC number, then
 * Dynamic SMEM is assumed to be enabled. Read the remaining
 * SMEM info for SMEM Size and Phy_addr from the other bytes.
 */

#if DYNAMIC_SMEM
uint32_t smem_get_base_addr()
{
	struct smem_addr_info *smem_info = NULL;

	smem_info = (struct smem_addr_info *) SMEM_TARG_INFO_ADDR;
	if(smem_info && (smem_info->identifier == SMEM_TARGET_INFO_IDENTIFIER))
		return smem_info->phy_addr;
	else
		return MSM_SHARED_BASE;
}
#endif

/* buf MUST be 4byte aligned, and len MUST be a multiple of 8. */
unsigned smem_read_alloc_entry(smem_mem_type_t type, void *buf, int len)
{
	struct smem_alloc_info *ainfo;
	unsigned *dest = buf;
	unsigned src;
	unsigned size;
	uint32_t smem_addr = 0;

#if DYNAMIC_SMEM
	smem_addr = smem_get_base_addr();
#else
	smem_addr = platform_get_smem_base_addr();
#endif

	smem = (struct smem *)smem_addr;

	if (((len & 0x3) != 0) || (((unsigned)buf & 0x3) != 0))
		return 1;

	if (type < SMEM_FIRST_VALID_TYPE || type > SMEM_LAST_VALID_TYPE)
		return 1;

	/* TODO: Use smem spinlocks */
	ainfo = &smem->alloc_info[type];
	if (readl(&ainfo->allocated) == 0)
		return 1;

	size = readl(&ainfo->size);

	if (size < (unsigned)((len + 7) & ~0x00000007))
		return 1;

	src = smem_addr + readl(&ainfo->offset);
	for (; len > 0; src += 4, len -= 4)
		*(dest++) = readl(src);

	return 0;
}

/* Return a pointer to smem_item with size */
void* smem_get_alloc_entry(smem_mem_type_t type, uint32_t* size)
{
	struct smem_alloc_info *ainfo = NULL;
	uint32_t smem_addr = 0;
	uint32_t base_ext = 0;
	uint32_t offset = 0;
	void *ret = NULL;

#if DYNAMIC_SMEM
	smem_addr = smem_get_base_addr();
#else
	smem_addr = platform_get_smem_base_addr();
#endif
	smem = (struct smem *)smem_addr;

	if (type < SMEM_FIRST_VALID_TYPE || type > SMEM_LAST_VALID_TYPE)
		return ret;

	ainfo = &smem->alloc_info[type];
	if (readl(&ainfo->allocated) == 0)
		return ret;

	*size = readl(&ainfo->size);
	base_ext = readl(&ainfo->base_ext);
	offset = readl(&ainfo->offset);

	if(base_ext)
	{
		ret = (void*)base_ext + offset;
	}
	else
	{
		ret = (void*) smem_addr + offset;
	}

	return ret;
}

unsigned
smem_read_alloc_entry_offset(smem_mem_type_t type, void *buf, int len,
			     int offset)
{
	struct smem_alloc_info *ainfo;
	unsigned *dest = buf;
	unsigned src;
	unsigned size = len;
	uint32_t smem_addr = 0;

#if DYNAMIC_SMEM
	smem_addr = smem_get_base_addr();
#else
	smem_addr = platform_get_smem_base_addr();
#endif

	smem = (struct smem *)smem_addr;

	if (((len & 0x3) != 0) || (((unsigned)buf & 0x3) != 0))
		return 1;

	if (type < SMEM_FIRST_VALID_TYPE || type > SMEM_LAST_VALID_TYPE)
		return 1;

	ainfo = &smem->alloc_info[type];
	if (readl(&ainfo->allocated) == 0)
		return 1;

	src = smem_addr + readl(&ainfo->offset) + offset;
	for (; size > 0; src += 4, size -= 4)
		*(dest++) = readl(src);

	return 0;
}

const char* smem_attr2str(int i) {
	switch(i) {
		case DEFAULT_ATTRB: return "default";
		case READ_ONLY: return "ro";
		case READWRITE: return "rw";
		default: return "-";
	}
}

const char* smem_category2str(int i) {
	switch(i) {
		case DEFAULT_CATEGORY: return "default";
		case SMI: return "smi";
		case EBI1: return "ebi1";
		case EBI2: return "ebi2";
		case QDSP6: return "qdsp6";
		case IRAM: return "iram";
		case IMEM: return "imem";
		case EBI0_CS0: return "ebi0_cs0";
		case EBI0_CS1: return "ebi0_cs1";
		case EBI1_CS0: return "ebi1_cs0";
		case EBI1_CS1: return "ebi1_cs1";
		case SDRAM: return "sdram";
		default: return "-";
	}
}

const char* smem_domain2str(int i) {
	switch(i) {
		case DEFAULT_DOMAIN: return "default";
		case APPS_DOMAIN: return "apps";
		case MODEM_DOMAIN: return "modem";
		case SHARED_DOMAIN: return "shared";
		default: return "-";
	}
}

const char* smem_type2str(int i) {
	switch(i) {
		case SYS_MEMORY: return "sys";
		case BOOT_REGION_MEMORY1: return "boot1";
		case BOOT_REGION_MEMORY2: return "boot2";
		case APPSBL_MEMORY: return "appsbl";
		case APPS_MEMORY: return "apps";
		default: return "-";
	}
}
