

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/buffer_head.h>
#include <linux/bitops.h>
#include <asm/byteorder.h>

#include <cluster/masklog.h>

#include "ocfs2.h"

#include "blockcheck.h"




static unsigned int calc_code_bit(unsigned int i, unsigned int *p_cache)
{
	unsigned int b, p = 0;

	/*
	 * Data bits are 0-based, but we're talking code bits, which
	 * are 1-based.
	 */
	b = i + 1;

	/* Use the cache if it is there */
	if (p_cache)
		p = *p_cache;
        b += p;

	/*
	 * For every power of two below our bit number, bump our bit.
	 *
	 * We compare with (b + 1) because we have to compare with what b
	 * would be _if_ it were bumped up by the parity bit.  Capice?
	 *
	 * p is set above.
	 */
	for (; (1 << p) < (b + 1); p++)
		b++;

	if (p_cache)
		*p_cache = p;

	return b;
}

u32 ocfs2_hamming_encode(u32 parity, void *data, unsigned int d, unsigned int nr)
{
	unsigned int i, b, p = 0;

	BUG_ON(!d);

	/*
	 * b is the hamming code bit number.  Hamming code specifies a
	 * 1-based array, but C uses 0-based.  So 'i' is for C, and 'b' is
	 * for the algorithm.
	 *
	 * The i++ in the for loop is so that the start offset passed
	 * to ocfs2_find_next_bit_set() is one greater than the previously
	 * found bit.
	 */
	for (i = 0; (i = ocfs2_find_next_bit(data, d, i)) < d; i++)
	{
		/*
		 * i is the offset in this hunk, nr + i is the total bit
		 * offset.
		 */
		b = calc_code_bit(nr + i, &p);

		/*
		 * Data bits in the resultant code are checked by
		 * parity bits that are part of the bit number
		 * representation.  Huh?
		 *
		 * <wikipedia href="http://en.wikipedia.org/wiki/Hamming_code">
		 * In other words, the parity bit at position 2^k
		 * checks bits in positions having bit k set in
		 * their binary representation.  Conversely, for
		 * instance, bit 13, i.e. 1101(2), is checked by
		 * bits 1000(2) = 8, 0100(2)=4 and 0001(2) = 1.
		 * </wikipedia>
		 *
		 * Note that 'k' is the _code_ bit number.  'b' in
		 * our loop.
		 */
		parity ^= b;
	}

	/* While the data buffer was treated as little endian, the
	 * return value is in host endian. */
	return parity;
}

u32 ocfs2_hamming_encode_block(void *data, unsigned int blocksize)
{
	return ocfs2_hamming_encode(0, data, blocksize * 8, 0);
}

void ocfs2_hamming_fix(void *data, unsigned int d, unsigned int nr,
		       unsigned int fix)
{
	unsigned int i, b;

	BUG_ON(!d);

	/*
	 * If the bit to fix has an hweight of 1, it's a parity bit.  One
	 * busted parity bit is its own error.  Nothing to do here.
	 */
	if (hweight32(fix) == 1)
		return;

	/*
	 * nr + d is the bit right past the data hunk we're looking at.
	 * If fix after that, nothing to do
	 */
	if (fix >= calc_code_bit(nr + d, NULL))
		return;

	/*
	 * nr is the offset in the data hunk we're starting at.  Let's
	 * start b at the offset in the code buffer.  See hamming_encode()
	 * for a more detailed description of 'b'.
	 */
	b = calc_code_bit(nr, NULL);
	/* If the fix is before this hunk, nothing to do */
	if (fix < b)
		return;

	for (i = 0; i < d; i++, b++)
	{
		/* Skip past parity bits */
		while (hweight32(b) == 1)
			b++;

		/*
		 * i is the offset in this data hunk.
		 * nr + i is the offset in the total data buffer.
		 * b is the offset in the total code buffer.
		 *
		 * Thus, when b == fix, bit i in the current hunk needs
		 * fixing.
		 */
		if (b == fix)
		{
			if (ocfs2_test_bit(i, data))
				ocfs2_clear_bit(i, data);
			else
				ocfs2_set_bit(i, data);
			break;
		}
	}
}

void ocfs2_hamming_fix_block(void *data, unsigned int blocksize,
			     unsigned int fix)
{
	ocfs2_hamming_fix(data, blocksize * 8, 0, fix);
}

void ocfs2_block_check_compute(void *data, size_t blocksize,
			       struct ocfs2_block_check *bc)
{
	u32 crc;
	u32 ecc;

	memset(bc, 0, sizeof(struct ocfs2_block_check));

	crc = crc32_le(~0, data, blocksize);
	ecc = ocfs2_hamming_encode_block(data, blocksize);

	/*
	 * No ecc'd ocfs2 structure is larger than 4K, so ecc will be no
	 * larger than 16 bits.
	 */
	BUG_ON(ecc > USHORT_MAX);

	bc->bc_crc32e = cpu_to_le32(crc);
	bc->bc_ecc = cpu_to_le16((u16)ecc);
}

int ocfs2_block_check_validate(void *data, size_t blocksize,
			       struct ocfs2_block_check *bc)
{
	int rc = 0;
	struct ocfs2_block_check check;
	u32 crc, ecc;

	check.bc_crc32e = le32_to_cpu(bc->bc_crc32e);
	check.bc_ecc = le16_to_cpu(bc->bc_ecc);

	memset(bc, 0, sizeof(struct ocfs2_block_check));

	/* Fast path - if the crc32 validates, we're good to go */
	crc = crc32_le(~0, data, blocksize);
	if (crc == check.bc_crc32e)
		goto out;

	mlog(ML_ERROR,
	     "CRC32 failed: stored: %u, computed %u.  Applying ECC.\n",
	     (unsigned int)check.bc_crc32e, (unsigned int)crc);

