
#include <linux/crc32.h>
#include "check.h"
#include "efi.h"

static int force_gpt;
static int __init
force_gpt_fn(char *str)
{
	force_gpt = 1;
	return 1;
}
__setup("gpt", force_gpt_fn);


static inline u32
efi_crc32(const void *buf, unsigned long len)
{
	return (crc32(~0L, buf, len) ^ ~0L);
}

static u64
last_lba(struct block_device *bdev)
{
	if (!bdev || !bdev->bd_inode)
		return 0;
	return (bdev->bd_inode->i_size >> 9) - 1ULL;
}

static inline int
pmbr_part_valid(struct partition *part)
{
        if (part->sys_ind == EFI_PMBR_OSTYPE_EFI_GPT &&
            le32_to_cpu(part->start_sect) == 1UL)
                return 1;
        return 0;
}

static int
is_pmbr_valid(legacy_mbr *mbr)
{
	int i;
	if (!mbr || le16_to_cpu(mbr->signature) != MSDOS_MBR_SIGNATURE)
                return 0;
	for (i = 0; i < 4; i++)
		if (pmbr_part_valid(&mbr->partition_record[i]))
                        return 1;
	return 0;
}

static size_t
read_lba(struct block_device *bdev, u64 lba, u8 * buffer, size_t count)
{
	size_t totalreadcount = 0;

	if (!bdev || !buffer || lba > last_lba(bdev))
                return 0;

	while (count) {
		int copied = 512;
		Sector sect;
		unsigned char *data = read_dev_sector(bdev, lba++, &sect);
		if (!data)
			break;
		if (copied > count)
			copied = count;
		memcpy(buffer, data, copied);
		put_dev_sector(sect);
		buffer += copied;
		totalreadcount +=copied;
		count -= copied;
	}
	return totalreadcount;
}

static gpt_entry *
alloc_read_gpt_entries(struct block_device *bdev, gpt_header *gpt)
{
	size_t count;
	gpt_entry *pte;
	if (!bdev || !gpt)
		return NULL;

	count = le32_to_cpu(gpt->num_partition_entries) *
                le32_to_cpu(gpt->sizeof_partition_entry);
	if (!count)
		return NULL;
	pte = kzalloc(count, GFP_KERNEL);
	if (!pte)
		return NULL;

	if (read_lba(bdev, le64_to_cpu(gpt->partition_entry_lba),
                     (u8 *) pte,
		     count) < count) {
		kfree(pte);
                pte=NULL;
		return NULL;
	}
	return pte;
}

static gpt_header *
alloc_read_gpt_header(struct block_device *bdev, u64 lba)
{
	gpt_header *gpt;
	if (!bdev)
		return NULL;

	gpt = kzalloc(sizeof (gpt_header), GFP_KERNEL);
	if (!gpt)
		return NULL;

	if (read_lba(bdev, lba, (u8 *) gpt,
		     sizeof (gpt_header)) < sizeof (gpt_header)) {
		kfree(gpt);
                gpt=NULL;
		return NULL;
	}

	return gpt;
}

