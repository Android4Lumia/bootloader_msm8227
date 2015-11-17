/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <debug.h>
#include <reg.h>
#include <platform/iomap.h>
#include <qgic.h>
#include <qtimer.h>
#include <platform/clock.h>
#include <arch/arm/mmu.h>
#include <mmu.h>
#include <smem.h>
#include <board.h>
#include <target/display.h>

#define MSM_IOMAP_SIZE     ((MSM_IOMAP_END - MSM_IOMAP_BASE)/MB)
#define MSM_SHARED_SIZE    2

/* LK memory - cacheable, write back */
#define LK_MEMORY         (MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | \
                           MMU_MEMORY_AP_READ_WRITE)

/* Peripherals - non-shared device */
#define IOMAP_MEMORY      (MMU_MEMORY_TYPE_DEVICE_SHARED | \
                           MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN | MMU_MEMORY_PXN)

/* SCRATCH memory - cacheable, write back */
#define SCRATCH_MEMORY       (MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | \
                           MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

/* COMMON memory - cacheable, write through */
#define COMMON_MEMORY       (MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH | \
                           MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

/* downlaod mode memory - cacheable, write through */
#define DLOAD_MEMORY       (MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH | \
                           MMU_MEMORY_AP_READ_ONLY | MMU_MEMORY_XN)

static uint64_t ddr_start;

static mmu_section_t default_mmu_section_table[] =
{
/*       Physical addr,    Virtual addr,     Mapping type ,              Size (in MB),            Flags */
    {    0x00000000,        0x00000000,       MMU_L2_NS_SECTION_MAPPING,  512,                IOMAP_MEMORY},
    {    MEMBASE,           MEMBASE,          MMU_L2_NS_SECTION_MAPPING,  (MEMSIZE / MB),      LK_MEMORY},
    {    KERNEL_ADDR,       KERNEL_ADDR,      MMU_L2_NS_SECTION_MAPPING,  KERNEL_SIZE,         SCRATCH_MEMORY},
    {    SCRATCH_ADDR,      SCRATCH_ADDR,     MMU_L2_NS_SECTION_MAPPING,  SCRATCH_SIZE,        SCRATCH_MEMORY},
    {    MSM_SHARED_BASE,   MSM_SHARED_BASE,  MMU_L2_NS_SECTION_MAPPING,  MSM_SHARED_SIZE,     COMMON_MEMORY},
    {    RPMB_SND_RCV_BUF,  RPMB_SND_RCV_BUF, MMU_L2_NS_SECTION_MAPPING,  RPMB_SND_RCV_BUF_SZ, IOMAP_MEMORY},
};

/* Map the ddr for download mode, this region belongs to non-hlos images and pil */
static mmu_section_t dload_mmu_section_table[] =
{
/*    Physical addr,    Virtual addr,     Mapping type ,              Size (in MB),      Flags */
    { 0x85800000,       0x85800000,       MMU_L2_NS_SECTION_MAPPING,  189,               DLOAD_MEMORY},
};

void platform_early_init(void)
{
	board_init();
	platform_clock_init();
	qgic_init();
	qtimer_init();
	scm_init();
}

void platform_init(void)
{
	dprintf(INFO, "platform_init()\n");
}

void platform_uninit(void)
{
#if DISPLAY_SPLASH_SCREEN
	display_shutdown();
#endif

	qtimer_uninit();
}

int platform_use_identity_mmu_mappings(void)
{
	/* Use only the mappings specified in this file. */
	return 0;
}

/* Setup memory for this platform */
void platform_init_mmu_mappings(void)
{
	int i;
	int table_sz = ARRAY_SIZE(default_mmu_section_table);

	/* Map default memory needed for lk , scratch, rpmb & iomap */
	for (i = 0 ; i < table_sz; i++)
		arm_mmu_map_entry(&default_mmu_section_table[i]);

	if (scm_device_enter_dload())
	{
		/* TZ & Hyp memory can be mapped only while entering the download mode */
		table_sz = ARRAY_SIZE(dload_mmu_section_table);

		for (i = 0 ; i < table_sz; i++)
			arm_mmu_map_entry(&dload_mmu_section_table[i]);
	}
}

addr_t platform_get_virt_to_phys_mapping(addr_t virt_addr)
{
	return virtual_to_physical_mapping(virt_addr);
}

addr_t platform_get_phys_to_virt_mapping(addr_t phys_addr)
{
	return physical_to_virtual_mapping(phys_addr);
}

uint32_t platform_get_sclk_count(void)
{
	return readl(MPM2_MPM_SLEEP_TIMETICK_COUNT_VAL);
}

addr_t get_bs_info_addr()
{
	return BS_INFO_ADDR;
}

uint32_t platform_get_qmp_rev()
{
	return readl(USB3_PHY_REVISION_ID3) << 24 | readl(USB3_PHY_REVISION_ID2) << 16 |
		   readl(USB3_PHY_REVISION_ID1) << 8 | readl(USB3_PHY_REVISION_ID0);
}

uint32_t platform_get_max_periph()
{
	return 256;
}

int platform_is_msm8996()
{
	if (board_platform_id() == MSM8996)
		return 1;
	else
		return 0;
}

uint64_t platform_get_ddr_start()
{
	return ddr_start;
}

bool platform_use_qmp_misc_settings()
{
	if (board_soc_version() < 0x30000)
		return true;

	return false;
}
