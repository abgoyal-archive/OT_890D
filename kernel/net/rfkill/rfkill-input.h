


#ifndef __RFKILL_INPUT_H
#define __RFKILL_INPUT_H

void rfkill_switch_all(enum rfkill_type type, enum rfkill_state state);
void rfkill_epo(void);
void rfkill_restore_states(void);
void rfkill_remove_epo_lock(void);
bool rfkill_is_epo_lock_active(void);
enum rfkill_state rfkill_get_global_state(const enum rfkill_type type);

#endif /* __RFKILL_INPUT_H */