	/* Ok, try ECC fixups */
	ecc = ocfs2_hamming_encode_block(data, blocksize);
	ocfs2_hamming_fix_block(data, blocksize, ecc ^ check.bc_ecc);

	/* And check the crc32 again */
	crc = crc32_le(~0, data, blocksize);
	if (crc == check.bc_crc32e)
		goto out;

	mlog(ML_ERROR, "Fixed CRC32 failed: stored: %u, computed %u\n",
	     (unsigned int)check.bc_crc32e, (unsigned int)crc);

	rc = -EIO;

out:
	bc->bc_crc32e = cpu_to_le32(check.bc_crc32e);
	bc->bc_ecc = cpu_to_le16(check.bc_ecc);

	return rc;
}

void ocfs2_block_check_compute_bhs(struct buffer_head **bhs, int nr,
				   struct ocfs2_block_check *bc)
{
	int i;
	u32 crc, ecc;

	BUG_ON(nr < 0);

	if (!nr)
		return;

	memset(bc, 0, sizeof(struct ocfs2_block_check));

	for (i = 0, crc = ~0, ecc = 0; i < nr; i++) {
		crc = crc32_le(crc, bhs[i]->b_data, bhs[i]->b_size);
		/*
		 * The number of bits in a buffer is obviously b_size*8.
		 * The offset of this buffer is b_size*i, so the bit offset
		 * of this buffer is b_size*8*i.
		 */
		ecc = (u16)ocfs2_hamming_encode(ecc, bhs[i]->b_data,
						bhs[i]->b_size * 8,
						bhs[i]->b_size * 8 * i);
	}

	/*
	 * No ecc'd ocfs2 structure is larger than 4K, so ecc will be no
	 * larger than 16 bits.
	 */
	BUG_ON(ecc > USHORT_MAX);

	bc->bc_crc32e = cpu_to_le32(crc);
	bc->bc_ecc = cpu_to_le16((u16)ecc);
}

int ocfs2_block_check_validate_bhs(struct buffer_head **bhs, int nr,
				   struct ocfs2_block_check *bc)
{
	int i, rc = 0;
	struct ocfs2_block_check check;
	u32 crc, ecc, fix;

	BUG_ON(nr < 0);

	if (!nr)
		return 0;

	check.bc_crc32e = le32_to_cpu(bc->bc_crc32e);
	check.bc_ecc = le16_to_cpu(bc->bc_ecc);

	memset(bc, 0, sizeof(struct ocfs2_block_check));

	/* Fast path - if the crc32 validates, we're good to go */
	for (i = 0, crc = ~0; i < nr; i++)
		crc = crc32_le(crc, bhs[i]->b_data, bhs[i]->b_size);
	if (crc == check.bc_crc32e)
		goto out;

	mlog(ML_ERROR,
	     "CRC32 failed: stored: %u, computed %u.  Applying ECC.\n",
	     (unsigned int)check.bc_crc32e, (unsigned int)crc);

	/* Ok, try ECC fixups */
	for (i = 0, ecc = 0; i < nr; i++) {
		/*
		 * The number of bits in a buffer is obviously b_size*8.
		 * The offset of this buffer is b_size*i, so the bit offset
		 * of this buffer is b_size*8*i.
		 */
		ecc = (u16)ocfs2_hamming_encode(ecc, bhs[i]->b_data,
						bhs[i]->b_size * 8,
						bhs[i]->b_size * 8 * i);
	}
	fix = ecc ^ check.bc_ecc;
	for (i = 0; i < nr; i++) {
		/*
		 * Try the fix against each buffer.  It will only affect
		 * one of them.
		 */
		ocfs2_hamming_fix(bhs[i]->b_data, bhs[i]->b_size * 8,
				  bhs[i]->b_size * 8 * i, fix);
	}

	/* And check the crc32 again */
	for (i = 0, crc = ~0; i < nr; i++)
		crc = crc32_le(crc, bhs[i]->b_data, bhs[i]->b_size);
	if (crc == check.bc_crc32e)
		goto out;

	mlog(ML_ERROR, "Fixed CRC32 failed: stored: %u, computed %u\n",
	     (unsigned int)check.bc_crc32e, (unsigned int)crc);

	rc = -EIO;

out:
	bc->bc_crc32e = cpu_to_le32(check.bc_crc32e);
	bc->bc_ecc = cpu_to_le16(check.bc_ecc);

	return rc;
}

void ocfs2_compute_meta_ecc(struct super_block *sb, void *data,
			    struct ocfs2_block_check *bc)
{
	if (ocfs2_meta_ecc(OCFS2_SB(sb)))
		ocfs2_block_check_compute(data, sb->s_blocksize, bc);
}

int ocfs2_validate_meta_ecc(struct super_block *sb, void *data,
			    struct ocfs2_block_check *bc)
{
	int rc = 0;

	if (ocfs2_meta_ecc(OCFS2_SB(sb)))
		rc = ocfs2_block_check_validate(data, sb->s_blocksize, bc);

	return rc;
}

void ocfs2_compute_meta_ecc_bhs(struct super_block *sb,
				struct buffer_head **bhs, int nr,
				struct ocfs2_block_check *bc)
{
	if (ocfs2_meta_ecc(OCFS2_SB(sb)))
		ocfs2_block_check_compute_bhs(bhs, nr, bc);
}

int ocfs2_validate_meta_ecc_bhs(struct super_block *sb,
				struct buffer_head **bhs, int nr,
				struct ocfs2_block_check *bc)
{
	int rc = 0;

	if (ocfs2_meta_ecc(OCFS2_SB(sb)))
		rc = ocfs2_block_check_validate_bhs(bhs, nr, bc);

	return rc;
}

