
#ifndef _HERMES_DLD_H
#define _HERMES_DLD_H

#include "hermes.h"

int hermesi_program_init(hermes_t *hw, u32 offset);
int hermesi_program_end(hermes_t *hw);
int hermes_program(hermes_t *hw, const char *first_block, const char *end);

int hermes_read_pda(hermes_t *hw,
		    __le16 *pda,
		    u32 pda_addr,
		    u16 pda_len,
		    int use_eeprom);
int hermes_apply_pda(hermes_t *hw,
		     const char *first_pdr,
		     const __le16 *pda);
int hermes_apply_pda_with_defaults(hermes_t *hw,
				   const char *first_pdr,
				   const __le16 *pda);

size_t hermes_blocks_length(const char *first_block);

#endif /* _HERMES_DLD_H */
