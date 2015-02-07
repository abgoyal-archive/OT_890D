


#include <linux/math64.h>
#include "ubi.h"

static int gluebi_get_device(struct mtd_info *mtd)
{
	struct ubi_volume *vol;

	vol = container_of(mtd, struct ubi_volume, gluebi_mtd);

	/*
	 * We do not introduce locks for gluebi reference count because the
	 * get_device()/put_device() calls are already serialized at MTD.
	 */
	if (vol->gluebi_refcount > 0) {
		/*
		 * The MTD device is already referenced and this is just one
		 * more reference. MTD allows many users to open the same
		 * volume simultaneously and do not distinguish between
		 * readers/writers/exclusive openers as UBI does. So we do not
		 * open the UBI volume again - just increase the reference
		 * counter and return.
		 */
		vol->gluebi_refcount += 1;
		return 0;
	}

	/*
	 * This is the first reference to this UBI volume via the MTD device
	 * interface. Open the corresponding volume in read-write mode.
	 */
	vol->gluebi_desc = ubi_open_volume(vol->ubi->ubi_num, vol->vol_id,
					   UBI_READWRITE);
	if (IS_ERR(vol->gluebi_desc))
		return PTR_ERR(vol->gluebi_desc);
	vol->gluebi_refcount += 1;
	return 0;
}

static void gluebi_put_device(struct mtd_info *mtd)
{
	struct ubi_volume *vol;

	vol = container_of(mtd, struct ubi_volume, gluebi_mtd);
	vol->gluebi_refcount -= 1;
	ubi_assert(vol->gluebi_refcount >= 0);
	if (vol->gluebi_refcount == 0)
		ubi_close_volume(vol->gluebi_desc);
}

static int gluebi_read(struct mtd_info *mtd, loff_t from, size_t len,
		       size_t *retlen, unsigned char *buf)
{
	int err = 0, lnum, offs, total_read;
	struct ubi_volume *vol;
	struct ubi_device *ubi;

	dbg_gen("read %zd bytes from offset %lld", len, from);

	if (len < 0 || from < 0 || from + len > mtd->size)
		return -EINVAL;

	vol = container_of(mtd, struct ubi_volume, gluebi_mtd);
	ubi = vol->ubi;

	lnum = div_u64_rem(from, mtd->erasesize, &offs);
	total_read = len;
	while (total_read) {
		size_t to_read = mtd->erasesize - offs;

		if (to_read > total_read)
			to_read = total_read;

		err = ubi_eba_read_leb(ubi, vol, lnum, buf, offs, to_read, 0);
		if (err)
			break;

		lnum += 1;
		offs = 0;
		total_read -= to_read;
		buf += to_read;
	}

	*retlen = len - total_read;
	return err;
}

static int gluebi_write(struct mtd_info *mtd, loff_t to, size_t len,
		       size_t *retlen, const u_char *buf)
{
	int err = 0, lnum, offs, total_written;
	struct ubi_volume *vol;
	struct ubi_device *ubi;

	dbg_gen("write %zd bytes to offset %lld", len, to);

	if (len < 0 || to < 0 || len + to > mtd->size)
		return -EINVAL;

	vol = container_of(mtd, struct ubi_volume, gluebi_mtd);
	ubi = vol->ubi;

	if (ubi->ro_mode)
		return -EROFS;

	lnum = div_u64_rem(to, mtd->erasesize, &offs);

	if (len % mtd->writesize || offs % mtd->writesize)
		return -EINVAL;

	total_written = len;
	while (total_written) {
		size_t to_write = mtd->erasesize - offs;

		if (to_write > total_written)
			to_write = total_written;

		err = ubi_eba_write_leb(ubi, vol, lnum, buf, offs, to_write,
					UBI_UNKNOWN);
		if (err)
			break;

		lnum += 1;
		offs = 0;
		total_written -= to_write;
		buf += to_write;
	}

	*retlen = len - total_written;
	return err;
}

static int gluebi_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int err, i, lnum, count;
	struct ubi_volume *vol;
	struct ubi_device *ubi;

	dbg_gen("erase %llu bytes at offset %llu", (unsigned long long)instr->len,
		 (unsigned long long)instr->addr);

	if (instr->addr < 0 || instr->addr > mtd->size - mtd->erasesize)
		return -EINVAL;

	if (instr->len < 0 || instr->addr + instr->len > mtd->size)
		return -EINVAL;

	if (mtd_mod_by_ws(instr->addr, mtd) || mtd_mod_by_ws(instr->len, mtd))
		return -EINVAL;

	lnum = mtd_div_by_eb(instr->addr, mtd);
	count = mtd_div_by_eb(instr->len, mtd);

	vol = container_of(mtd, struct ubi_volume, gluebi_mtd);
	ubi = vol->ubi;

	if (ubi->ro_mode)
		return -EROFS;

	for (i = 0; i < count; i++) {
		err = ubi_eba_unmap_leb(ubi, vol, lnum + i);
		if (err)
			goto out_err;
	}

	/*
	 * MTD erase operations are synchronous, so we have to make sure the
	 * physical eraseblock is wiped out.
	 */
	err = ubi_wl_flush(ubi);
	if (err)
		goto out_err;

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);
	return 0;

out_err:
	instr->state = MTD_ERASE_FAILED;
	instr->fail_addr = (long long)lnum * mtd->erasesize;
	return err;
}

int ubi_create_gluebi(struct ubi_device *ubi, struct ubi_volume *vol)
{
	struct mtd_info *mtd = &vol->gluebi_mtd;

	mtd->name = kmemdup(vol->name, vol->name_len + 1, GFP_KERNEL);
	if (!mtd->name)
		return -ENOMEM;

	mtd->type = MTD_UBIVOLUME;
	if (!ubi->ro_mode)
		mtd->flags = MTD_WRITEABLE;
	mtd->writesize  = ubi->min_io_size;
	mtd->owner      = THIS_MODULE;
	mtd->erasesize  = vol->usable_leb_size;
	mtd->read       = gluebi_read;
	mtd->write      = gluebi_write;
	mtd->erase      = gluebi_erase;
	mtd->get_device = gluebi_get_device;
	mtd->put_device = gluebi_put_device;

	/*
	 * In case of dynamic volume, MTD device size is just volume size. In
	 * case of a static volume the size is equivalent to the amount of data
	 * bytes.
	 */
	if (vol->vol_type == UBI_DYNAMIC_VOLUME)
		mtd->size = (long long)vol->usable_leb_size * vol->reserved_pebs;
	else
		mtd->size = vol->used_bytes;

	if (add_mtd_device(mtd)) {
		ubi_err("cannot not add MTD device");
		kfree(mtd->name);
		return -ENFILE;
	}

	dbg_gen("added mtd%d (\"%s\"), size %llu, EB size %u",
		mtd->index, mtd->name, (unsigned long long)mtd->size, mtd->erasesize);
	return 0;
}

int ubi_destroy_gluebi(struct ubi_volume *vol)
{
	int err;
	struct mtd_info *mtd = &vol->gluebi_mtd;

	dbg_gen("remove mtd%d", mtd->index);
	err = del_mtd_device(mtd);
	if (err)
		return err;
	kfree(mtd->name);
	return 0;
}

void ubi_gluebi_updated(struct ubi_volume *vol)
{
	struct mtd_info *mtd = &vol->gluebi_mtd;

	if (vol->vol_type == UBI_STATIC_VOLUME)
		mtd->size = vol->used_bytes;
}
