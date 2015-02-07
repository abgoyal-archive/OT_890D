

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include "smack.h"

struct smack_known smack_known_huh = {
	.smk_next	= NULL,
	.smk_known	= "?",
	.smk_secid	= 2,
	.smk_cipso	= NULL,
};

struct smack_known smack_known_hat = {
	.smk_next	= &smack_known_huh,
	.smk_known	= "^",
	.smk_secid	= 3,
	.smk_cipso	= NULL,
};

struct smack_known smack_known_star = {
	.smk_next	= &smack_known_hat,
	.smk_known	= "*",
	.smk_secid	= 4,
	.smk_cipso	= NULL,
};

struct smack_known smack_known_floor = {
	.smk_next	= &smack_known_star,
	.smk_known	= "_",
	.smk_secid	= 5,
	.smk_cipso	= NULL,
};

struct smack_known smack_known_invalid = {
	.smk_next	= &smack_known_floor,
	.smk_known	= "",
	.smk_secid	= 6,
	.smk_cipso	= NULL,
};

struct smack_known smack_known_web = {
	.smk_next	= &smack_known_invalid,
	.smk_known	= "@",
	.smk_secid	= 7,
	.smk_cipso	= NULL,
};

struct smack_known *smack_known = &smack_known_web;

static u32 smack_next_secid = 10;

int smk_access(char *subject_label, char *object_label, int request)
{
	u32 may = MAY_NOT;
	struct smk_list_entry *sp;
	struct smack_rule *srp;

	/*
	 * Hardcoded comparisons.
	 *
	 * A star subject can't access any object.
	 */
	if (subject_label == smack_known_star.smk_known ||
	    strcmp(subject_label, smack_known_star.smk_known) == 0)
		return -EACCES;
	/*
	 * An internet object can be accessed by any subject.
	 * Tasks cannot be assigned the internet label.
	 * An internet subject can access any object.
	 */
	if (object_label == smack_known_web.smk_known ||
	    subject_label == smack_known_web.smk_known ||
	    strcmp(object_label, smack_known_web.smk_known) == 0 ||
	    strcmp(subject_label, smack_known_web.smk_known) == 0)
		return 0;
	/*
	 * A star object can be accessed by any subject.
	 */
	if (object_label == smack_known_star.smk_known ||
	    strcmp(object_label, smack_known_star.smk_known) == 0)
		return 0;
	/*
	 * An object can be accessed in any way by a subject
	 * with the same label.
	 */
	if (subject_label == object_label ||
	    strcmp(subject_label, object_label) == 0)
		return 0;
	/*
	 * A hat subject can read any object.
	 * A floor object can be read by any subject.
	 */
	if ((request & MAY_ANYREAD) == request) {
		if (object_label == smack_known_floor.smk_known ||
		    strcmp(object_label, smack_known_floor.smk_known) == 0)
			return 0;
		if (subject_label == smack_known_hat.smk_known ||
		    strcmp(subject_label, smack_known_hat.smk_known) == 0)
			return 0;
	}
	/*
	 * Beyond here an explicit relationship is required.
	 * If the requested access is contained in the available
	 * access (e.g. read is included in readwrite) it's
	 * good.
	 */
	for (sp = smack_list; sp != NULL; sp = sp->smk_next) {
		srp = &sp->smk_rule;

		if (srp->smk_subject == subject_label ||
		    strcmp(srp->smk_subject, subject_label) == 0) {
			if (srp->smk_object == object_label ||
			    strcmp(srp->smk_object, object_label) == 0) {
				may = srp->smk_access;
				break;
			}
		}
	}
	/*
	 * This is a bit map operation.
	 */
	if ((request & may) == request)
		return 0;

	return -EACCES;
}

