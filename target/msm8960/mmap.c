/* Copyright (c) 2009-2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Linux Foundation nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <reg.h>
#include <debug.h>
#include <assert.h>
#include <stdint.h>
#include <arch/arm/mmu.h>
#include <smem.h>
#include <board.h>
#include <baseband.h>

#define MB (1024u * 1024u)

#define PHYS_MEM_ADDR 0x80000000

typedef void* (*platform_mmap_cb_t)(void* pdata, paddr_t addr, size_t size, bool reserved);

typedef struct {
	size_t size;
	paddr_t addr;
} atag_mem_info;

static atag_mem_info apq8064_standalone_first_256M[] = {
	{	.size = (140 * MB),
		.addr = PHYS_MEM_ADDR + 2*MB
	},
	{	.size = (58 * MB),
		.addr = PHYS_MEM_ADDR + (160 * MB)
	},
	{	.size = (4 * MB),
		.addr = PHYS_MEM_ADDR + (236 * MB)
	},
	{	.size = (7 * MB),
		.addr = PHYS_MEM_ADDR + (247 * MB)
	}
};

static atag_mem_info apq8064_fusion_first_256M[] = {
	{	.size = (140 * MB),
		.addr = PHYS_MEM_ADDR + 2*MB
	},
	{	.size = (74 * MB),
		.addr = PHYS_MEM_ADDR + (144 * MB)
	},
	{	.size = (4 * MB),
		.addr = PHYS_MEM_ADDR + (236 * MB)
	},
	{	.size = (7 * MB),
		.addr = PHYS_MEM_ADDR + (247 * MB)
	},
	{	.size = MB,
		.addr = PHYS_MEM_ADDR + (255 * MB)
	}
};

static atag_mem_info mpq8064_first_256M[] = {
	{	.size = (140 * MB),
		.addr = PHYS_MEM_ADDR + 2*MB
	},
	{	.size = (74 * MB),
		.addr = PHYS_MEM_ADDR + (144 * MB)
	},
	{	.size = (20 * MB),
		.addr = PHYS_MEM_ADDR + (236 * MB)
	}
};

static atag_mem_info msm8960_default_first_256M[] = {
	{	.size = (140 * MB),
		.addr = PHYS_MEM_ADDR + 2*MB
	}
};

static atag_mem_info msm8930_default_first_256M[] = {
	{	.size = (140 * MB),
		.addr = PHYS_MEM_ADDR + 2*MB
	},
	{	.size = (4 * MB),
		.addr = PHYS_MEM_ADDR + (236 * MB)
	},
	{	.size = (3 * MB),
		.addr = PHYS_MEM_ADDR + (248 * MB)
	},
	{	.size = (1 * MB),
		.addr = PHYS_MEM_ADDR + (254 * MB)
	}
};

static void* platform_mmap_report_meminfo_table(void* pdata, platform_mmap_cb_t cb, atag_mem_info usable_mem_map[], unsigned num_regions)
{
	uint32_t i;

	paddr_t end_last = PHYS_MEM_ADDR;

	for (i=0; i < num_regions; i++) {
		if(usable_mem_map[i].addr>end_last)
			pdata = cb(pdata, end_last, usable_mem_map[i].addr-end_last, true);

		pdata = cb(pdata, usable_mem_map[i].addr, usable_mem_map[i].size, false);
		end_last = usable_mem_map[i].addr + usable_mem_map[i].size;
	}

	if(end_last!=PHYS_MEM_ADDR+256*MB)
		pdata = cb(pdata, end_last, (PHYS_MEM_ADDR+256*MB)-end_last, true);

	return pdata;
}

static void* platform_mmap_report_first_256MB(void* pdata, platform_mmap_cb_t cb)
{
	uint32_t platform_id = board_platform_id();
	uint32_t baseband = board_baseband();

	switch (platform_id) {
		case APQ8064:
		case APQ8064AA:
		case APQ8064AB:
			if(baseband == BASEBAND_MDM) {
				// Use 8064 Fusion 3 memory map
				pdata = platform_mmap_report_meminfo_table(pdata, cb,
							apq8064_fusion_first_256M,
							ARRAY_SIZE(apq8064_fusion_first_256M));
			}
			else {
				// Use 8064 standalone memory map
				pdata = platform_mmap_report_meminfo_table(pdata, cb,
							apq8064_standalone_first_256M,
							ARRAY_SIZE(apq8064_standalone_first_256M));
			}
			break;

		case MPQ8064:
			pdata = platform_mmap_report_meminfo_table(pdata, cb,
							mpq8064_first_256M,
							ARRAY_SIZE(mpq8064_first_256M));
			break;

		case MSM8130:
		case MSM8230:
		case MSM8930:
		case MSM8630:
		case MSM8130AA:
		case MSM8230AA:
		case MSM8630AA:
		case MSM8930AA:
		case MSM8930AB:
		case MSM8630AB:
		case MSM8230AB:
		case MSM8130AB:
		case APQ8030AB:
		case APQ8030:
		case APQ8030AA:
			pdata = platform_mmap_report_meminfo_table(pdata, cb,
						msm8930_default_first_256M,
						ARRAY_SIZE(msm8930_default_first_256M));
			break;

		case MSM8960: // fall through
		case MSM8960AB:
		case APQ8060AB:
		case MSM8260AB:
		case MSM8660AB:
		default:
			pdata = platform_mmap_report_meminfo_table(pdata, cb,
						msm8960_default_first_256M,
						ARRAY_SIZE(msm8960_default_first_256M));
			break;
	}

	return pdata;
}

void* platform_get_mmap(void* pdata, platform_mmap_cb_t cb) {
	uint32_t i;
	struct smem_ram_ptable ram_ptable;

	// Make sure RAM partition table is initialized
	ASSERT(smem_ram_ptable_init(&ram_ptable));
	for(i=0; i<ram_ptable.len; i++) {
		struct smem_ram_ptn part = ram_ptable.parts[i];

		if(part.category==SDRAM && part.type==SYS_MEMORY) {
			// patch first 256M
			if(part.start==PHYS_MEM_ADDR) {
				pdata = platform_mmap_report_first_256MB(pdata, cb);

				if (part.size > 256*MB) {
					pdata = cb(pdata, (paddr_t) part.start+256*MB, (size_t)part.size-256*MB, false);
				}
			}

			// Pass along all other usable memory regions to Linux
			else pdata = cb(pdata, (paddr_t) part.start, (size_t)part.size, false);
		}
	}

	return pdata;
}
