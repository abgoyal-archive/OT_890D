

#include <linux/init.h>
#include <linux/time.h>
#include <linux/ioport.h>
#include <sound/core.h>

void release_and_free_resource(struct resource *res)
{
	if (res) {
		release_resource(res);
		kfree(res);
	}
}

EXPORT_SYMBOL(release_and_free_resource);

#ifdef CONFIG_SND_VERBOSE_PRINTK
void snd_verbose_printk(const char *file, int line, const char *format, ...)
{
	va_list args;
	
	if (format[0] == '<' && format[1] >= '0' && format[1] <= '7' && format[2] == '>') {
		char tmp[] = "<0>";
		tmp[1] = format[1];
		printk("%sALSA %s:%d: ", tmp, file, line);
		format += 3;
	} else {
		printk("ALSA %s:%d: ", file, line);
	}
	va_start(args, format);
	vprintk(format, args);
	va_end(args);
}

EXPORT_SYMBOL(snd_verbose_printk);
#endif

#if defined(CONFIG_SND_DEBUG) && defined(CONFIG_SND_VERBOSE_PRINTK)
void snd_verbose_printd(const char *file, int line, const char *format, ...)
{
	va_list args;
	
	if (format[0] == '<' && format[1] >= '0' && format[1] <= '7' && format[2] == '>') {
		char tmp[] = "<0>";
		tmp[1] = format[1];
		printk("%sALSA %s:%d: ", tmp, file, line);
		format += 3;
	} else {
		printk(KERN_DEBUG "ALSA %s:%d: ", file, line);
	}
	va_start(args, format);
	vprintk(format, args);
	va_end(args);

}

EXPORT_SYMBOL(snd_verbose_printd);
#endif

#ifdef CONFIG_PCI
#include <linux/pci.h>
const struct snd_pci_quirk *
snd_pci_quirk_lookup(struct pci_dev *pci, const struct snd_pci_quirk *list)
{
	const struct snd_pci_quirk *q;

	for (q = list; q->subvendor; q++)
		if (q->subvendor == pci->subsystem_vendor &&
		    (!q->subdevice || q->subdevice == pci->subsystem_device))
			return q;
	return NULL;
}

EXPORT_SYMBOL(snd_pci_quirk_lookup);
#endif
