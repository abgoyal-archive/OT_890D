

#define strcpy __inline_strcpy
#include <asm/string.h>
#undef strcpy

#include <linux/module.h>

char *strcpy(char *dest, const char *src)
{
	return __inline_strcpy(dest, src);
}
EXPORT_SYMBOL(strcpy);
