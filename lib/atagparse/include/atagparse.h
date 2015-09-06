#ifndef ATAGPARSE_H
#define ATAGPARSE_H

uint32_t lkargs_get_machinetype(void);
const char* lkargs_get_command_line(void);
uint32_t lkargs_get_platform_id(void);
uint32_t lkargs_get_variant_id(void);
uint32_t lkargs_get_soc_rev(void);
bool lkargs_has_board_info(void);
void atag_parse(void);

#endif // ATAGPARSE_H
