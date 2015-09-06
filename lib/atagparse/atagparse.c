#include <debug.h>
#include <string.h>
#include <malloc.h>
#include <board.h>
#include <atagparse.h>
#include "atags.h"

#if DEVICE_TREE
#include <libfdt.h>
#include <dev_tree.h>
#endif

// lk boot args
extern uint32_t lk_boot_args[4];

// atags backup
static void* tags_copy = NULL;

// parsed data
static uint32_t machinetype = 0;
static char* command_line = NULL;
static uint32_t platform_id = 0;
static uint32_t variant_id = 0;
static uint32_t soc_rev = 0;

uint32_t lkargs_get_machinetype(void) {
	return machinetype;
}

const char* lkargs_get_command_line(void) {
	return command_line;
}


uint32_t lkargs_get_platform_id(void) {
	return platform_id;
}

uint32_t lkargs_get_variant_id(void) {
	return variant_id;
}

uint32_t lkargs_get_soc_rev(void) {
	return soc_rev;
}

// backup functions
static int save_atags(const struct tag *tags)
{
	const struct tag *t = tags;
	for (; t->hdr.size; t = tag_next(t));
	t++;
	uint32_t tags_size = ((uint32_t)t)-((uint32_t)tags);

	tags_copy = malloc(tags_size);
	if(!tags_copy) {
		dprintf(CRITICAL, "Error saving atags!\n");
		return -1;
	}

	memcpy(tags_copy, tags, tags_size);
	return 0;
}

#if DEVICE_TREE
static int save_fdt(void* fdt)
{
	uint32_t tags_size = fdt_totalsize(fdt);
	tags_copy = malloc(tags_size);
	if(!tags_copy) {
		dprintf(CRITICAL, "Error saving fdt!\n");
		return -1;
	}

	memcpy(tags_copy, fdt, tags_size);
	return 0;
}
#endif

// parse ATAGS
static int parse_atag_core(const struct tag *tag)
{
	return 0;
}

static int parse_atag_mem32(const struct tag *tag)
{
	dprintf(INFO, "0x%08x-0x%08x\n", tag->u.mem.start, tag->u.mem.start+tag->u.mem.size);
	return 0;
}

static int parse_atag_cmdline(const struct tag *tag)
{
	command_line = malloc(COMMAND_LINE_SIZE+1);
	if(!tags_copy) {
		dprintf(CRITICAL, "Error allocating cmdline memory!\n");
		return -1;
	}

	strlcpy(command_line, tag->u.cmdline.cmdline, COMMAND_LINE_SIZE);

	return 0;
}

static struct tagtable tagtable[] = {
	{ATAG_CORE, parse_atag_core},
	{ATAG_MEM, parse_atag_mem32},
	{ATAG_CMDLINE, parse_atag_cmdline},
};

static int parse_atag(const struct tag *tag)
{
	struct tagtable *t;
	struct tagtable *t_end = tagtable+ARRAY_SIZE(tagtable);

	for (t = tagtable; t < t_end; t++)
		if (tag->hdr.tag == t->tag) {
			t->parse(tag);
			break;
		}

	return t < t_end;
}

static void parse_atags(const struct tag *t)
{
	for (; t->hdr.size; t = tag_next(t))
		if (!parse_atag(t))
			dprintf(INFO, "Ignoring unrecognised tag 0x%08x\n",
				t->hdr.tag);
}

// parse FDT
#if DEVICE_TREE
struct dt_entry_v1
{
	uint32_t platform_id;
	uint32_t variant_id;
	uint32_t soc_rev;
	uint32_t offset;
	uint32_t size;
};

static struct dt_entry* get_dt_entry(void *dtb, uint32_t dtb_size)
{
	int root_offset;
	const void *prop = NULL;
	const char *plat_prop = NULL;
	const char *board_prop = NULL;
	const char *pmic_prop = NULL;
	char *model = NULL;
	struct dt_entry *cur_dt_entry = NULL;
	struct dt_entry *dt_entry_array = NULL;
	struct board_id *board_data = NULL;
	struct plat_id *platform_data = NULL;
	struct pmic_id *pmic_data = NULL;
	int len;
	int len_board_id;
	int len_plat_id;
	int min_plat_id_len = 0;
	int len_pmic_id;
	uint32_t dtb_ver;
	uint32_t num_entries = 0;
	uint32_t i, j, k, n;
	uint32_t msm_data_count;
	uint32_t board_data_count;
	uint32_t pmic_data_count;

	root_offset = fdt_path_offset(dtb, "/");
	if (root_offset < 0)
		return NULL;