int smk_curacc(char *obj_label, u32 mode)
{
	int rc;

	rc = smk_access(current_security(), obj_label, mode);
	if (rc == 0)
		return 0;

	/*
	 * Return if a specific label has been designated as the
	 * only one that gets privilege and current does not
	 * have that label.
	 */
	if (smack_onlycap != NULL && smack_onlycap != current->cred->security)
		return rc;

	if (capable(CAP_MAC_OVERRIDE))
		return 0;

	return rc;
}

static DEFINE_MUTEX(smack_known_lock);

struct smack_known *smk_import_entry(const char *string, int len)
{
	struct smack_known *skp;
	char smack[SMK_LABELLEN];
	int found;
	int i;

	if (len <= 0 || len > SMK_MAXLEN)
		len = SMK_MAXLEN;

	for (i = 0, found = 0; i < SMK_LABELLEN; i++) {
		if (found)
			smack[i] = '\0';
		else if (i >= len || string[i] > '~' || string[i] <= ' ' ||
			 string[i] == '/') {
			smack[i] = '\0';
			found = 1;
		} else
			smack[i] = string[i];
	}

	if (smack[0] == '\0')
		return NULL;

	mutex_lock(&smack_known_lock);

	for (skp = smack_known; skp != NULL; skp = skp->smk_next)
		if (strncmp(skp->smk_known, smack, SMK_MAXLEN) == 0)
			break;

	if (skp == NULL) {
		skp = kzalloc(sizeof(struct smack_known), GFP_KERNEL);
		if (skp != NULL) {
			skp->smk_next = smack_known;
			strncpy(skp->smk_known, smack, SMK_MAXLEN);
			skp->smk_secid = smack_next_secid++;
			skp->smk_cipso = NULL;
			spin_lock_init(&skp->smk_cipsolock);
			/*
			 * Make sure that the entry is actually
			 * filled before putting it on the list.
			 */
			smp_mb();
			smack_known = skp;
		}
	}

	mutex_unlock(&smack_known_lock);

	return skp;
}

char *smk_import(const char *string, int len)
{
	struct smack_known *skp;

	skp = smk_import_entry(string, len);
	if (skp == NULL)
		return NULL;
	return skp->smk_known;
}

char *smack_from_secid(const u32 secid)
{
	struct smack_known *skp;

	for (skp = smack_known; skp != NULL; skp = skp->smk_next)
		if (skp->smk_secid == secid)
			return skp->smk_known;

	/*
	 * If we got this far someone asked for the translation
	 * of a secid that is not on the list.
	 */
	return smack_known_invalid.smk_known;
}

u32 smack_to_secid(const char *smack)
{
	struct smack_known *skp;

	for (skp = smack_known; skp != NULL; skp = skp->smk_next)
		if (strncmp(skp->smk_known, smack, SMK_MAXLEN) == 0)
			return skp->smk_secid;
	return 0;
}

void smack_from_cipso(u32 level, char *cp, char *result)
{
	struct smack_known *kp;
	char *final = NULL;

	for (kp = smack_known; final == NULL && kp != NULL; kp = kp->smk_next) {
		if (kp->smk_cipso == NULL)
			continue;

		spin_lock_bh(&kp->smk_cipsolock);

		if (kp->smk_cipso->smk_level == level &&
		    memcmp(kp->smk_cipso->smk_catset, cp, SMK_LABELLEN) == 0)
			final = kp->smk_known;

		spin_unlock_bh(&kp->smk_cipsolock);
	}
	if (final == NULL)
		final = smack_known_huh.smk_known;
	strncpy(result, final, SMK_MAXLEN);
	return;
}

int smack_to_cipso(const char *smack, struct smack_cipso *cp)
{
	struct smack_known *kp;

	for (kp = smack_known; kp != NULL; kp = kp->smk_next)
		if (kp->smk_known == smack ||
		    strcmp(kp->smk_known, smack) == 0)
			break;

	if (kp == NULL || kp->smk_cipso == NULL)
		return -ENOENT;

	memcpy(cp, kp->smk_cipso, sizeof(struct smack_cipso));
	return 0;
}
