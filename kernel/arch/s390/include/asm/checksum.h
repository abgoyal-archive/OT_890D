
#ifndef _S390_CHECKSUM_H
#define _S390_CHECKSUM_H


#include <asm/uaccess.h>

static inline __wsum
csum_partial(const void *buff, int len, __wsum sum)
{
	register unsigned long reg2 asm("2") = (unsigned long) buff;
	register unsigned long reg3 asm("3") = (unsigned long) len;

	asm volatile(
		"0:	cksm	%0,%1\n"	/* do checksum on longs */
		"	jo	0b\n"
		: "+d" (sum), "+d" (reg2), "+d" (reg3) : : "cc", "memory");
	return sum;
}

static inline __wsum
csum_partial_copy_from_user(const void __user *src, void *dst,
                                          int len, __wsum sum,
                                          int *err_ptr)
{
	int missing;

	missing = copy_from_user(dst, src, len);
	if (missing) {
		memset(dst + len - missing, 0, missing);
		*err_ptr = -EFAULT;
	}
		
	return csum_partial(dst, len, sum);
}


static inline __wsum
csum_partial_copy_nocheck (const void *src, void *dst, int len, __wsum sum)
{
        memcpy(dst,src,len);
	return csum_partial(dst, len, sum);
}

static inline __sum16 csum_fold(__wsum sum)
{
#ifndef __s390x__
	register_pair rp;

	asm volatile(
		"	slr	%N1,%N1\n"	/* %0 = H L */
		"	lr	%1,%0\n"	/* %0 = H L, %1 = H L 0 0 */
		"	srdl	%1,16\n"	/* %0 = H L, %1 = 0 H L 0 */
		"	alr	%1,%N1\n"	/* %0 = H L, %1 = L H L 0 */
		"	alr	%0,%1\n"	/* %0 = H+L+C L+H */
		"	srl	%0,16\n"	/* %0 = H+L+C */
		: "+&d" (sum), "=d" (rp) : : "cc");
#else /* __s390x__ */
	asm volatile(
		"	sr	3,3\n"		/* %0 = H*65536 + L */
		"	lr	2,%0\n"		/* %0 = H L, 2/3 = H L / 0 0 */
		"	srdl	2,16\n"		/* %0 = H L, 2/3 = 0 H / L 0 */
		"	alr	2,3\n"		/* %0 = H L, 2/3 = L H / L 0 */
		"	alr	%0,2\n"		/* %0 = H+L+C L+H */
		"	srl	%0,16\n"	/* %0 = H+L+C */
		: "+&d" (sum) : : "cc", "2", "3");
#endif /* __s390x__ */
	return (__force __sum16) ~sum;
}

static inline __sum16 ip_fast_csum(const void *iph, unsigned int ihl)
{
	return csum_fold(csum_partial(iph, ihl*4, 0));
}

static inline __wsum
csum_tcpudp_nofold(__be32 saddr, __be32 daddr,
                   unsigned short len, unsigned short proto,
                   __wsum sum)
{
	__u32 csum = (__force __u32)sum;

	csum += (__force __u32)saddr;
	if (csum < (__force __u32)saddr)
		csum++;

	csum += (__force __u32)daddr;
	if (csum < (__force __u32)daddr)
		csum++;

	csum += len + proto;
	if (csum < len + proto)
		csum++;

	return (__force __wsum)csum;
}


static inline __sum16
csum_tcpudp_magic(__be32 saddr, __be32 daddr,
                  unsigned short len, unsigned short proto,
                  __wsum sum)
{
	return csum_fold(csum_tcpudp_nofold(saddr,daddr,len,proto,sum));
}


static inline __sum16 ip_compute_csum(const void *buff, int len)
{
	return csum_fold(csum_partial(buff, len, 0));
}

#endif /* _S390_CHECKSUM_H */


