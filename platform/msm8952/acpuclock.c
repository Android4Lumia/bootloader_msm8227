/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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
#include <err.h>
#include <assert.h>
#include <debug.h>
#include <reg.h>
#include <platform/timer.h>
#include <platform/iomap.h>
#include <mmc.h>
#include <clock.h>
#include <platform/clock.h>
#include <platform.h>

#define MAX_LOOPS	500

void hsusb_clock_init(void)
{
	int ret;
	struct clk *iclk, *cclk;

	ret = clk_get_set_enable("usb_iface_clk", 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set usb_iface_clk ret = %d\n", ret);
		ASSERT(0);
	}

	ret = clk_get_set_enable("usb_core_clk", 133330000, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set usb_core_clk ret = %d\n", ret);
		ASSERT(0);
	}

	mdelay(20);

	iclk = clk_get("usb_iface_clk");
	cclk = clk_get("usb_core_clk");

	clk_disable(iclk);
	clk_disable(cclk);

	mdelay(20);

	/* Start the block reset for usb */
	writel(1, USB_HS_BCR);

	mdelay(20);

	/* Take usb block out of reset */
	writel(0, USB_HS_BCR);

	mdelay(20);

	ret = clk_enable(iclk);

	if(ret)
	{
		dprintf(CRITICAL, "failed to set usb_iface_clk after async ret = %d\n", ret);
		ASSERT(0);
	}

	ret = clk_enable(cclk);

	if(ret)
	{
		dprintf(CRITICAL, "failed to set usb_iface_clk after async ret = %d\n", ret);
		ASSERT(0);
	}
}

void clock_init_mmc(uint32_t interface)
{
	char clk_name[64];
	int ret;

	snprintf(clk_name, sizeof(clk_name), "sdc%u_iface_clk", interface);

	/* enable interface clock */
	ret = clk_get_set_enable(clk_name, 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set sdc1_iface_clk ret = %d\n", ret);
		ASSERT(0);
	}
}

/* Configure MMC clock */
void clock_config_mmc(uint32_t interface, uint32_t freq)
{
	int ret = 1;
	char clk_name[64];

	snprintf(clk_name, sizeof(clk_name), "sdc%u_core_clk", interface);

	if(freq == MMC_CLK_400KHZ)
	{
		ret = clk_get_set_enable(clk_name, 400000, 1);
	}
	else if(freq == MMC_CLK_50MHZ)
	{
		ret = clk_get_set_enable(clk_name, 50000000, 1);
	}
	else if(freq == MMC_CLK_177MHZ)
	{
		ret = clk_get_set_enable(clk_name, 177770000, 1);
	}
	else if(freq == MMC_CLK_192MHZ)
	{
		if (platform_is_msm8956() && platform_is_msm8976_v_1_1())

			ret = clk_get_set_enable(clk_name, 186400000, 1);
		else

			ret = clk_get_set_enable(clk_name, 192000000, 1);
	}
	else if(freq == MMC_CLK_200MHZ)
	{
		ret = clk_get_set_enable(clk_name, 200000000, 1);
	}
	else if(freq == MMC_CLK_400MHZ)
	{
		if (platform_is_msm8956() && platform_is_msm8976_v_1_1())

			ret = clk_get_set_enable(clk_name, 372800000, 1);
		else

			ret = clk_get_set_enable(clk_name, 384000000, 1);
	}
	else
	{
		dprintf(CRITICAL, "sdc frequency (%u) is not supported\n", freq);
		ASSERT(0);
	}

	if(ret)
	{
		dprintf(CRITICAL, "failed to set %s ret = %d\n", clk_name, ret);
		ASSERT(0);
	}
}

