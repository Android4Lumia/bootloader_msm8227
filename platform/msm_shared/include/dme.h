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

#ifndef _DME_H_
#define _DME_H_

#include <sys/types.h>
#include <upiu.h>

/* Bit field of UFSHCI_UICCMDR register */
#define UICCMDR_DME_GET                                  0x01
#define UICCMDR_DME_SET                                  0x02
#define UICCMDR_DME_PEER_GET                             0x03
#define UICCMDR_DME_PEER_SET                             0x04
#define UICCMDR_DME_POWERON                              0x10
#define UICCMDR_DME_POWEROFF                             0x11
#define UICCMDR_DME_ENABLE                               0x12
#define UICCMDR_DME_RESET                                0x14
#define UICCMDR_DME_ENDPOINTRESET                        0x15
#define UICCMDR_DME_LINKSTARTUP                          0x16
#define UICCMDR_DME_HIBERNATE_ENTER                      0x17
#define UICCMDR_DME_HIBERNATE_EXIT                       0x18
#define UICCMDR_DME_TEST_MODE                            0x1a

/* Retry value for commands. */
#define DME_NOP_NUM_RETRIES                              20
#define DME_FDEVICEINIT_RETRIES                          20
#define DME_FPOWERONWPEN_RETRIES                         20
#define DME_BPURGESTATUS_RETRIES                         500000

/* Timeout value for commands. */
#define DME_NOP_QUERY_TIMEOUT                            10
#define DME_LINK_START_TIMEOUT                           1

/* UFS descriptor type ident value */
enum utp_ufs_desc_type
{
	UFS_DESC_IDN_DEVICE         = 0x00,
	UFS_DESC_IDN_CONFIGURATION  = 0x01,
	UFS_DESC_IDN_UNIT           = 0x02,
	UFS_DESC_IDN_INTERCONNECT   = 0x04,
	UFS_DESC_IDN_STRING         = 0x05,
	UFS_DESC_IDN_GEOMETRY       = 0x07,
	UFS_DESC_IDN_POWER          = 0x08,
};

// Attributes definitions
#define UFS_IDX_bBootLunEn          0x00
#define UFS_IDX_bCurrentPowerMode   0x01
#define UFS_IDX_bActiveICCLevel     0x03
#define UFS_IDX_bBootLunID          0x04
#define UFS_IDX_bPurgeStatus        0x06
#define UFS_IDX_bRefClkFreq         0x0a
#define UFS_IDX_bConfigDescrLock    0x0b

enum utp_query_req_upiu_func_type
{
	UPIU_QUERY_FUNC_STD_READ_REQ  = 0x01,
	UPIU_QUERY_FUNC_STD_WRITE_REQ = 0x81,
};

/* Flags definitions */
#define UFS_IDX_fDeviceInit         0x01
#define UFS_IDX_fPowerOnWPEn        0x03
#define UFS_IDX_fPurgeEn            0x06

enum utp_query_response_upiu_type
{
	UPIU_QUERY_RESP_SUCCESS               = 0x00,
	UPIU_QUERY_RESP_PARAM_NOT_READABLE    = 0xF6,
	UPIU_QUERY_RESP_PARAM_NOT_WRITABLE    = 0xF7,
	UPIU_QUERY_RESP_PARAM_ALREADY_WRITTEN = 0xF8,
	UPIU_QUERY_RESP_INVALID_LEN           = 0xF9,
	UPIU_QUERY_RESP_INVALID_VALUE         = 0xFA,
	UPIU_QUERY_RESP_INVALID_SELECTOR      = 0xFB,
	UPIU_QUERY_RESP_INVALID_IDN           = 0xFC,
	UPIU_QUERY_RESP_INVALID_OPCODE        = 0xFD,
	UPIU_QUERY_RESP_GEN_FAILURE           = 0xFE,
};

struct dme_get_req_type
{
	uint16_t attribute;
	uint16_t index;
	uint32_t *mibval;
};

#define DEV_DESC_LEN                   0x1F

struct utp_query_req_upiu_type
{
	enum upiu_query_opcode_type opcode;
	uint8_t idn;
	uint8_t index;
	uint8_t selector;
	addr_t  buf;
	size_t  buf_len;
};

struct ufs_dev_desc
{
	uint8_t  desc_len;
	uint8_t  desc_type;
	uint8_t  dev_type;
	uint8_t  dev_class;
	uint8_t  dev_sub_class;
	uint8_t  protocol;
	uint8_t  num_lu;
	uint8_t  num_wlun;
	uint8_t  boot_en;
	uint8_t  desc_access_en;
	uint8_t  init_pwr_mode;
	uint8_t  high_pior_lun;
	uint8_t  sec_removal_type;
	uint8_t  sec_lun;
	uint8_t  resv_0;
	uint8_t  active_icc_level;
	uint16_t spec_ver;
	uint16_t manufacture_date;
	uint8_t  manufacture_name;
	uint8_t  product_name;
	uint8_t  serial_num;
	uint8_t  oem_id;
	uint16_t manufacture_id;
	uint8_t  ud0_base_offset;
	uint8_t  ud_config_p_len;
	uint8_t  dev_rtt_cap;
}__PACKED;

