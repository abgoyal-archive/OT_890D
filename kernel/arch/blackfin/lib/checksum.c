

#include <linux/module.h>
#include <net/checksum.h>
#include <asm/checksum.h>

#ifdef CONFIG_IP_CHECKSUM_L1
static unsigned short do_csum(const unsigned char *buff, int len)__attribute__((l1_text));
#endif

static unsigned short do_csum(const unsigned char *buff, int len)
{
	register unsigned long sum = 0;
	int swappem = 0;

	if (1 & (unsigned long)buff) {
		sum = *buff << 8;
		buff++;
		len--;
		++swappem;
	}

	while (len > 1) {
		sum += *(unsigned short *)buff;
		buff += 2;
		len -= 2;
	}

	if (len > 0)
		sum += *buff;

	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	if (swappem)
		sum = ((sum & 0xff00) >> 8) + ((sum & 0x00ff) << 8);

	return sum;

}

__sum16 ip_fast_csum(unsigned char *iph, unsigned int ihl)
{
	return (__force __sum16)~do_csum(iph, ihl * 4);
}
EXPORT_SYMBOL(ip_fast_csum);

__wsum csum_partial(const void *buff, int len, __wsum sum)
{
	/*
	 * Just in case we get nasty checksum data...
	 * Like 0xffff6ec3 in the case of our IPv6 multicast header.
	 * We fold to begin with, as well as at the end.
	 */
	sum = (sum & 0xffff) + (sum >> 16);

	sum += do_csum(buff, len);

	sum = (sum & 0xffff) + (sum >> 16);

	return sum;
}
EXPORT_SYMBOL(csum_partial);

__sum16 ip_compute_csum(const void *buff, int len)
{
	return (__force __sum16)~do_csum(buff, len);
}


__wsum
csum_partial_copy_from_user(const void __user *src, void *dst,
			    int len, __wsum sum, int *csum_err)
{
	if (csum_err)
		*csum_err = 0;
	memcpy(dst, (__force void *)src, len);
	return csum_partial(dst, len, sum);
}


__wsum csum_partial_copy(const void *src, void *dst, int len, __wsum sum)
{
	memcpy(dst, src, len);
	return csum_partial(dst, len, sum);
}
EXPORT_SYMBOL(csum_partial_copy);
