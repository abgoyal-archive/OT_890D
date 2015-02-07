
#ifndef _LINUX_SECUREBITS_H
#define _LINUX_SECUREBITS_H 1

#define SECUREBITS_DEFAULT 0x00000000

#define SECURE_NOROOT			0
#define SECURE_NOROOT_LOCKED		1  /* make bit-0 immutable */

#define SECURE_NO_SETUID_FIXUP		2
#define SECURE_NO_SETUID_FIXUP_LOCKED	3  /* make bit-2 immutable */

#define SECURE_KEEP_CAPS		4
#define SECURE_KEEP_CAPS_LOCKED		5  /* make bit-4 immutable */

#define issecure_mask(X)	(1 << (X))
#define issecure(X)		(issecure_mask(X) & current_cred_xxx(securebits))

#define SECURE_ALL_BITS		(issecure_mask(SECURE_NOROOT) | \
				 issecure_mask(SECURE_NO_SETUID_FIXUP) | \
				 issecure_mask(SECURE_KEEP_CAPS))
#define SECURE_ALL_LOCKS	(SECURE_ALL_BITS << 1)

#endif /* !_LINUX_SECUREBITS_H */
