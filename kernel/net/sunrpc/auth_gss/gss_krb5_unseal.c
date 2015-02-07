



#include <linux/types.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/sunrpc/gss_krb5.h>
#include <linux/crypto.h>

#ifdef RPC_DEBUG
# define RPCDBG_FACILITY        RPCDBG_AUTH
#endif



u32
gss_verify_mic_kerberos(struct gss_ctx *gss_ctx,
		struct xdr_buf *message_buffer, struct xdr_netobj *read_token)
{
	struct krb5_ctx		*ctx = gss_ctx->internal_ctx_id;
	int			signalg;
	int			sealalg;
	char			cksumdata[16];
	struct xdr_netobj	md5cksum = {.len = 0, .data = cksumdata};
	s32			now;
	int			direction;
	u32			seqnum;
	unsigned char		*ptr = (unsigned char *)read_token->data;
	int			bodysize;

	dprintk("RPC:       krb5_read_token\n");

	if (g_verify_token_header(&ctx->mech_used, &bodysize, &ptr,
					read_token->len))
		return GSS_S_DEFECTIVE_TOKEN;

	if ((ptr[0] != ((KG_TOK_MIC_MSG >> 8) & 0xff)) ||
	    (ptr[1] !=  (KG_TOK_MIC_MSG & 0xff)))
		return GSS_S_DEFECTIVE_TOKEN;

	/* XXX sanity-check bodysize?? */

	signalg = ptr[2] + (ptr[3] << 8);
	if (signalg != SGN_ALG_DES_MAC_MD5)
		return GSS_S_DEFECTIVE_TOKEN;

	sealalg = ptr[4] + (ptr[5] << 8);
	if (sealalg != SEAL_ALG_NONE)
		return GSS_S_DEFECTIVE_TOKEN;

	if ((ptr[6] != 0xff) || (ptr[7] != 0xff))
		return GSS_S_DEFECTIVE_TOKEN;

	if (make_checksum("md5", ptr, 8, message_buffer, 0, &md5cksum))
		return GSS_S_FAILURE;

	if (krb5_encrypt(ctx->seq, NULL, md5cksum.data, md5cksum.data, 16))
		return GSS_S_FAILURE;

	if (memcmp(md5cksum.data + 8, ptr + GSS_KRB5_TOK_HDR_LEN, 8))
		return GSS_S_BAD_SIG;

	/* it got through unscathed.  Make sure the context is unexpired */

	now = get_seconds();

	if (now > ctx->endtime)
		return GSS_S_CONTEXT_EXPIRED;

	/* do sequencing checks */

	if (krb5_get_seq_num(ctx->seq, ptr + GSS_KRB5_TOK_HDR_LEN, ptr + 8, &direction, &seqnum))
		return GSS_S_FAILURE;

	if ((ctx->initiate && direction != 0xff) ||
	    (!ctx->initiate && direction != 0))
		return GSS_S_BAD_SIG;

	return GSS_S_COMPLETE;
}