/* Configure UART clock based on the UART block id*/
void clock_config_uart_dm(uint8_t id)
{
	int ret;
	char iclk[64];
	char cclk[64];

	snprintf(iclk, sizeof(iclk), "uart%u_iface_clk", id);
	snprintf(cclk, sizeof(cclk), "uart%u_core_clk", id);

	ret = clk_get_set_enable(iclk, 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set %s ret = %d\n", iclk, ret);
		ASSERT(0);
	}

	ret = clk_get_set_enable(cclk, 7372800, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set %s ret = %d\n", cclk, ret);
		ASSERT(0);
	}
}

/* Control the MDSS GDSC */
void mdp_gdsc_ctrl(uint8_t enable)
{
	uint32_t reg = 0;
	reg = readl(MDP_GDSCR);
	if (enable) {
		if (!(reg & GDSC_POWER_ON_BIT)) {
			reg &=  ~(BIT(0) | GDSC_EN_FEW_WAIT_MASK);
			reg |= GDSC_EN_FEW_WAIT_256_MASK;
			writel(reg, MDP_GDSCR);
			while(!(readl(MDP_GDSCR) & (GDSC_POWER_ON_BIT)));
		} else {
			dprintf(SPEW, "MDP GDSC already enabled\n");
		}
	} else {
		reg |= BIT(0);
		writel(reg, MDP_GDSCR);
		while(readl(MDP_GDSCR) & (GDSC_POWER_ON_BIT));
	}
}

/* Enable all the MDP branch clocks */
void mdp_clock_enable(void)
{
	int ret;

	ret = clk_get_set_enable("mdp_ahb_clk", 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set mdp_ahb_clk ret = %d\n", ret);
		ASSERT(0);
	}

	/* Set MDP clock to 320MHz */
	ret = clk_get_set_enable("mdss_mdp_clk_src", 320000000, 1);

	if(ret)
	{
		dprintf(CRITICAL, "failed to set mdp_clk_src ret = %d\n", ret);
		ASSERT(0);
	}

	ret = clk_get_set_enable("mdss_vsync_clk", 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set mdss vsync clk ret = %d\n", ret);
		ASSERT(0);
	}

	ret = clk_get_set_enable("mdss_mdp_clk", 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set mdp_clk ret = %d\n", ret);
		ASSERT(0);
	}
}

/* Disable all the MDP branch clocks */
void mdp_clock_disable(void)
{
	clk_disable(clk_get("mdss_vsync_clk"));
	clk_disable(clk_get("mdss_mdp_clk"));
	clk_disable(clk_get("mdss_mdp_clk_src"));
	clk_disable(clk_get("mdp_ahb_clk"));
}

/* Disable all the bus clocks needed by MDSS */
void mdss_bus_clocks_disable(void)
{
	/* Disable MDSS AXI clock */
	clk_disable(clk_get("mdss_axi_clk"));
}

/* Enable all the bus clocks needed by MDSS */
void mdss_bus_clocks_enable(void)
{
	int ret;

	/* Configure AXI clock */
	ret = clk_get_set_enable("mdss_axi_clk", 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set mdss_axi_clk ret = %d\n", ret);
		ASSERT(0);
	}
}

static void rcg_update_config(uint32_t reg)
{
	int i;

	for (i = 0; i < MAX_LOOPS; i++) {
		if (!(readl(reg) & BIT(0)))
			return;
		udelay(1);
	}

	dprintf(CRITICAL, "failed to update rcg config for reg = 0x%x\n", reg);
	ASSERT(0);
}

static void branch_clk_halt_check(uint32_t reg)
{
	int i;

	for (i = 0; i < MAX_LOOPS; i++) {
		if (!(readl(reg) & BIT(31)))
			return;
		udelay(1);
	}

	dprintf(CRITICAL, "failed to enable branch for reg = 0x%x\n", reg);
	ASSERT(0);
}

/* Disable all the branch clocks needed by the DSI controller */
void gcc_dsi_clocks_disable(uint32_t flags)
{
	if (flags & MMSS_DSI_CLKS_FLAG_DSI0) {
		clk_disable(clk_get("mdss_esc0_clk"));
		writel(0x0, DSI_PIXEL0_CBCR);
		writel(0x0, DSI_BYTE0_CBCR);
	}

	if (flags & MMSS_DSI_CLKS_FLAG_DSI1) {
		clk_disable(clk_get("mdss_esc1_clk"));
		writel(0x0, DSI_PIXEL1_CBCR);
		writel(0x0, DSI_BYTE1_CBCR);
	}
}

