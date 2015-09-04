#include <err.h>
#include <debug.h>
#include <stdint.h>
#include <mmc.h>
#include <board.h>
#include <target.h>
#include <dev/keys.h>
#include <dev/ssbi.h>
#include <dev/pm8921.h>
#include <dev/fbcon.h>
#include <mipi_dsi.h>
#include <target/display.h>
#include <platform/iomap.h>
#include <platform/timer.h>
#include <partition_parser.h>

#include <uefiapi.h>

/////////////////////////////////////////////////////////////////////////
//                                KEYS                                 //
/////////////////////////////////////////////////////////////////////////

static int target_power_key(void)
{
	uint8_t ret = 0;

	pm8921_pwrkey_status(&ret);
	return ret;
}

static int event_source_poll(key_event_source_t* source) {
	uint16_t value = target_power_key();
	if(keys_set_report_key(source, 0, value)){
		keys_post_event(13, value);
	}

	return NO_ERROR;
}

static key_event_source_t event_source = {
	.poll = event_source_poll
};

/////////////////////////////////////////////////////////////////////////
//                            PLATFORM                                 //
/////////////////////////////////////////////////////////////////////////

void msm_clocks_init(void);
void platform_init_timer(void);
void apq8064_keypad_init(void);

static pm8921_dev_t pmic;

void api_platform_early_init(void) {
	// from platform_early_init, but without GIC
	msm_clocks_init();
	platform_init_timer();
	board_init();

	// UART
	target_early_init();
}

void api_platform_init(void) {
	// from target_init
	// Initialize PMIC driver
	pmic.read = (pm8921_read_func) & pa1_ssbi2_read_bytes;
	pmic.write = (pm8921_write_func) & pa1_ssbi2_write_bytes;
	pm8921_init(&pmic);

	keys_init();
	apq8064_keypad_init();
	keys_add_source(&event_source);
}

/////////////////////////////////////////////////////////////////////////
//                            BlockIO                                  //
/////////////////////////////////////////////////////////////////////////

extern char sn_buf[13];

static unsigned mmc_sdc_base[] =
    { MSM_SDC1_BASE, MSM_SDC2_BASE, MSM_SDC3_BASE, MSM_SDC4_BASE };

static int api_mmc_init(lkapi_biodev_t* dev) {
	unsigned base_addr;
	unsigned char slot;
	static int initialized = 0;

	if(initialized)
		goto out;

	/* Trying Slot 1 first */
	slot = 1;
	base_addr = mmc_sdc_base[slot - 1];
	if (mmc_boot_main(slot, base_addr)) {
		/* Trying Slot 3 next */
		slot = 3;
		base_addr = mmc_sdc_base[slot - 1];
		if (mmc_boot_main(slot, base_addr)) {
			dprintf(CRITICAL, "mmc init failed!");
			ASSERT(0);
		}
	}

	target_serialno((unsigned char *) sn_buf);
	dprintf(SPEW,"serial number: %s\n",sn_buf);

	initialized = 1;

out:
	if(dev)
		dev->num_blocks = mmc_get_device_capacity()/dev->block_size;
	return 0;
}

static int api_mmc_read(lkapi_biodev_t* dev, unsigned long long lba, unsigned long buffersize, void* buffer) {
	if(lba>dev->num_blocks-1)
		return -1;
	if(buffersize % dev->block_size)
		return -1;
	if(lba + (buffersize/dev->block_size) > dev->num_blocks)
		return -1;
	if(!buffer)
		return -1;
	if(!buffersize)
		return 0;

	int rc = mmc_read(BLOCK_SIZE * lba, buffer, buffersize);
	return rc != MMC_BOOT_E_SUCCESS;
}

static int api_mmc_write(lkapi_biodev_t* dev, unsigned long long lba, unsigned long buffersize, void* buffer) {
	if(lba>dev->num_blocks-1)
		return -1;
	if(buffersize % dev->block_size)
		return -1;
	if(lba + (buffersize/dev->block_size) > dev->num_blocks)
		return -1;
	if(!buffer)
		return -1;
	if(!buffersize)
		return 0;

// there's no reason that UEFI should be able to do this
// also it could be too risky because we have no experience with uncached buffers
#if 0
	int rc = mmc_write(BLOCK_SIZE * lba, buffersize, buffer);
	dprintf(CRITICAL, "%s(%p, %llu, %lu, %p) = %d\n", __func__, dev, lba, buffersize, buffer, rc);
	return rc != MMC_BOOT_E_SUCCESS;
#else
	ASSERT(0);
	return 0;
#endif
}

#define VNOR_SIZE 0x10000
static uint64_t vnor_lba_start = 0;
static uint64_t vnor_lba_count = (VNOR_SIZE/BLOCK_SIZE);

int vnor_init(lkapi_biodev_t* dev)
{
	api_mmc_init(NULL);

	int index = INVALID_PTN;
	uint64_t ptn = 0;
	uint64_t size;

	// get partition
	index = partition_get_index("aboot");
	ptn = partition_get_offset(index);
	if(ptn == 0) {
		return -1;
	}

	// get size
	size = partition_get_size(index);

	// calculate vnor offset
	vnor_lba_start = (ptn + size - VNOR_SIZE)/BLOCK_SIZE;

	return 0;
}

static int vnor_read(lkapi_biodev_t* dev, unsigned long long lba, unsigned long buffersize, void* buffer) {
	int rc = mmc_read(dev->block_size * (vnor_lba_start + lba), buffer, buffersize);
	return rc != MMC_BOOT_E_SUCCESS;
}

static int vnor_write(lkapi_biodev_t* dev, unsigned long long lba, unsigned long buffersize, void* buffer) {
	int rc = mmc_write(BLOCK_SIZE * (vnor_lba_start + lba), buffersize, buffer);
	return rc != MMC_BOOT_E_SUCCESS;
}

int api_bio_list(lkapi_biodev_t* list) {
	int count = 0, dev;

	// we currently don't need this and it could lead to misconfigurations
#if 0
	// VNOR
	dev = count++;
	if(list) {
		list[dev].type = LKAPI_BIODEV_TYPE_VNOR;
		list[dev].block_size = BLOCK_SIZE;
		list[dev].num_blocks = vnor_lba_count;
		list[dev].init = vnor_init;
		list[dev].read = vnor_read;
		list[dev].write = vnor_write;
	}
#endif

	// MMC
	dev = count++;
	if(list) {
		list[dev].type = LKAPI_BIODEV_TYPE_MMC;
		list[dev].block_size = BLOCK_SIZE;
		list[dev].num_blocks = 0;
		list[dev].init = api_mmc_init;
		list[dev].read = api_mmc_read;
		list[dev].write = api_mmc_write;
	}

	return count;
}
