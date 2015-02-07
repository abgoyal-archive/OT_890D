
/* Kernel module to match ESP parameters. */


#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>

#include <linux/netfilter/xt_esp.h>
#include <linux/netfilter/x_tables.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv6/ip6_tables.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yon Uriarte <yon@astaro.de>");
MODULE_DESCRIPTION("Xtables: IPsec-ESP packet match");
MODULE_ALIAS("ipt_esp");
MODULE_ALIAS("ip6t_esp");

#if 0
#define duprintf(format, args...) printk(format , ## args)
#else
#define duprintf(format, args...)
#endif

/* Returns 1 if the spi is matched by the range, 0 otherwise */
static inline bool
spi_match(u_int32_t min, u_int32_t max, u_int32_t spi, bool invert)
{
	bool r;
	duprintf("esp spi_match:%c 0x%x <= 0x%x <= 0x%x", invert ? '!' : ' ',
		 min, spi, max);
	r = (spi >= min && spi <= max) ^ invert;
	duprintf(" result %s\n", r ? "PASS" : "FAILED");
	return r;
}

static bool esp_mt(const struct sk_buff *skb, const struct xt_match_param *par)
{
	const struct ip_esp_hdr *eh;
	struct ip_esp_hdr _esp;
	const struct xt_esp *espinfo = par->matchinfo;

	/* Must not be a fragment. */
	if (par->fragoff != 0)
		return false;

	eh = skb_header_pointer(skb, par->thoff, sizeof(_esp), &_esp);
	if (eh == NULL) {
		/* We've been asked to examine this packet, and we
		 * can't.  Hence, no choice but to drop.
		 */
		duprintf("Dropping evil ESP tinygram.\n");
		*par->hotdrop = true;
		return false;
	}

	return spi_match(espinfo->spis[0], espinfo->spis[1], ntohl(eh->spi),
			 !!(espinfo->invflags & XT_ESP_INV_SPI));
}

static bool esp_mt_check(const struct xt_mtchk_param *par)
{
	const struct xt_esp *espinfo = par->matchinfo;

	if (espinfo->invflags & ~XT_ESP_INV_MASK) {
		duprintf("xt_esp: unknown flags %X\n", espinfo->invflags);
		return false;
	}

	return true;
}

static struct xt_match esp_mt_reg[] __read_mostly = {
	{
		.name		= "esp",
		.family		= NFPROTO_IPV4,
		.checkentry	= esp_mt_check,
		.match		= esp_mt,
		.matchsize	= sizeof(struct xt_esp),
		.proto		= IPPROTO_ESP,
		.me		= THIS_MODULE,
	},
	{
		.name		= "esp",
		.family		= NFPROTO_IPV6,
		.checkentry	= esp_mt_check,
		.match		= esp_mt,
		.matchsize	= sizeof(struct xt_esp),
		.proto		= IPPROTO_ESP,
		.me		= THIS_MODULE,
	},
};

static int __init esp_mt_init(void)
{
	return xt_register_matches(esp_mt_reg, ARRAY_SIZE(esp_mt_reg));
}

static void __exit esp_mt_exit(void)
{
	xt_unregister_matches(esp_mt_reg, ARRAY_SIZE(esp_mt_reg));
}

module_init(esp_mt_init);
module_exit(esp_mt_exit);
