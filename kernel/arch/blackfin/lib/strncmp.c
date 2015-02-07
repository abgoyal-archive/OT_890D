

#define strncmp __inline_strncmp
#include <asm/string.h>
#undef strncmp

#include <linux/module.h>

int strncmp(const char *cs, const char *ct, size_t count)
{
	return __inline_strncmp(cs, ct, count);
}
EXPORT_SYMBOL(strncmp);