struct ufs_geometry_desc
{
	uint8_t   desc_len;
	uint8_t   desc_type;
	uint8_t   media_technology;
	uint8_t   resv1;
	uint64_t  total_raw_device_capacity;
	uint8_t   resv2;
	uint32_t  segment_size;
	uint8_t   allocation_unit_size;
	uint8_t   min_addr_block_size;
	uint8_t   optimal_read_block_size;
	uint8_t   optimal_write_block_size;
	uint8_t   max_in_buffer_size;
	uint8_t   max_out_buffer_zie;
	uint8_t   rpmb_read_write_size;
	uint8_t   resv3;
	uint8_t   data_ordering;
	uint8_t   max_context_id_number;
	uint8_t   sys_data_tag_unit_size;
	uint8_t   sys_data_tag_res_size;
	uint8_t   supported_sec_r_types;
	uint16_t  supported_memory_types;
	uint32_t  system_code_max_n_alloc_u;
	uint16_t  system_code_cap_adj_fac;
	uint32_t  non_persist_max_n_alloc_u;
	uint16_t  non_persist_cap_adj_fac;
	uint32_t  enhanced_1_max_n_alloc_u;
	uint16_t  enhanced_1_cap_adj_fac;
	uint32_t  enhanced_2_max_n_alloc_u;
	uint16_t  enhanced_2_cap_adj_fac;
	uint32_t  enhanced_3_max_n_alloc_u;
	uint16_t  enhanced_3_cap_adj_fac;
	uint32_t  enhanced_4_max_n_alloc_u;
	uint16_t  enhanced_4_cap_adj_fac;
}__PACKED;

struct ufs_dev_desc_config_params
{
	uint8_t  num_lu;
	uint8_t  boot_enable;
	uint8_t  desc_access_en;
	uint8_t  init_pwr_mode;
	uint8_t  high_pior_lun;
	uint8_t  sec_removal_type;
	uint8_t  active_icc_level;
	uint16_t dev_rtt_cap;
	uint8_t  resv[5];
}__PACKED;

struct ufs_unit_desc_config_params
{
	uint8_t lu_enable;
	uint8_t boot_lun_id;
	uint8_t lu_wp;
	uint8_t mem_type;
	uint32_t num_alloc_units;
	uint8_t data_reliability;
	uint8_t logical_blk_size;
	uint8_t provisioning_type;
	uint16_t ctx_capabilities;
	uint8_t resv[3];
}__PACKED;

struct ufs_config_desc
{
	uint8_t                            desc_len;
	uint8_t                            desc_type;
	struct ufs_dev_desc_config_params  config_params;
	struct ufs_unit_desc_config_params unit_params[8];
}__PACKED;

struct ufs_string_desc
{
	uint8_t desc_len;
	uint8_t desc_type;
	uint8_t serial_num[128];
}__PACKED;

int dme_send_linkstartup_req(struct ufs_dev *dev);
int dme_get_req(struct ufs_dev *dev, struct dme_get_req_type *req);
int utp_build_query_req_upiu(struct upiu_trans_mgmt_query_hdr *req_upiu,
								  struct upiu_req_build_type *upiu_data);

/* Nop Query is used to check the connection to the device
 */
int dme_send_nop_query(struct ufs_dev *dev);

/* Device Init is used to set the fDeviceInit flag to initiate device
 * initialization
 */
int dme_set_fdeviceinit(struct ufs_dev *dev);

/* Power On WP Enable is used to enable Power on WP for all LUN's
 * that have WP bit enabled.
 */
int dme_set_fpoweronwpen(struct ufs_dev *dev);

/* Purge is used to securely erase blocks that are unmapped in ufs */
int dme_set_fpurgeenable(struct ufs_dev *dev);

/* Unit descriptor gives the characteristics and capabilities of
 * logical units.
 */
int dme_read_unit_desc(struct ufs_dev *dev, uint8_t index);

/* Device descriptor is the main descriptor and it provides the device
 * class, sub-class, the protocol to access the device and the maximum
 * number oflogical units in the device.
 */
int dme_read_device_desc(struct ufs_dev *dev);

/* Geometry Descriptor contains RPMB read write size which indicates total
   number of rpmb frames allowed in a single SCSI security command.
*/
int dme_read_geo_desc(struct ufs_dev *dev);

/* function to return the boot lun currently booting from */
int dme_get_bbootlunen(struct ufs_dev *dev);

/* function to set the boot lun to either 0, 1 or 2 */
int dme_set_bbootlunen(struct ufs_dev *dev, uint32_t val);
#endif