static int
is_gpt_valid(struct block_device *bdev, u64 lba,
	     gpt_header **gpt, gpt_entry **ptes)
{
	u32 crc, origcrc;
	u64 lastlba;

	if (!bdev || !gpt || !ptes)
		return 0;
	if (!(*gpt = alloc_read_gpt_header(bdev, lba)))
		return 0;

	/* Check the GUID Partition Table signature */
	if (le64_to_cpu((*gpt)->signature) != GPT_HEADER_SIGNATURE) {
		pr_debug("GUID Partition Table Header signature is wrong:"
			 "%lld != %lld\n",
			 (unsigned long long)le64_to_cpu((*gpt)->signature),
			 (unsigned long long)GPT_HEADER_SIGNATURE);
		goto fail;
	}

	/* Check the GUID Partition Table CRC */
	origcrc = le32_to_cpu((*gpt)->header_crc32);
	(*gpt)->header_crc32 = 0;
	crc = efi_crc32((const unsigned char *) (*gpt), le32_to_cpu((*gpt)->header_size));

	if (crc != origcrc) {
		pr_debug("GUID Partition Table Header CRC is wrong: %x != %x\n",
			 crc, origcrc);
		goto fail;
	}
	(*gpt)->header_crc32 = cpu_to_le32(origcrc);

	/* Check that the my_lba entry points to the LBA that contains
	 * the GUID Partition Table */
	if (le64_to_cpu((*gpt)->my_lba) != lba) {
		pr_debug("GPT my_lba incorrect: %lld != %lld\n",
			 (unsigned long long)le64_to_cpu((*gpt)->my_lba),
			 (unsigned long long)lba);
		goto fail;
	}

	/* Check the first_usable_lba and last_usable_lba are
	 * within the disk.
	 */
	lastlba = last_lba(bdev);
	if (le64_to_cpu((*gpt)->first_usable_lba) > lastlba) {
		pr_debug("GPT: first_usable_lba incorrect: %lld > %lld\n",
			 (unsigned long long)le64_to_cpu((*gpt)->first_usable_lba),
			 (unsigned long long)lastlba);
		goto fail;
	}
	if (le64_to_cpu((*gpt)->last_usable_lba) > lastlba) {
		pr_debug("GPT: last_usable_lba incorrect: %lld > %lld\n",
			 (unsigned long long)le64_to_cpu((*gpt)->last_usable_lba),
			 (unsigned long long)lastlba);
		goto fail;
	}

	if (!(*ptes = alloc_read_gpt_entries(bdev, *gpt)))
		goto fail;

	/* Check the GUID Partition Entry Array CRC */
	crc = efi_crc32((const unsigned char *) (*ptes),
			le32_to_cpu((*gpt)->num_partition_entries) *
			le32_to_cpu((*gpt)->sizeof_partition_entry));

	if (crc != le32_to_cpu((*gpt)->partition_entry_array_crc32)) {
		pr_debug("GUID Partitition Entry Array CRC check failed.\n");
		goto fail_ptes;
	}

	/* We're done, all's well */
	return 1;

 fail_ptes:
	kfree(*ptes);
	*ptes = NULL;
 fail:
	kfree(*gpt);
	*gpt = NULL;
	return 0;
}

static inline int
is_pte_valid(const gpt_entry *pte, const u64 lastlba)
{
	if ((!efi_guidcmp(pte->partition_type_guid, NULL_GUID)) ||
	    le64_to_cpu(pte->starting_lba) > lastlba         ||
	    le64_to_cpu(pte->ending_lba)   > lastlba)
		return 0;
	return 1;
}

