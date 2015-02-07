


#include <linux/types.h>
#include <linux/slab.h>
#include <linux/sunrpc/gss_krb5.h>
#include <linux/crypto.h>

#ifdef RPC_DEBUG
# define RPCDBG_FACILITY        RPCDBG_AUTH
#endif

s32
krb5_make_seq_num(struct crypto_blkcipher *key,
		int direction,
		u32 seqnum,
		unsigned char *cksum, unsigned char *buf)
{
	unsigned char plain[8];

	plain[0] = (unsigned char) (seqnum & 0xff);
	plain[1] = (unsigned char) ((seqnum >> 8) & 0xff);
	plain[2] = (unsigned char) ((seqnum >> 16) & 0xff);
	plain[3] = (unsigned char) ((seqnum >> 24) & 0xff);

	plain[4] = direction;
	plain[5] = direction;
	plain[6] = direction;
	plain[7] = direction;

	return krb5_encrypt(key, cksum, plain, buf, 8);
}

s32
krb5_get_seq_num(struct crypto_blkcipher *key,
	       unsigned char *cksum,
	       unsigned char *buf,
	       int *direction, u32 *seqnum)
{
	s32 code;
	unsigned char plain[8];

	dprintk("RPC:       krb5_get_seq_num:\n");

	if ((code = krb5_decrypt(key, cksum, buf, plain, 8)))
		return code;

	if ((plain[4] != plain[5]) || (plain[4] != plain[6])
				   || (plain[4] != plain[7]))
		return (s32)KG_BAD_SEQ;

	*direction = plain[4];

	*seqnum = ((plain[0]) |
		   (plain[1] << 8) | (plain[2] << 16) | (plain[3] << 24));

	return (0);
}
