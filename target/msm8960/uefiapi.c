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

int api_mmc_init(lkapi_biodev_t* dev) {
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
