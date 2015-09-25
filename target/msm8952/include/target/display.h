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
 *
 */
#ifndef _TARGET_MSM8952_DISPLAY_H
#define _TARGET_MSM8952_DISPLAY_H

/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#include <display_resource.h>

/*---------------------------------------------------------------------------*/
/* Target Physical configuration                                             */
/*---------------------------------------------------------------------------*/

static const uint32_t panel_strength_ctrl[] = {
  0xff, 0x06
};

static const char panel_bist_ctrl[] = {
  0x00, 0x00, 0xb1, 0xff, 0x00, 0x00
};

extern uint32_t panel_regulator_settings[7];

static const uint32_t dcdc_regulator_settings_lpm[] = {
  0x03, 0x08, 0x07, 0x00, 0x20, 0x07, 0x01
};

static const uint32_t ldo_regulator_settings_lpm[] = {
  0x00, 0x01, 0x01, 0x00, 0x20, 0x07, 0x00
};

static const uint32_t dcdc_regulator_settings_hpm[] = {
  0x03, 0x09, 0x03, 0x00, 0x20, 0x07, 0x01
};

static const uint32_t ldo_regulator_settings_hpm[] = {
  0x00, 0x01, 0x01, 0x00, 0x20, 0x07, 0x00
};

static const char panel_lane_config[] = {
  0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x97,
  0x01, 0xc0, 0x00, 0x00, 0x05, 0x00, 0x00, 0x01, 0x97,
  0x01, 0xc0, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x01, 0x97,
  0x01, 0xc0, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x01, 0x97,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff
};

static const uint32_t panel_physical_ctrl[] = {
  0x5f, 0x00, 0x00, 0x10
};

/*---------------------------------------------------------------------------*/
/* Other Configuration                                                       */
/*---------------------------------------------------------------------------*/
#define DISPLAY_CMDLINE_PREFIX " mdss_mdp.panel="

#define MIPI_FB_ADDR 0x83200000

#define MIPI_HSYNC_PULSE_WIDTH       12
#define MIPI_HSYNC_BACK_PORCH_DCLK   32
#define MIPI_HSYNC_FRONT_PORCH_DCLK  144

#define MIPI_VSYNC_PULSE_WIDTH       4
#define MIPI_VSYNC_BACK_PORCH_LINES  3
#define MIPI_VSYNC_FRONT_PORCH_LINES 9

/*---------------------------------------------------------------------------*/
/* Functions		                                                     */
/*---------------------------------------------------------------------------*/
int target_display_pre_on();
int target_display_pre_off();
int target_display_post_on();
int target_display_post_off();
int target_cont_splash_screen();
int target_display_get_base_offset(uint32_t base);
void target_force_cont_splash_disable(uint8_t override);
uint8_t target_panel_auto_detect_enabled();
uint32_t oem_panel_max_auto_detect_panels();

#endif