static void
compare_gpts(gpt_header *pgpt, gpt_header *agpt, u64 lastlba)
{
	int error_found = 0;
	if (!pgpt || !agpt)
		return;
	if (le64_to_cpu(pgpt->my_lba) != le64_to_cpu(agpt->alternate_lba)) {
		printk(KERN_WARNING
		       "GPT:Primary header LBA != Alt. header alternate_lba\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->my_lba),
                       (unsigned long long)le64_to_cpu(agpt->alternate_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->alternate_lba) != le64_to_cpu(agpt->my_lba)) {
		printk(KERN_WARNING
		       "GPT:Primary header alternate_lba != Alt. header my_lba\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->alternate_lba),
                       (unsigned long long)le64_to_cpu(agpt->my_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->first_usable_lba) !=
            le64_to_cpu(agpt->first_usable_lba)) {
		printk(KERN_WARNING "GPT:first_usable_lbas don't match.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->first_usable_lba),
                       (unsigned long long)le64_to_cpu(agpt->first_usable_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->last_usable_lba) !=
            le64_to_cpu(agpt->last_usable_lba)) {
		printk(KERN_WARNING "GPT:last_usable_lbas don't match.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->last_usable_lba),
                       (unsigned long long)le64_to_cpu(agpt->last_usable_lba));
		error_found++;
	}
	if (efi_guidcmp(pgpt->disk_guid, agpt->disk_guid)) {
		printk(KERN_WARNING "GPT:disk_guids don't match.\n");
		error_found++;
	}
	if (le32_to_cpu(pgpt->num_partition_entries) !=
            le32_to_cpu(agpt->num_partition_entries)) {
		printk(KERN_WARNING "GPT:num_partition_entries don't match: "
		       "0x%x != 0x%x\n",
		       le32_to_cpu(pgpt->num_partition_entries),
		       le32_to_cpu(agpt->num_partition_entries));
		error_found++;
	}
	if (le32_to_cpu(pgpt->sizeof_partition_entry) !=
            le32_to_cpu(agpt->sizeof_partition_entry)) {
		printk(KERN_WARNING
		       "GPT:sizeof_partition_entry values don't match: "
		       "0x%x != 0x%x\n",
                       le32_to_cpu(pgpt->sizeof_partition_entry),
		       le32_to_cpu(agpt->sizeof_partition_entry));
		error_found++;
	}
	if (le32_to_cpu(pgpt->partition_entry_array_crc32) !=
            le32_to_cpu(agpt->partition_entry_array_crc32)) {
		printk(KERN_WARNING
		       "GPT:partition_entry_array_crc32 values don't match: "
		       "0x%x != 0x%x\n",
                       le32_to_cpu(pgpt->partition_entry_array_crc32),
		       le32_to_cpu(agpt->partition_entry_array_crc32));
		error_found++;
	}
	if (le64_to_cpu(pgpt->alternate_lba) != lastlba) {
		printk(KERN_WARNING
		       "GPT:Primary header thinks Alt. header is not at the end of the disk.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
			(unsigned long long)le64_to_cpu(pgpt->alternate_lba),
			(unsigned long long)lastlba);
		error_found++;
	}

	if (le64_to_cpu(agpt->my_lba) != lastlba) {
		printk(KERN_WARNING
		       "GPT:Alternate GPT header not at the end of the disk.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
			(unsigned long long)le64_to_cpu(agpt->my_lba),
			(unsigned long long)lastlba);
		error_found++;
	}

	if (error_found)
		printk(KERN_WARNING
		       "GPT: Use GNU Parted to correct GPT errors.\n");
	return;
}

static int
find_valid_gpt(struct block_device *bdev, gpt_header **gpt, gpt_entry **ptes)
{
	int good_pgpt = 0, good_agpt = 0, good_pmbr = 0;
	gpt_header *pgpt = NULL, *agpt = NULL;
	gpt_entry *pptes = NULL, *aptes = NULL;
	legacy_mbr *legacymbr;
	u64 lastlba;
	if (!bdev || !gpt || !ptes)
		return 0;

	lastlba = last_lba(bdev);
        if (!force_gpt) {
                /* This will be added to the EFI Spec. per Intel after v1.02. */
                legacymbr = kzalloc(sizeof (*legacymbr), GFP_KERNEL);
                if (legacymbr) {
                        read_lba(bdev, 0, (u8 *) legacymbr,
                                 sizeof (*legacymbr));
                        good_pmbr = is_pmbr_valid(legacymbr);
                        kfree(legacymbr);
                }
                if (!good_pmbr)
                        goto fail;
        }

	good_pgpt = is_gpt_valid(bdev, GPT_PRIMARY_PARTITION_TABLE_LBA,
				 &pgpt, &pptes);
        if (good_pgpt)
		good_agpt = is_gpt_valid(bdev,
					 le64_to_cpu(pgpt->alternate_lba),
					 &agpt, &aptes);
        if (!good_agpt && force_gpt)
                good_agpt = is_gpt_valid(bdev, lastlba,
                                         &agpt, &aptes);

        /* The obviously unsuccessful case */
        if (!good_pgpt && !good_agpt)
                goto fail;

        compare_gpts(pgpt, agpt, lastlba);

        /* The good cases */
        if (good_pgpt) {
                *gpt  = pgpt;
                *ptes = pptes;
                kfree(agpt);
                kfree(aptes);
                if (!good_agpt) {
                        printk(KERN_WARNING 
			       "Alternate GPT is invalid, "
                               "using primary GPT.\n");
                }
                return 1;
        }
        else if (good_agpt) {
                *gpt  = agpt;
                *ptes = aptes;
                kfree(pgpt);
                kfree(pptes);
                printk(KERN_WARNING 
                       "Primary GPT is invalid, using alternate GPT.\n");
                return 1;
        }

 fail:
        kfree(pgpt);
        kfree(agpt);
        kfree(pptes);
        kfree(aptes);
        *gpt = NULL;
        *ptes = NULL;
        return 0;
}

int
efi_partition(struct parsed_partitions *state, struct block_device *bdev)
{
	gpt_header *gpt = NULL;
	gpt_entry *ptes = NULL;
	u32 i;

	if (!find_valid_gpt(bdev, &gpt, &ptes) || !gpt || !ptes) {
		kfree(gpt);
		kfree(ptes);
		return 0;
	}

	pr_debug("GUID Partition Table is valid!  Yea!\n");

	for (i = 0; i < le32_to_cpu(gpt->num_partition_entries) && i < state->limit-1; i++) {
		if (!is_pte_valid(&ptes[i], last_lba(bdev)))
			continue;

		put_partition(state, i+1, le64_to_cpu(ptes[i].starting_lba),
				 (le64_to_cpu(ptes[i].ending_lba) -
                                  le64_to_cpu(ptes[i].starting_lba) +
				  1ULL));

		/* If this is a RAID volume, tell md */
		if (!efi_guidcmp(ptes[i].partition_type_guid,
				 PARTITION_LINUX_RAID_GUID))
			state->parts[i+1].flags = 1;
	}
	kfree(ptes);
	kfree(gpt);
	printk("\n");
	return 1;
}
