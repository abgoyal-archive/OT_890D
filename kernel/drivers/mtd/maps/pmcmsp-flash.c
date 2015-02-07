

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>

#include <msp_prom.h>
#include <msp_regs.h>


static struct mtd_info **msp_flash;
static struct mtd_partition **msp_parts;
static struct map_info *msp_maps;
static int fcnt;

#define DEBUG_MARKER printk(KERN_NOTICE "%s[%d]\n", __func__, __LINE__)

static int __init init_msp_flash(void)
{
	int i, j;
	int offset, coff;
	char *env;
	int pcnt;
	char flash_name[] = "flash0";
	char part_name[] = "flash0_0";
	unsigned addr, size;

	/* If ELB is disabled by "ful-mux" mode, we can't get at flash */
	if ((*DEV_ID_REG & DEV_ID_SINGLE_PC) &&
	    (*ELB_1PC_EN_REG & SINGLE_PCCARD)) {
		printk(KERN_NOTICE "Single PC Card mode: no flash access\n");
		return -ENXIO;
	}

	/* examine the prom environment for flash devices */
	for (fcnt = 0; (env = prom_getenv(flash_name)); fcnt++)
		flash_name[5] = '0' + fcnt + 1;

	if (fcnt < 1)
		return -ENXIO;

	printk(KERN_NOTICE "Found %d PMC flash devices\n", fcnt);

	msp_flash = kmalloc(fcnt * sizeof(struct map_info *), GFP_KERNEL);
	msp_parts = kmalloc(fcnt * sizeof(struct mtd_partition *), GFP_KERNEL);
	msp_maps = kcalloc(fcnt, sizeof(struct mtd_info), GFP_KERNEL);
	if (!msp_flash || !msp_parts || !msp_maps) {
		kfree(msp_maps);
		kfree(msp_parts);
		kfree(msp_flash);
		return -ENOMEM;
	}

	/* loop over the flash devices, initializing each */
	for (i = 0; i < fcnt; i++) {
		/* examine the prom environment for flash partititions */
		part_name[5] = '0' + i;
		part_name[7] = '0';
		for (pcnt = 0; (env = prom_getenv(part_name)); pcnt++)
			part_name[7] = '0' + pcnt + 1;

		if (pcnt == 0) {
			printk(KERN_NOTICE "Skipping flash device %d "
				"(no partitions defined)\n", i);
			continue;
		}

		msp_parts[i] = kcalloc(pcnt, sizeof(struct mtd_partition),
				       GFP_KERNEL);

		/* now initialize the devices proper */
		flash_name[5] = '0' + i;
		env = prom_getenv(flash_name);

		if (sscanf(env, "%x:%x", &addr, &size) < 2)
			return -ENXIO;
		addr = CPHYSADDR(addr);

		printk(KERN_NOTICE
			"MSP flash device \"%s\": 0x%08x at 0x%08x\n",
			flash_name, size, addr);
		/* This must matchs the actual size of the flash chip */
		msp_maps[i].size = size;
		msp_maps[i].phys = addr;

		/*
		 * Platforms have a specific limit of the size of memory
		 * which may be mapped for flash:
		 */
		if (size > CONFIG_MSP_FLASH_MAP_LIMIT)
			size = CONFIG_MSP_FLASH_MAP_LIMIT;
		msp_maps[i].virt = ioremap(addr, size);
		msp_maps[i].bankwidth = 1;
		msp_maps[i].name = strncpy(kmalloc(7, GFP_KERNEL),
					flash_name, 7);

		if (msp_maps[i].virt == NULL)
			return -ENXIO;

		for (j = 0; j < pcnt; j++) {
			part_name[5] = '0' + i;
			part_name[7] = '0' + j;

			env = prom_getenv(part_name);

			if (sscanf(env, "%x:%x:%n", &offset, &size, &coff) < 2)
				return -ENXIO;

			msp_parts[i][j].size = size;
			msp_parts[i][j].offset = offset;
			msp_parts[i][j].name = env + coff;
		}

		/* now probe and add the device */
		simple_map_init(&msp_maps[i]);
		msp_flash[i] = do_map_probe("cfi_probe", &msp_maps[i]);
		if (msp_flash[i]) {
			msp_flash[i]->owner = THIS_MODULE;
			add_mtd_partitions(msp_flash[i], msp_parts[i], pcnt);
		} else {
			printk(KERN_ERR "map probe failed for flash\n");
			return -ENXIO;
		}
	}

	return 0;
}

static void __exit cleanup_msp_flash(void)
{
	int i;

	for (i = 0; i < sizeof(msp_flash) / sizeof(struct mtd_info **); i++) {
		del_mtd_partitions(msp_flash[i]);
		map_destroy(msp_flash[i]);
		iounmap((void *)msp_maps[i].virt);

		/* free the memory */
		kfree(msp_maps[i].name);
		kfree(msp_parts[i]);
	}

	kfree(msp_flash);
	kfree(msp_parts);
	kfree(msp_maps);
}

MODULE_AUTHOR("PMC-Sierra, Inc");
MODULE_DESCRIPTION("MTD map driver for PMC-Sierra MSP boards");
MODULE_LICENSE("GPL");

module_init(init_msp_flash);
module_exit(cleanup_msp_flash);
