

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <asm/mach-types.h>

#define WINDOW_ADDR 0x00000000
#define WINDOW_SIZE 0x00800000
#define BANK_WIDTH 2

static struct mtd_info *mymtd;

struct map_info sharpsl_map = {
	.name = "sharpsl-flash",
	.size = WINDOW_SIZE,
	.bankwidth = BANK_WIDTH,
	.phys = WINDOW_ADDR
};

static struct mtd_partition sharpsl_partitions[1] = {
	{
		name:		"Boot PROM Filesystem",
	}
};

static int __init init_sharpsl(void)
{
	struct mtd_partition *parts;
	int nb_parts = 0;
	char *part_type = "static";

	printk(KERN_NOTICE "Sharp SL series flash device: %x at %x\n",
		WINDOW_SIZE, WINDOW_ADDR);
	sharpsl_map.virt = ioremap(WINDOW_ADDR, WINDOW_SIZE);
	if (!sharpsl_map.virt) {
		printk("Failed to ioremap\n");
		return -EIO;
	}

	simple_map_init(&sharpsl_map);

	mymtd = do_map_probe("map_rom", &sharpsl_map);
	if (!mymtd) {
		iounmap(sharpsl_map.virt);
		return -ENXIO;
	}

	mymtd->owner = THIS_MODULE;

	if (machine_is_corgi() || machine_is_shepherd() || machine_is_husky()
		|| machine_is_poodle()) {
		sharpsl_partitions[0].size=0x006d0000;
		sharpsl_partitions[0].offset=0x00120000;
	} else if (machine_is_tosa()) {
		sharpsl_partitions[0].size=0x006a0000;
		sharpsl_partitions[0].offset=0x00160000;
	} else if (machine_is_spitz() || machine_is_akita() || machine_is_borzoi()) {
		sharpsl_partitions[0].size=0x006b0000;
		sharpsl_partitions[0].offset=0x00140000;
	} else {
		map_destroy(mymtd);
		iounmap(sharpsl_map.virt);
		return -ENODEV;
	}

	parts = sharpsl_partitions;
	nb_parts = ARRAY_SIZE(sharpsl_partitions);

	printk(KERN_NOTICE "Using %s partition definition\n", part_type);
	add_mtd_partitions(mymtd, parts, nb_parts);

	return 0;
}

static void __exit cleanup_sharpsl(void)
{
	if (mymtd) {
		del_mtd_partitions(mymtd);
		map_destroy(mymtd);
	}
	if (sharpsl_map.virt) {
		iounmap(sharpsl_map.virt);
		sharpsl_map.virt = 0;
	}
}

module_init(init_sharpsl);
module_exit(cleanup_sharpsl);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SHARP (Original: Arnold Christensen <AKC@pel.dk>)");
MODULE_DESCRIPTION("MTD map driver for SHARP SL series");
