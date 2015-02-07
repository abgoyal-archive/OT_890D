
#include <linux/module.h>
#include <asm/machvec.h>
#include <asm/io.h>

void memcpy_fromio(void *to, const volatile void __iomem *from, unsigned long count)
{
	unsigned char *p = to;
        while (count) {
                count--;
                *p = readb(from);
                p++;
                from++;
        }
}
EXPORT_SYMBOL(memcpy_fromio);

void memcpy_toio(volatile void __iomem *to, const void *from, unsigned long count)
{
	const unsigned char *p = from;
        while (count) {
                count--;
                writeb(*p, to);
                p++;
                to++;
        }
}
EXPORT_SYMBOL(memcpy_toio);

void memset_io(volatile void __iomem *dst, int c, unsigned long count)
{
        while (count) {
                count--;
                writeb(c, dst);
                dst++;
        }
}
EXPORT_SYMBOL(memset_io);

void __iomem *ioport_map(unsigned long port, unsigned int nr)
{
	void __iomem *ret;

	ret = __ioport_map_trapped(port, nr);
	if (ret)
		return ret;

	return __ioport_map(port, nr);
}
EXPORT_SYMBOL(ioport_map);

void ioport_unmap(void __iomem *addr)
{
	sh_mv.mv_ioport_unmap(addr);
}
EXPORT_SYMBOL(ioport_unmap);