	prop = fdt_getprop(dtb, root_offset, "model", &len);
	if (prop && len > 0) {
		model = (char *) malloc(sizeof(char) * len);
		ASSERT(model);
		strlcpy(model, prop, len);
	} else {
		dprintf(INFO, "model does not exist in device tree\n");
	}
	/* Find the pmic-id prop from DTB , if pmic-id is present then
	* the DTB is version 3, otherwise find the board-id prop from DTB ,
	* if board-id is present then the DTB is version 2 */
	pmic_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,pmic-id", &len_pmic_id);
	board_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,board-id", &len_board_id);
	if (pmic_prop && (len_pmic_id > 0) && board_prop && (len_board_id > 0)) {
		if ((len_pmic_id % PMIC_ID_SIZE) || (len_board_id % BOARD_ID_SIZE))
		{
			dprintf(CRITICAL, "qcom,pmic-id(%d) or qcom,board-id(%d) in device tree is not a multiple of (%d %d)\n",
				len_pmic_id, len_board_id, PMIC_ID_SIZE, BOARD_ID_SIZE);
			return NULL;
		}
		dtb_ver = DEV_TREE_VERSION_V3;
		min_plat_id_len = PLAT_ID_SIZE;
	} else if (board_prop && len_board_id > 0) {
		if (len_board_id % BOARD_ID_SIZE)
		{
			dprintf(CRITICAL, "qcom,board-id in device tree is (%d) not a multiple of (%d)\n",
				len_board_id, BOARD_ID_SIZE);
			return NULL;
		}
		dtb_ver = DEV_TREE_VERSION_V2;
		min_plat_id_len = PLAT_ID_SIZE;
	} else {
		dtb_ver = DEV_TREE_VERSION_V1;
		min_plat_id_len = DT_ENTRY_V1_SIZE;
	}

	/* Get the msm-id prop from DTB */
	plat_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,msm-id", &len_plat_id);
	if (!plat_prop || len_plat_id <= 0) {
		dprintf(INFO, "qcom,msm-id entry not found\n");
		return NULL;
	} else if (len_plat_id % min_plat_id_len) {
		dprintf(INFO, "qcom,msm-id in device tree is (%d) not a multiple of (%d)\n",
			len_plat_id, min_plat_id_len);
		return NULL;
	}

