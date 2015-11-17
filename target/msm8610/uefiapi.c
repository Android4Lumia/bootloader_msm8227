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

extern struct mmc_device *dev;

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

void api_platform_uninit(void) {
	// from target_uninit
	mmc_put_card_to_sleep(dev);

	// Disable HC mode before jumping to kernel
	sdhci_mode_disable(&dev->host);
}

/////////////////////////////////////////////////////////////////////////
//                            BlockIO                                  //
/////////////////////////////////////////////////////////////////////////

int api_mmc_init(lkapi_biodev_t* biodev) {
	unsigned base_addr;
	unsigned char slot;
	static int initialized = 0;

	if(initialized)
		goto out;

	target_sdc_init();

	initialized = 1;

out:
	if(biodev)
		biodev->num_blocks = mmc_get_device_capacity()/biodev->block_size;
	return 0;
}

void* api_mmap_get_platform_mappings(void* pdata, lkapi_mmap_mappings_cb_t cb) {
	pdata = cb(pdata, MSM_IOMAP_BASE, MSM_IOMAP_BASE, (MSM_IOMAP_END - MSM_IOMAP_BASE), LKAPI_MEMORY_DEVICE);
	pdata = cb(pdata, SYSTEM_IMEM_BASE, SYSTEM_IMEM_BASE, 1*1024, LKAPI_MEMORY_DEVICE);

	return pdata;
}
