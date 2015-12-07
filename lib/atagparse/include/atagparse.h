#ifndef ATAGPARSE_H
#define ATAGPARSE_H

typedef enum {
	LKARGS_UEFI_BM_NORMAL = 0,
	LKARGS_UEFI_BM_RECOVERY,
} lkargs_uefi_bootmode;

uint32_t lkargs_get_machinetype(void);
const char* lkargs_get_command_line(void);
uint32_t lkargs_get_platform_id(void);
uint32_t lkargs_get_variant_id(void);
uint32_t lkargs_get_soc_rev(void);
lkargs_uefi_bootmode lkargs_get_uefi_bootmode(void);
bool lkargs_has_board_info(void);
void atag_parse(void);

unsigned *lkargs_gen_meminfo_atags(unsigned *ptr);
uint32_t lkargs_gen_meminfo_fdt(void *fdt, uint32_t memory_node_offset);

#endif // ATAGPARSE_H
