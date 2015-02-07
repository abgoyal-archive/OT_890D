


#include <asm/unaligned.h>
#include <scsi/fc/fc_gs.h>
#include <scsi/fc/fc_ns.h>
#include <scsi/fc/fc_els.h>
#include <scsi/libfc.h>
#include <scsi/fc_encode.h>

static struct fc_seq *fc_elsct_send(struct fc_lport *lport,
				    struct fc_rport *rport,
				    struct fc_frame *fp,
				    unsigned int op,
				    void (*resp)(struct fc_seq *,
						 struct fc_frame *fp,
						 void *arg),
				    void *arg, u32 timer_msec)
{
	enum fc_rctl r_ctl;
	u32 did;
	enum fc_fh_type fh_type;
	int rc;

	/* ELS requests */
	if ((op >= ELS_LS_RJT) && (op <= ELS_AUTH_ELS))
		rc = fc_els_fill(lport, rport, fp, op, &r_ctl, &did, &fh_type);
	else
		/* CT requests */
		rc = fc_ct_fill(lport, fp, op, &r_ctl, &did, &fh_type);

	if (rc)
		return NULL;

	fc_fill_fc_hdr(fp, r_ctl, did, fc_host_port_id(lport->host), fh_type,
		       FC_FC_FIRST_SEQ | FC_FC_END_SEQ | FC_FC_SEQ_INIT, 0);

	return lport->tt.exch_seq_send(lport, fp, resp, NULL, arg, timer_msec);
}

int fc_elsct_init(struct fc_lport *lport)
{
	if (!lport->tt.elsct_send)
		lport->tt.elsct_send = fc_elsct_send;

	return 0;
}
EXPORT_SYMBOL(fc_elsct_init);
