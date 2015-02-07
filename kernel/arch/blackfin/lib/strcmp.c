

#define strcmp __inline_strcmp
#include <asm/string.h>
#undef strcmp

#include <linux/module.h>

int strcmp(const char *dest, const char *src)
{
	return __inline_strcmp(dest, src);
}
EXPORT_SYMBOL(strcmp);
