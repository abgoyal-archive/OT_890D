

/*! \file */

#if !defined(_BIT_OPERATIONS_H_)
#define _BIT_OPERATIONS_H_

#if defined(__i386__)  ||  defined(__x86_64__)
static __inline__ int top_bit(unsigned int bits)
{
	int res;

	__asm__(" xorl %[res],%[res];\n"
		" decl %[res];\n"
		" bsrl %[bits],%[res]\n"
		:[res] "=&r" (res)
		:[bits] "rm"(bits)
	);
	return res;
}

static __inline__ int bottom_bit(unsigned int bits)
{
	int res;

	__asm__(" xorl %[res],%[res];\n"
		" decl %[res];\n"
		" bsfl %[bits],%[res]\n"
		:[res] "=&r" (res)
		:[bits] "rm"(bits)
	);
	return res;
}
#else
static __inline__ int top_bit(unsigned int bits)
{
	int i;

	if (bits == 0)
		return -1;
	i = 0;
	if (bits & 0xFFFF0000) {
		bits &= 0xFFFF0000;
		i += 16;
	}
	if (bits & 0xFF00FF00) {
		bits &= 0xFF00FF00;
		i += 8;
	}
	if (bits & 0xF0F0F0F0) {
		bits &= 0xF0F0F0F0;
		i += 4;
	}
	if (bits & 0xCCCCCCCC) {
		bits &= 0xCCCCCCCC;
		i += 2;
	}
	if (bits & 0xAAAAAAAA) {
		bits &= 0xAAAAAAAA;
		i += 1;
	}
	return i;
}

static __inline__ int bottom_bit(unsigned int bits)
{
	int i;

	if (bits == 0)
		return -1;
	i = 32;
	if (bits & 0x0000FFFF) {
		bits &= 0x0000FFFF;
		i -= 16;
	}
	if (bits & 0x00FF00FF) {
		bits &= 0x00FF00FF;
		i -= 8;
	}
	if (bits & 0x0F0F0F0F) {
		bits &= 0x0F0F0F0F;
		i -= 4;
	}
	if (bits & 0x33333333) {
		bits &= 0x33333333;
		i -= 2;
	}
	if (bits & 0x55555555) {
		bits &= 0x55555555;
		i -= 1;
	}
	return i;
}
#endif

static __inline__ uint8_t bit_reverse8(uint8_t x)
{
#if defined(__i386__)  ||  defined(__x86_64__)
	/* If multiply is fast */
	return ((x * 0x0802U & 0x22110U) | (x * 0x8020U & 0x88440U)) *
	    0x10101U >> 16;
#else
	/* If multiply is slow, but we have a barrel shifter */
	x = (x >> 4) | (x << 4);
	x = ((x & 0xCC) >> 2) | ((x & 0x33) << 2);
	return ((x & 0xAA) >> 1) | ((x & 0x55) << 1);
#endif
}

uint16_t bit_reverse16(uint16_t data);

uint32_t bit_reverse32(uint32_t data);

uint32_t bit_reverse_4bytes(uint32_t data);

int one_bits32(uint32_t x);

uint32_t make_mask32(uint32_t x);

uint16_t make_mask16(uint16_t x);

static __inline__ uint32_t least_significant_one32(uint32_t x)
{
	return (x & (-(int32_t) x));
}

static __inline__ uint32_t most_significant_one32(uint32_t x)
{
#if defined(__i386__)  ||  defined(__x86_64__)
	return 1 << top_bit(x);
#else
	x = make_mask32(x);
	return (x ^ (x >> 1));
#endif
}

static __inline__ int parity8(uint8_t x)
{
	x = (x ^ (x >> 4)) & 0x0F;
	return (0x6996 >> x) & 1;
}

static __inline__ int parity16(uint16_t x)
{
	x ^= (x >> 8);
	x = (x ^ (x >> 4)) & 0x0F;
	return (0x6996 >> x) & 1;
}

static __inline__ int parity32(uint32_t x)
{
	x ^= (x >> 16);
	x ^= (x >> 8);
	x = (x ^ (x >> 4)) & 0x0F;
	return (0x6996 >> x) & 1;
}

#endif
/*- End of file ------------------------------------------------------------*/
