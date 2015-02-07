

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

#include <asm/blackfin.h>




static DEFINE_SPINLOCK(dtest_lock);

/* Takes a void pointer */
#define IADDR2DTEST(x) \
	({ unsigned long __addr = (unsigned long)(x); \
		(__addr & 0x47F8)        | /* address bits 14 & 10:3 */ \
		(__addr & 0x0800) << 15  | /* address bit  11        */ \
		(__addr  & 0x3000) << 4  | /* address bits 13:12     */ \
		(__addr  & 0x8000) << 8  | /* address bit  15        */ \
		(0x1000004);               /* isram access           */ \
	})

/* Takes a pointer, and returns the offset (in bits) which things should be shifted */
#define ADDR2OFFSET(x) ((((unsigned long)(x)) & 0x7) * 8)

/* Takes a pointer, determines if it is the last byte in the isram 64-bit data type */
#define ADDR2LAST(x) ((((unsigned long)x) & 0x7) == 0x7)

static void isram_write(const void *addr, uint64_t data)
{
	uint32_t cmd;
	unsigned long flags;

	if (addr >= (void *)(L1_CODE_START + L1_CODE_LENGTH))
		return;

	cmd = IADDR2DTEST(addr) | 1;             /* write */

	/*
	 * Writes to DTEST_DATA[0:1] need to be atomic with write to DTEST_COMMAND
	 * While in exception context - atomicity is guaranteed or double fault
	 */
	spin_lock_irqsave(&dtest_lock, flags);

	bfin_write_DTEST_DATA0(data & 0xFFFFFFFF);
	bfin_write_DTEST_DATA1(data >> 32);

	/* use the builtin, since interrupts are already turned off */
	__builtin_bfin_csync();
	bfin_write_DTEST_COMMAND(cmd);
	__builtin_bfin_csync();

	bfin_write_DTEST_COMMAND(0);
	__builtin_bfin_csync();

	spin_unlock_irqrestore(&dtest_lock, flags);
}

static uint64_t isram_read(const void *addr)
{
	uint32_t cmd;
	unsigned long flags;
	uint64_t ret;

	if (addr > (void *)(L1_CODE_START + L1_CODE_LENGTH))
		return 0;

	cmd = IADDR2DTEST(addr) | 0;              /* read */

	/*
	 * Reads of DTEST_DATA[0:1] need to be atomic with write to DTEST_COMMAND
	 * While in exception context - atomicity is guaranteed or double fault
	 */
	spin_lock_irqsave(&dtest_lock, flags);
	/* use the builtin, since interrupts are already turned off */
	__builtin_bfin_csync();
	bfin_write_DTEST_COMMAND(cmd);
	__builtin_bfin_csync();
	ret = bfin_read_DTEST_DATA0() | ((uint64_t)bfin_read_DTEST_DATA1() << 32);

	bfin_write_DTEST_COMMAND(0);
	__builtin_bfin_csync();
	spin_unlock_irqrestore(&dtest_lock, flags);

	return ret;
}

static bool isram_check_addr(const void *addr, size_t n)
{
	if ((addr >= (void *)L1_CODE_START) &&
	    (addr < (void *)(L1_CODE_START + L1_CODE_LENGTH))) {
		if ((addr + n) >= (void *)(L1_CODE_START + L1_CODE_LENGTH)) {
			show_stack(NULL, NULL);
			printk(KERN_ERR "isram_memcpy: copy involving %p length "
					"(%zu) too long\n", addr, n);
		}
		return true;
	}
	return false;
}

void *isram_memcpy(void *dest, const void *src, size_t n)
{
	uint64_t data_in = 0, data_out = 0;
	size_t count;
	bool dest_in_l1, src_in_l1, need_data, put_data;
	unsigned char byte, *src_byte, *dest_byte;

	src_byte = (unsigned char *)src;
	dest_byte = (unsigned char *)dest;

	dest_in_l1 = isram_check_addr(dest, n);
	src_in_l1 = isram_check_addr(src, n);

	need_data = true;
	put_data = true;
	for (count = 0; count < n; count++) {
		if (src_in_l1) {
			if (need_data) {
				data_in = isram_read(src + count);
				need_data = false;
			}

			if (ADDR2LAST(src + count))
				need_data = true;

			byte = (unsigned char)((data_in >> ADDR2OFFSET(src + count)) & 0xff);

		} else {
			/* src is in L2 or L3 - so just dereference*/
			byte = src_byte[count];
		}

		if (dest_in_l1) {
			if (put_data) {
				data_out = isram_read(dest + count);
				put_data = false;
			}

			data_out &= ~((uint64_t)0xff << ADDR2OFFSET(dest + count));
			data_out |= ((uint64_t)byte << ADDR2OFFSET(dest + count));

			if (ADDR2LAST(dest + count)) {
				put_data = true;
				isram_write(dest + count, data_out);
			}
		} else {
			/* dest in L2 or L3 - so just dereference */
			dest_byte[count] = byte;
		}
	}

	/* make sure we dump the last byte if necessary */
	if (dest_in_l1 && !put_data)
		isram_write(dest + count, data_out);

	return dest;
}
EXPORT_SYMBOL(isram_memcpy);

