

#include <linux/crc32.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/atomic.h>
#include "crc32defs.h"
#if CRC_LE_BITS == 8
#define tole(x) __constant_cpu_to_le32(x)
#define tobe(x) __constant_cpu_to_be32(x)
#else
#define tole(x) (x)
#define tobe(x) (x)
#endif
#include "crc32table.h"

MODULE_AUTHOR("Matt Domsch <Matt_Domsch@dell.com>");
MODULE_DESCRIPTION("Ethernet CRC32 calculations");
MODULE_LICENSE("GPL");

u32 __pure crc32_le(u32 crc, unsigned char const *p, size_t len);

#if CRC_LE_BITS == 1

u32 __pure crc32_le(u32 crc, unsigned char const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	return crc;
}
#else				/* Table-based approach */

u32 __pure crc32_le(u32 crc, unsigned char const *p, size_t len)
{
# if CRC_LE_BITS == 8
	const u32      *b =(u32 *)p;
	const u32      *tab = crc32table_le;

# ifdef __LITTLE_ENDIAN
#  define DO_CRC(x) crc = tab[ (crc ^ (x)) & 255 ] ^ (crc>>8)
# else
#  define DO_CRC(x) crc = tab[ ((crc >> 24) ^ (x)) & 255] ^ (crc<<8)
# endif

	crc = __cpu_to_le32(crc);
	/* Align it */
	if(unlikely(((long)b)&3 && len)){
		do {
			u8 *p = (u8 *)b;
			DO_CRC(*p++);
			b = (void *)p;
		} while ((--len) && ((long)b)&3 );
	}
	if(likely(len >= 4)){
		/* load data 32 bits wide, xor data 32 bits wide. */
		size_t save_len = len & 3;
	        len = len >> 2;
		--b; /* use pre increment below(*++b) for speed */
		do {
			crc ^= *++b;
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
		} while (--len);
		b++; /* point to next byte(s) */
		len = save_len;
	}
	/* And the last few bytes */
	if(len){
		do {
			u8 *p = (u8 *)b;
			DO_CRC(*p++);
			b = (void *)p;
		} while (--len);
	}

	return __le32_to_cpu(crc);
#undef ENDIAN_SHIFT
#undef DO_CRC

# elif CRC_LE_BITS == 4
	while (len--) {
		crc ^= *p++;
		crc = (crc >> 4) ^ crc32table_le[crc & 15];
		crc = (crc >> 4) ^ crc32table_le[crc & 15];
	}
	return crc;
# elif CRC_LE_BITS == 2
	while (len--) {
		crc ^= *p++;
		crc = (crc >> 2) ^ crc32table_le[crc & 3];
		crc = (crc >> 2) ^ crc32table_le[crc & 3];
		crc = (crc >> 2) ^ crc32table_le[crc & 3];
		crc = (crc >> 2) ^ crc32table_le[crc & 3];
	}
	return crc;
# endif
}
#endif

u32 __pure crc32_be(u32 crc, unsigned char const *p, size_t len);

#if CRC_BE_BITS == 1

u32 __pure crc32_be(u32 crc, unsigned char const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 24;
		for (i = 0; i < 8; i++)
			crc =
			    (crc << 1) ^ ((crc & 0x80000000) ? CRCPOLY_BE :
					  0);
	}
	return crc;
}

#else				/* Table-based approach */
u32 __pure crc32_be(u32 crc, unsigned char const *p, size_t len)
{
# if CRC_BE_BITS == 8
	const u32      *b =(u32 *)p;
	const u32      *tab = crc32table_be;

# ifdef __LITTLE_ENDIAN
#  define DO_CRC(x) crc = tab[ (crc ^ (x)) & 255 ] ^ (crc>>8)
# else
#  define DO_CRC(x) crc = tab[ ((crc >> 24) ^ (x)) & 255] ^ (crc<<8)
# endif

	crc = __cpu_to_be32(crc);
	/* Align it */
	if(unlikely(((long)b)&3 && len)){
		do {
			u8 *p = (u8 *)b;
			DO_CRC(*p++);
			b = (u32 *)p;
		} while ((--len) && ((long)b)&3 );
	}
	if(likely(len >= 4)){
		/* load data 32 bits wide, xor data 32 bits wide. */
		size_t save_len = len & 3;
	        len = len >> 2;
		--b; /* use pre increment below(*++b) for speed */
		do {
			crc ^= *++b;
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
		} while (--len);
		b++; /* point to next byte(s) */
		len = save_len;
	}
	/* And the last few bytes */
	if(len){
		do {
			u8 *p = (u8 *)b;
			DO_CRC(*p++);
			b = (void *)p;
		} while (--len);
	}
	return __be32_to_cpu(crc);
#undef ENDIAN_SHIFT
#undef DO_CRC

# elif CRC_BE_BITS == 4
	while (len--) {
		crc ^= *p++ << 24;
		crc = (crc << 4) ^ crc32table_be[crc >> 28];
		crc = (crc << 4) ^ crc32table_be[crc >> 28];
	}
	return crc;
# elif CRC_BE_BITS == 2
	while (len--) {
		crc ^= *p++ << 24;
		crc = (crc << 2) ^ crc32table_be[crc >> 30];
		crc = (crc << 2) ^ crc32table_be[crc >> 30];
		crc = (crc << 2) ^ crc32table_be[crc >> 30];
		crc = (crc << 2) ^ crc32table_be[crc >> 30];
	}
	return crc;
# endif
}
#endif

