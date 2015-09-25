/* Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
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
#include <platform.h>
#include <rpmb.h>
#include <rpmb_listener.h>
#include <mmc_sdhci.h>
#include <boot_device.h>
#include <debug.h>
#include <target.h>

static bool lksec_app_loaded;

static void *dev;
static int app_handle;
struct rpmb_init_info info;

int rpmb_init()
{
	int ret = 0;

	dev = target_mmc_device();

	/* 1. Initialize storage specific data */
	if (platform_boot_dev_isemmc())
	{
		struct mmc_device *mmc_dev = (struct mmc_device *) dev;
		info.size = mmc_dev->card.rpmb_size / RPMB_MIN_BLK_SZ;
		if (mmc_dev->card.ext_csd[MMC_EXT_CSD_REV] < 8)
		{
			dprintf(SPEW, "EMMC Version < 5.1\n");
			info.rel_wr_count = mmc_dev->card.rel_wr_count;
		}
		else
		{
			if (mmc_dev->card.ext_csd[MMC_EXT_CSD_EN_RPMB_REL_WR] == 0)
			{
				dprintf(SPEW, "EMMC Version >= 5.1 EN_RPMB_REL_WR = 0\n");
				// according to emmc version 5.1 and above if EN_RPMB_REL_WR in extended
				// csd is not set the maximum number of frames that can be reliably written
				// to emmc would be 2
				info.rel_wr_count = 2;
			}
			else
			{
				dprintf(SPEW, "EMMC Version >= 5.1 EN_RPMB_REL_WR = 1\n");
				// according to emmc version 5.1 and above if EN_RPMB_REL_WR in extended
				// csd is set the maximum number of frames that can be reliably written
				// to emmc would be 32
				info.rel_wr_count = 32;
			}
		}
		info.dev_type  = EMMC_RPMB;
	}
	else
	{
		struct ufs_dev *ufs_dev = (struct ufs_dev *) dev;
		ufs_rpmb_init(ufs_dev);
		info.size = ufs_dev->rpmb_num_blocks;
		info.rel_wr_count = ufs_dev->rpmb_rw_size;
		info.dev_type  = UFS_RPMB;
	}

	/* Initialize Qseecom */
	ret = qseecom_init();

	if (ret < 0)
	{
		dprintf(CRITICAL, "Failed to initialize qseecom, error: %d\n", ret);
		goto err;
	}

	/* Register & start the listener */
	ret = rpmb_listener_start();
	if (ret < 0)
	{
		dprintf(CRITICAL, "Error registering the handler\n");
		goto err;
	}

	/* Start Qseecom */
	ret = qseecom_tz_init();

	if (ret < 0)
	{
		dprintf(CRITICAL, "Failed to start qseecom, error: %d\n", ret);
		goto err;
	}

err:
	return ret;
}

struct rpmb_init_info *rpmb_get_init_info()
{
	return &info;
}

int rpmb_read(uint32_t *req, uint32_t req_len, uint32_t *resp, uint32_t *resp_len)
{
	if (platform_boot_dev_isemmc())
		return rpmb_read_emmc(dev, req, req_len, resp, resp_len);
	else
		return rpmb_read_ufs(dev, req, req_len, resp, resp_len);
}

int rpmb_write(uint32_t *req, uint32_t req_len, uint32_t rel_wr_count, uint32_t *resp, uint32_t *resp_len)
{
	if (platform_boot_dev_isemmc())
		return rpmb_write_emmc(dev, req, req_len, rel_wr_count, resp, resp_len);
	else
		return rpmb_write_ufs(dev, req, req_len, rel_wr_count, resp, resp_len);
}

/* This API calls into TZ app to read device_info */
int read_device_info_rpmb(void *info, uint32_t sz)
{
	int ret = 0;
	struct send_cmd_req read_req = {0};
	struct send_cmd_rsp read_rsp = {0};

	/*
	 * Load the sec app for first time
	 */
	if (!lksec_app_loaded)
	{
		if (load_sec_app() < 0)
		{
			dprintf(CRITICAL, "Failed to load App for rpmb\n");
			ASSERT(0);
		}
		lksec_app_loaded = true;
	}

	read_req.cmd_id = CLIENT_CMD_READ_LK_DEVICE_STATE;
	read_req.data   = (uint32_t) info;
	read_req.len    = sz;

	/* Read the device info */
	arch_clean_invalidate_cache_range((addr_t) info, sz);
	ret = qseecom_send_command(app_handle, (void*) &read_req, sizeof(read_req), (void*) &read_rsp, sizeof(read_rsp));
	arch_invalidate_cache_range((addr_t) info, sz);

	if (ret < 0 || read_rsp.status < 0)
	{
		dprintf(CRITICAL, "Reading device info failed: Error: %d\n", read_rsp.status);
		return -1;
	}

	return 0;
}

int write_device_info_rpmb(void *info, uint32_t sz)
{
	int ret = 0;

	struct send_cmd_req write_req = {0};
	struct send_cmd_rsp write_rsp = {0};

	write_req.cmd_id = CLIENT_CMD_WRITE_LK_DEVICE_STATE;
	write_req.data   = (uint32_t) info;
	write_req.len    = sz;

	/* Write the device info */
	arch_clean_invalidate_cache_range((addr_t) info, sz);
	ret = qseecom_send_command(app_handle, (void *)&write_req, sizeof(write_req), (void *)&write_rsp, sizeof(write_rsp));
	arch_invalidate_cache_range((addr_t) info, sz);

	if (ret < 0 || write_rsp.status < 0)
	{
		dprintf(CRITICAL, "Writing device info failed: Error: %d\n", write_rsp.status);
		return -1;
	}

	return 0;
}

int rpmb_get_app_handle()
{
	return app_handle;
}

int rpmb_uninit()
{
	int ret = 0;

	ret = rpmb_listener_stop(RPMB_LSTNR_ID);
	if (ret < 0)
	{
		dprintf(CRITICAL, "Failed to stop Qseecom Listener\n");
		return ret;
	}
	ret = qseecom_exit();

	if (ret < 0)
	{
		dprintf(CRITICAL, "Failed to exit qseecom \n");
		return ret;
	}

	return ret;
}

int load_sec_app()
{
	/* start TZ app */
	app_handle = qseecom_start_app("lksecapp");

	if (app_handle < 0)
	{
		dprintf(CRITICAL, "Failure to load TZ app: lksecapp, error: %d\n", app_handle);
		return app_handle;
	}

	return 0;
}

int unload_sec_app()
{
	int ret = 0;

	struct send_cmd_req req = {0};
	struct send_cmd_rsp rsp = {0};

	req.cmd_id = CLIENT_CMD_LK_END_MILESTONE;
	rsp.cmd_id = CLIENT_CMD_LK_END_MILESTONE;

	/* Milestone end command */
	ret = qseecom_send_command(app_handle, (void *)&req, sizeof(req), (void *)&rsp, sizeof(rsp));

	if (ret < 0 || rsp.status < 0)
	{
		dprintf(CRITICAL, "Failed to send milestone end command: Error: %x\n", rsp.status);
		return ret;
	}

	if (qseecom_shutdown_app(app_handle) < 0)
	{
		dprintf(CRITICAL, "Failed to Shutdown sec app\n");
		ASSERT(0);
	}


	return 0;
}

bool is_sec_app_loaded()
{
	return lksec_app_loaded;
}