/* Configure all the branch clocks needed by the DSI controller */
void gcc_dsi_clocks_enable(uint32_t flags, bool use_dsi1_pll, uint8_t pclk0_m,
		uint8_t pclk0_n, uint8_t pclk0_d)
{
	int ret;
	int dsi0_cfg_rcgr, dsi1_cfg_rcgr = 0;

	dsi0_cfg_rcgr = BIT(8); /* DSI0 can only be sourced from PLL0 */

	/*
	 * DSI1<->PLL1 for 1.) 8956 v1.0 always 2.) Single DSI cases on DSI1
	 * DSI1<->PLL0 for 8956 v1.1 split DSI cases
	 */
	if (!platform_is_msm8976_v_1_1() || use_dsi1_pll)
		dsi1_cfg_rcgr = BIT(8);
	else if (flags == (MMSS_DSI_CLKS_FLAG_DSI0 | MMSS_DSI_CLKS_FLAG_DSI1))
		dsi1_cfg_rcgr = BIT(8) | BIT(9);

	if (flags & MMSS_DSI_CLKS_FLAG_DSI0) {
		/* Enable DSI0 branch clocks */

		/* Set the source for DSI0 byte RCG */
		writel(dsi0_cfg_rcgr, DSI_BYTE0_CFG_RCGR);
		/* Set the update RCG bit */
		writel(0x1, DSI_BYTE0_CMD_RCGR);
		rcg_update_config(DSI_BYTE0_CMD_RCGR);
		/* Enable the branch clock */
		writel(0x1, DSI_BYTE0_CBCR);
		branch_clk_halt_check(DSI_BYTE0_CBCR);

		/* Configure Pixel clock */
		/* Set the source for DSI0 pixel RCG */
		writel(dsi0_cfg_rcgr, DSI_PIXEL0_CFG_RCGR);
		/* Set the MND for DSI0 pixel clock */
		writel(pclk0_m, DSI_PIXEL0_M);
		writel(pclk0_n, DSI_PIXEL0_N);
		writel(pclk0_d, DSI_PIXEL0_D);
		/* Set the update RCG bit */
		writel(0x1, DSI_PIXEL0_CMD_RCGR);
		rcg_update_config(DSI_PIXEL0_CMD_RCGR);
		/* Enable the branch clock */
		writel(0x1, DSI_PIXEL0_CBCR);
		branch_clk_halt_check(DSI_PIXEL0_CBCR);

		/* Configure ESC clock */
		ret = clk_get_set_enable("mdss_esc0_clk", 0, 1);
		if (ret) {
			dprintf(CRITICAL, "failed to set esc0_clk ret = %d\n", ret);
			ASSERT(0);
		}
	}

	if (flags & MMSS_DSI_CLKS_FLAG_DSI1) {
		/* Enable DSI1 branch clocks */

		/* Set the source for DSI1 byte RCG */
		writel(dsi1_cfg_rcgr, DSI_BYTE1_CFG_RCGR);
		/* Set the update RCG bit */
		writel(0x1, DSI_BYTE1_CMD_RCGR);
		rcg_update_config(DSI_BYTE1_CMD_RCGR);
		/* Enable the branch clock */
		writel(0x1, DSI_BYTE1_CBCR);
		branch_clk_halt_check(DSI_BYTE1_CBCR);

		/* Configure Pixel clock */
		/* Set the source for DSI1 pixel RCG */
		writel(dsi1_cfg_rcgr, DSI_PIXEL1_CFG_RCGR);
		/* Set the MND for DSI1 pixel clock */
		writel(pclk0_m, DSI_PIXEL1_M);
		writel(pclk0_n, DSI_PIXEL1_N);
		writel(pclk0_d, DSI_PIXEL1_D);
		/* Set the update RCG bit */
		writel(0x1, DSI_PIXEL1_CMD_RCGR);
		rcg_update_config(DSI_PIXEL1_CMD_RCGR);
		/* Enable the branch clock */
		writel(0x1, DSI_PIXEL1_CBCR);
		branch_clk_halt_check(DSI_PIXEL1_CBCR);

		/* Configure ESC clock */
		ret = clk_get_set_enable("mdss_esc1_clk", 0, 1);
		if (ret) {
			dprintf(CRITICAL, "failed to set esc1_clk ret = %d\n", ret);
			ASSERT(0);
		}
	}
}