	/*
	 * If DTB version is '1' look for <x y z> pair in the DTB
	 * x: platform_id
	 * y: variant_id
	 * z: SOC rev
	 */
	if (dtb_ver == DEV_TREE_VERSION_V1) {
		cur_dt_entry = (struct dt_entry *)
				malloc(sizeof(struct dt_entry));

		if (!cur_dt_entry) {
			dprintf(CRITICAL, "Out of memory\n");
			return NULL;
		}
		memset(cur_dt_entry, 0, sizeof(struct dt_entry));

		if(len_plat_id>DT_ENTRY_V1_SIZE) {
			dprintf(INFO, "Found multiple dtentries!\n");
		}

		{
			cur_dt_entry->platform_id = fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->platform_id);
			cur_dt_entry->variant_id = fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->variant_id);
			cur_dt_entry->soc_rev = fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->soc_rev);
			cur_dt_entry->board_hw_subtype =
				fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->variant_id) >> 0x18;
			cur_dt_entry->pmic_rev[0] = board_pmic_target(0);
			cur_dt_entry->pmic_rev[1] = board_pmic_target(1);
			cur_dt_entry->pmic_rev[2] = board_pmic_target(2);
			cur_dt_entry->pmic_rev[3] = board_pmic_target(3);
			cur_dt_entry->offset = (uint32_t)dtb;
			cur_dt_entry->size = dtb_size;

			dprintf(SPEW, "Found an appended flattened device tree (%s - %u %u 0x%x)\n",
				*model ? model : "unknown",
				cur_dt_entry->platform_id, cur_dt_entry->variant_id, cur_dt_entry->soc_rev);

		}
	}
	/*
	 * If DTB Version is '3' then we have split DTB with board & msm data & pmic
	 * populated saperately in board-id & msm-id & pmic-id prop respectively.
	 * Extract the data & prepare a look up table
	 */
	else if (dtb_ver == DEV_TREE_VERSION_V2 || dtb_ver == DEV_TREE_VERSION_V3) {
		board_data_count = (len_board_id / BOARD_ID_SIZE);
		msm_data_count = (len_plat_id / PLAT_ID_SIZE);
		/* If dtb version is v2.0, the pmic_data_count will be <= 0 */
		pmic_data_count = (len_pmic_id / PMIC_ID_SIZE);

		/* If we are using dtb v3.0, then we have split board, msm & pmic data in the DTB
		*  If we are using dtb v2.0, then we have split board & msmdata in the DTB
		*/
		board_data = (struct board_id *) malloc(sizeof(struct board_id) * (len_board_id / BOARD_ID_SIZE));
		ASSERT(board_data);
		platform_data = (struct plat_id *) malloc(sizeof(struct plat_id) * (len_plat_id / PLAT_ID_SIZE));
		ASSERT(platform_data);
		if (dtb_ver == DEV_TREE_VERSION_V3) {
			pmic_data = (struct pmic_id *) malloc(sizeof(struct pmic_id) * (len_pmic_id / PMIC_ID_SIZE));
			ASSERT(pmic_data);
		}
		i = 0;

		/* Extract board data from DTB */
		for(i = 0 ; i < board_data_count; i++) {
			board_data[i].variant_id = fdt32_to_cpu(((struct board_id *)board_prop)->variant_id);
			board_data[i].platform_subtype = fdt32_to_cpu(((struct board_id *)board_prop)->platform_subtype);
			/* For V2/V3 version of DTBs we have platform version field as part
			 * of variant ID, in such case the subtype will be mentioned as 0x0
			 * As the qcom, board-id = <0xSSPMPmPH, 0x0>
			 * SS -- Subtype
			 * PM -- Platform major version
			 * Pm -- Platform minor version
			 * PH -- Platform hardware CDP/MTP
			 * In such case to make it compatible with LK algorithm move the subtype
			 * from variant_id to subtype field
			 */
			if (board_data[i].platform_subtype == 0)
				board_data[i].platform_subtype =
					fdt32_to_cpu(((struct board_id *)board_prop)->variant_id) >> 0x18;

			len_board_id -= sizeof(struct board_id);
			board_prop += sizeof(struct board_id);
		}

		/* Extract platform data from DTB */
		for(i = 0 ; i < msm_data_count; i++) {
			platform_data[i].platform_id = fdt32_to_cpu(((struct plat_id *)plat_prop)->platform_id);
			platform_data[i].soc_rev = fdt32_to_cpu(((struct plat_id *)plat_prop)->soc_rev);
			len_plat_id -= sizeof(struct plat_id);
			plat_prop += sizeof(struct plat_id);
		}

		if (dtb_ver == DEV_TREE_VERSION_V3 && pmic_prop) {
			/* Extract pmic data from DTB */
			for(i = 0 ; i < pmic_data_count; i++) {
				pmic_data[i].pmic_version[0]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[0]);
				pmic_data[i].pmic_version[1]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[1]);
				pmic_data[i].pmic_version[2]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[2]);
				pmic_data[i].pmic_version[3]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[3]);
				len_pmic_id -= sizeof(struct pmic_id);
				pmic_prop += sizeof(struct pmic_id);
			}

			/* We need to merge board & platform data into dt entry structure */
			num_entries = msm_data_count * board_data_count * pmic_data_count;
		} else {
			/* We need to merge board & platform data into dt entry structure */
			num_entries = msm_data_count * board_data_count;
		}

		if ((((uint64_t)msm_data_count * (uint64_t)board_data_count * (uint64_t)pmic_data_count) !=
			msm_data_count * board_data_count * pmic_data_count) ||
			(((uint64_t)msm_data_count * (uint64_t)board_data_count) != msm_data_count * board_data_count)) {

			free(board_data);
			free(platform_data);
			if (pmic_data)
				free(pmic_data);
			if (model)
				free(model);
			return NULL;
		}

		dt_entry_array = (struct dt_entry*) malloc(sizeof(struct dt_entry) * num_entries);
		ASSERT(dt_entry_array);

		/* If we have '<X>; <Y>; <Z>' as platform data & '<A>; <B>; <C>' as board data.
		 * Then dt entry should look like
		 * <X ,A >;<X, B>;<X, C>;
		 * <Y ,A >;<Y, B>;<Y, C>;
		 * <Z ,A >;<Z, B>;<Z, C>;
		 */
		i = 0;
		k = 0;
		n = 0;
		for (i = 0; i < msm_data_count; i++) {
			for (j = 0; j < board_data_count; j++) {
				if (dtb_ver == DEV_TREE_VERSION_V3 && pmic_prop) {
					for (n = 0; n < pmic_data_count; n++) {
						dt_entry_array[k].platform_id = platform_data[i].platform_id;
						dt_entry_array[k].soc_rev = platform_data[i].soc_rev;
						dt_entry_array[k].variant_id = board_data[j].variant_id;
						dt_entry_array[k].board_hw_subtype = board_data[j].platform_subtype;
						dt_entry_array[k].pmic_rev[0]= pmic_data[n].pmic_version[0];
						dt_entry_array[k].pmic_rev[1]= pmic_data[n].pmic_version[1];
						dt_entry_array[k].pmic_rev[2]= pmic_data[n].pmic_version[2];
						dt_entry_array[k].pmic_rev[3]= pmic_data[n].pmic_version[3];
						dt_entry_array[k].offset = (uint32_t)dtb;
						dt_entry_array[k].size = dtb_size;
						k++;
					}

				} else {
					dt_entry_array[k].platform_id = platform_data[i].platform_id;
					dt_entry_array[k].soc_rev = platform_data[i].soc_rev;
					dt_entry_array[k].variant_id = board_data[j].variant_id;
					dt_entry_array[k].board_hw_subtype = board_data[j].platform_subtype;
					dt_entry_array[k].pmic_rev[0]= board_pmic_target(0);
					dt_entry_array[k].pmic_rev[1]= board_pmic_target(1);
					dt_entry_array[k].pmic_rev[2]= board_pmic_target(2);
					dt_entry_array[k].pmic_rev[3]= board_pmic_target(3);
					dt_entry_array[k].offset = (uint32_t)dtb;
					dt_entry_array[k].size = dtb_size;
					k++;
				}
			}
		}

		if (num_entries>0) {
			if(num_entries>1) {
				dprintf(INFO, "Found multiple dtentries!\n");
			}

			dprintf(SPEW, "Found an appended flattened device tree (%s - %u %u %u 0x%x)\n",
				*model ? model : "unknown",
				dt_entry_array[i].platform_id, dt_entry_array[i].variant_id, dt_entry_array[i].board_hw_subtype, dt_entry_array[i].soc_rev);

			// allocate dt entry
			cur_dt_entry = (struct dt_entry *) malloc(sizeof(struct dt_entry));
			if (!cur_dt_entry) {
				dprintf(CRITICAL, "Out of memory\n");
				return NULL;
			}

			// copy entry
			memcpy(cur_dt_entry, &dt_entry_array[0], sizeof(struct dt_entry));
		}

		free(board_data);
		free(platform_data);
		if (pmic_data)
			free(pmic_data);
		free(dt_entry_array);
	}
	if (model)
		free(model);
	return cur_dt_entry;
}

