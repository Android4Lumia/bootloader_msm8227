#include <err.h>
#include <debug.h>
#include <stdint.h>
#include <mmc.h>
#include <board.h>
#include <target.h>
#include <pm8x41.h>
#include <dev/keys.h>
#include <dev/fbcon.h>
#include <mipi_dsi.h>
#include <target/display.h>
#include <platform/iomap.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <partition_parser.h>

#include <uefiapi.h>

#define PMIC_ARB_CHANNEL_NUM    0
#define PMIC_ARB_OWNER_ID       0

/////////////////////////////////////////////////////////////////////////
//                                KEYS                                 //
/////////////////////////////////////////////////////////////////////////

#define TLMM_VOL_UP_BTN_GPIO    72

/* Return 1 if vol_up pressed */
static int target_volume_up(void)
{
	uint8_t status = 0;

	gpio_tlmm_config(TLMM_VOL_UP_BTN_GPIO, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA, GPIO_ENABLE);

	/* Wait for the configuration to complete.*/
	mdelay(1);
	/* Get status of GPIO */
	status = gpio_status(TLMM_VOL_UP_BTN_GPIO);

	/* Active low signal. */
	return !status;
}

/* Return 1 if vol_down pressed */
static uint32_t target_volume_down(void)
{
	/* Volume down button tied in with PMIC RESIN. */
	return pm8x41_resin_status();
}

static int target_power_key(void)
{
	return pm8x41_get_pwrkey_is_pressed();
}

static int event_source_poll(key_event_source_t* source) {
	uint16_t value = target_power_key();
	if(keys_set_report_key(source, 0, value)){
		keys_post_event(13, value);
	}

	value = target_volume_up();
	if(keys_set_report_key(source, 1,value)){
		keys_post_event(0x1b, value);
		keys_post_event(0x5b, value);
		keys_post_event(0x41, value);
	}

	value = target_volume_down();
	if(keys_set_report_key(source, 2,value)){
		keys_post_event(0x1b, value);
		keys_post_event(0x5b, value);
		keys_post_event(0x42, value);
	}

	return NO_ERROR;
}

static key_event_source_t event_source = {
	.poll = event_source_poll
};

/////////////////////////////////////////////////////////////////////////
//                            PLATFORM                                 //
/////////////////////////////////////////////////////////////////////////

void api_platform_early_init(void) {
	// from platform_early_init, but without GIC
	board_init();
	platform_clock_init();
	qtimer_init();

	// UART
	target_early_init();
}

void api_platform_init(void) {
	// from target_init
	// Initialize PMIC driver
	spmi_init(PMIC_ARB_CHANNEL_NUM, PMIC_ARB_OWNER_ID);

	keys_init();
	keys_add_source(&event_source);
}

/////////////////////////////////////////////////////////////////////////
//                            BlockIO                                  //
/////////////////////////////////////////////////////////////////////////

static int api_mmc_init(lkapi_biodev_t* dev) {
	unsigned base_addr;
	unsigned char slot;
	static int initialized = 0;

	if(initialized)
		goto out;

	target_sdc_init();

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
	return rc != 0;
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
	return rc != 0;
#else
	ASSERT(0);
	return 0;
#endif
}

int api_bio_list(lkapi_biodev_t* list) {
	int count = 0, dev;

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

void* api_mmap_get_platform_mappings(void* pdata, lkapi_mmap_mappings_cb_t cb) {
	pdata = cb(pdata, MSM_IOMAP_BASE, MSM_IOMAP_BASE, (MSM_IOMAP_END - MSM_IOMAP_BASE), LKAPI_MEMORY_DEVICE);
	pdata = cb(pdata, SYSTEM_IMEM_BASE, SYSTEM_IMEM_BASE, 1*1024, LKAPI_MEMORY_DEVICE);

	return pdata;
}
