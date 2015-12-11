#include <err.h>
#include <debug.h>
#include <stdint.h>
#include <mmc.h>
#include <spmi.h>
#include <board.h>
#include <target.h>
#include <pm8x41.h>
#include <qtimer.h>
#include <dev/keys.h>
#include <dev/fbcon.h>
#include <mipi_dsi.h>
#include <sdhci_msm.h>
#include <target/display.h>
#include <platform/iomap.h>
#include <platform/clock.h>
#include <platform/gpio.h>
#include <partition_parser.h>

#include <uefiapi.h>

#define PMIC_ARB_CHANNEL_NUM    0
#define PMIC_ARB_OWNER_ID       0

static uint32_t pmic_ver;

/////////////////////////////////////////////////////////////////////////
//                                KEYS                                 //
/////////////////////////////////////////////////////////////////////////

#define TLMM_VOL_UP_BTN_GPIO    106

int platform_is_8974(void);

/* Return 1 if vol_up pressed */
static int target_volume_up(void)
{
	uint8_t status = 0;
	struct pm8x41_gpio gpio;

	/* CDP vol_up seems to be always grounded. So gpio status is read as 0,
	 * whether key is pressed or not.
	 * Ignore volume_up key on CDP for now.
	 */
	if (board_hardware_id() == HW_PLATFORM_SURF)
		return 0;

	/* Configure the GPIO */
	gpio.direction = PM_GPIO_DIR_IN;
	gpio.function  = 0;
	gpio.pull      = PM_GPIO_PULL_UP_30;
	gpio.vin_sel   = 2;

	pm8x41_gpio_config(5, &gpio);

	/* Wait for the pmic gpio config to take effect */
	thread_sleep(1);

	/* Get status of P_GPIO_5 */
	pm8x41_gpio_get(5, &status);

	return !status; /* active low */
}

/* Return 1 if vol_down pressed */
static uint32_t target_volume_down(void)
{
	/* Volume down button is tied in with RESIN on MSM8974. */
	if (platform_is_8974() && (pmic_ver == PM8X41_VERSION_V2))
		return pm8x41_v2_resin_status();
	else
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

	/* Save PM8941 version info. */
	pmic_ver = pm8x41_get_pmic_rev();

	keys_init();
	keys_add_source(&event_source);
}

void api_platform_uninit(void) {
	// from target_uninit
#if MMC_SDHCI_SUPPORT
	mmc_put_card_to_sleep(dev);
#else
	mmc_put_card_to_sleep(dev);
#endif

	// Disable HC mode before jumping to kernel
	sdhci_mode_disable(&dev->host);
}


/////////////////////////////////////////////////////////////////////////
//                            BlockIO                                  //
/////////////////////////////////////////////////////////////////////////

void set_sdc_power_ctrl(void);
void target_mmc_sdhci_init(void);
void target_mmc_mci_init(void);

int api_mmc_init(void) {
	/*
	 * Set drive strength & pull ctrl for
	 * emmc
	 */
	set_sdc_power_ctrl();

#if MMC_SDHCI_SUPPORT
	target_mmc_sdhci_init();
#else
	target_mmc_mci_init();
#endif

	return 0;
}

void* api_mmap_get_platform_mappings(void* pdata, lkapi_mmap_mappings_cb_t cb) {
	pdata = cb(pdata, MSM_IOMAP_BASE, MSM_IOMAP_BASE, (MSM_IOMAP_END - MSM_IOMAP_BASE), LKAPI_MEMORY_DEVICE);
	pdata = cb(pdata, SYSTEM_IMEM_BASE, SYSTEM_IMEM_BASE, 1*1024, LKAPI_MEMORY_DEVICE);

	return pdata;
}