static uint32_t dt_mem_next_cell(const uint32_t **cellp)
{
	const uint32_t *p = *cellp;

	(*cellp)++;
	return fdt32_to_cpu(*p);
}

static int parse_fdt(void* fdt)
{
	int ret = 0;
	uint32_t offset;
	int len;

	// get memory node
	ret = fdt_path_offset(fdt, "/memory");
	if (ret < 0)
	{
		dprintf(CRITICAL, "Could not find memory node.\n");
	}
	else {
		offset = ret;

		// get reg node
		const uint32_t* reg = fdt_getprop(fdt, offset, "reg", &len);
		const uint32_t* reg_end = (uint32_t*)((uint8_t*)reg)+len;
		if(!reg) {
			dprintf(CRITICAL, "Could not find reg node.\n");
		}
		else {
			while(reg<reg_end-sizeof(uint32_t)*3) {
				uint32_t base = dt_mem_next_cell(&reg);
				uint32_t size = dt_mem_next_cell(&reg);

				dprintf(INFO, "0x%08x-0x%08x\n", base, base+size);
			}
		}
	}

	// get chosen node
	ret = fdt_path_offset(fdt, "/chosen");
	if (ret < 0)
	{
		dprintf(CRITICAL, "Could not find chosen node.\n");
		return ret;
	}
	else {
		offset = ret;

		// get bootargs
		const char* bootargs = (const char *)fdt_getprop(fdt, offset, "bootargs", &len);
		if (bootargs && len>0)
		{
			command_line = malloc(len);
			memcpy(command_line, bootargs, len);
		}
	}

	// get dt entry
	struct dt_entry* dt_entry = get_dt_entry(fdt, fdt_totalsize(fdt));
	if(!dt_entry) {
		dprintf(CRITICAL, "Could not find dt entry.\n");
	}
	else {
		platform_id = dt_entry->platform_id;
		variant_id = dt_entry->variant_id;
		soc_rev = dt_entry->soc_rev;

		dprintf(INFO, "platform_id=%d variant_id=%d soc_rev=%X\n", platform_id, variant_id, soc_rev);
	}

	return 0;
}
#endif

void atag_parse(void) {
	dprintf(INFO, "bootargs: 0x%x 0x%x 0x%x 0x%x\n",
		lk_boot_args[0],
		lk_boot_args[1],
		lk_boot_args[2],
		lk_boot_args[3]
	);

	// machine type
	machinetype = lk_boot_args[1];
	dprintf(INFO, "machinetype: %u\n", machinetype);

	void* tags = (void*)lk_boot_args[2];
	struct tag *atags = (struct tag *)tags;

#if DEVICE_TREE
	// fdt
	if(!fdt_check_header(tags)) {
		save_fdt(tags);
		parse_fdt(tags);
	} else
#endif

	// atags
	if (atags->hdr.tag == ATAG_CORE) {
		save_atags(atags);
		parse_atags(atags);
	}

	// unknown
	else {
		dprintf(CRITICAL, "Invalid atags!\n");
		return;
	}

	dprintf(INFO, "cmdline=[%s]\n", command_line);
}