EXPORT_SYMBOL(crc32_le);
EXPORT_SYMBOL(crc32_be);


#ifdef UNITTEST

#include <stdlib.h>
#include <stdio.h>

#if 0				/*Not used at present */
static void
buf_dump(char const *prefix, unsigned char const *buf, size_t len)
{
	fputs(prefix, stdout);
	while (len--)
		printf(" %02x", *buf++);
	putchar('\n');

}
#endif

static void bytereverse(unsigned char *buf, size_t len)
{
	while (len--) {
		unsigned char x = bitrev8(*buf);
		*buf++ = x;
	}
}

static void random_garbage(unsigned char *buf, size_t len)
{
	while (len--)
		*buf++ = (unsigned char) random();
}

#if 0				/* Not used at present */
static void store_le(u32 x, unsigned char *buf)
{
	buf[0] = (unsigned char) x;
	buf[1] = (unsigned char) (x >> 8);
	buf[2] = (unsigned char) (x >> 16);
	buf[3] = (unsigned char) (x >> 24);
}
#endif

static void store_be(u32 x, unsigned char *buf)
{
	buf[0] = (unsigned char) (x >> 24);
	buf[1] = (unsigned char) (x >> 16);
	buf[2] = (unsigned char) (x >> 8);
	buf[3] = (unsigned char) x;
}

static u32 test_step(u32 init, unsigned char *buf, size_t len)
{
	u32 crc1, crc2;
	size_t i;

	crc1 = crc32_be(init, buf, len);
	store_be(crc1, buf + len);
	crc2 = crc32_be(init, buf, len + 4);
	if (crc2)
		printf("\nCRC cancellation fail: 0x%08x should be 0\n",
		       crc2);

	for (i = 0; i <= len + 4; i++) {
		crc2 = crc32_be(init, buf, i);
		crc2 = crc32_be(crc2, buf + i, len + 4 - i);
		if (crc2)
			printf("\nCRC split fail: 0x%08x\n", crc2);
	}

	/* Now swap it around for the other test */

	bytereverse(buf, len + 4);
	init = bitrev32(init);
	crc2 = bitrev32(crc1);
	if (crc1 != bitrev32(crc2))
		printf("\nBit reversal fail: 0x%08x -> 0x%08x -> 0x%08x\n",
		       crc1, crc2, bitrev32(crc2));
	crc1 = crc32_le(init, buf, len);
	if (crc1 != crc2)
		printf("\nCRC endianness fail: 0x%08x != 0x%08x\n", crc1,
		       crc2);
	crc2 = crc32_le(init, buf, len + 4);
	if (crc2)
		printf("\nCRC cancellation fail: 0x%08x should be 0\n",
		       crc2);

	for (i = 0; i <= len + 4; i++) {
		crc2 = crc32_le(init, buf, i);
		crc2 = crc32_le(crc2, buf + i, len + 4 - i);
		if (crc2)
			printf("\nCRC split fail: 0x%08x\n", crc2);
	}

	return crc1;
}

#define SIZE 64
#define INIT1 0
#define INIT2 0

int main(void)
{
	unsigned char buf1[SIZE + 4];
	unsigned char buf2[SIZE + 4];
	unsigned char buf3[SIZE + 4];
	int i, j;
	u32 crc1, crc2, crc3;

	for (i = 0; i <= SIZE; i++) {
		printf("\rTesting length %d...", i);
		fflush(stdout);
		random_garbage(buf1, i);
		random_garbage(buf2, i);
		for (j = 0; j < i; j++)
			buf3[j] = buf1[j] ^ buf2[j];

		crc1 = test_step(INIT1, buf1, i);
		crc2 = test_step(INIT2, buf2, i);
		/* Now check that CRC(buf1 ^ buf2) = CRC(buf1) ^ CRC(buf2) */
		crc3 = test_step(INIT1 ^ INIT2, buf3, i);
		if (crc3 != (crc1 ^ crc2))
			printf("CRC XOR fail: 0x%08x != 0x%08x ^ 0x%08x\n",
			       crc3, crc1, crc2);
	}
	printf("\nAll test complete.  No failures expected.\n");
	return 0;
}

#endif				/* UNITTEST */