/* Function to asynchronously reset CE.
 * Function assumes that all the CE clocks are off.
 */
static void ce_async_reset(uint8_t instance)
{
	/* Start the block reset for CE */
	writel(1, GCC_CRYPTO_BCR);

	udelay(2);

	/* Take CE block out of reset */
	writel(0, GCC_CRYPTO_BCR);

	udelay(2);
}

void clock_ce_enable(uint8_t instance)
{
	int ret;
	char clk_name[64];

	snprintf(clk_name, sizeof(clk_name), "ce%u_src_clk", instance);
	ret = clk_get_set_enable(clk_name, 160000000, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set ce%u_src_clk ret = %d\n", instance, ret);
		ASSERT(0);
	}

	snprintf(clk_name, sizeof(clk_name), "ce%u_core_clk", instance);
	ret = clk_get_set_enable(clk_name, 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set ce%u_core_clk ret = %d\n", instance, ret);
		ASSERT(0);
	}

	snprintf(clk_name, sizeof(clk_name), "ce%u_ahb_clk", instance);
	ret = clk_get_set_enable(clk_name, 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set ce%u_ahb_clk ret = %d\n", instance, ret);
		ASSERT(0);
	}

	snprintf(clk_name, sizeof(clk_name), "ce%u_axi_clk", instance);
	ret = clk_get_set_enable(clk_name, 0, 1);
	if(ret)
	{
		dprintf(CRITICAL, "failed to set ce%u_axi_clk ret = %d\n", instance, ret);
		ASSERT(0);
	}

	/* Wait for 48 * #pipes cycles.
	 * This is necessary as immediately after an access control reset (boot up)
	 * or a debug re-enable, the Crypto core sequentially clears its internal
	 * pipe key storage memory. If pipe key initialization writes are attempted
	 * during this time, they may be overwritten by the internal clearing logic.
	 */
	udelay(1);

}

void clock_ce_disable(uint8_t instance)
{
	struct clk *ahb_clk;
	struct clk *cclk;
	struct clk *axi_clk;
	struct clk *src_clk;
	char clk_name[64];

	snprintf(clk_name, sizeof(clk_name), "ce%u_src_clk", instance);
	src_clk = clk_get(clk_name);

	snprintf(clk_name, sizeof(clk_name), "ce%u_ahb_clk", instance);
	ahb_clk = clk_get(clk_name);

	snprintf(clk_name, sizeof(clk_name), "ce%u_axi_clk", instance);
	axi_clk = clk_get(clk_name);

	snprintf(clk_name, sizeof(clk_name), "ce%u_core_clk", instance);
	cclk    = clk_get(clk_name);

	clk_disable(ahb_clk);
	clk_disable(axi_clk);
	clk_disable(cclk);
	clk_disable(src_clk);

	/* Some delay for the clocks to stabalize. */
	udelay(1);

}

void clock_config_ce(uint8_t instance)
{
	/* Need to enable the clock before disabling since the clk_disable()
	 * has a check to default to nop when the clk_enable() is not called
	 * on that particular clock.
	 */
	clock_ce_enable(instance);

	clock_ce_disable(instance);

	ce_async_reset(instance);

	clock_ce_enable(instance);
}
