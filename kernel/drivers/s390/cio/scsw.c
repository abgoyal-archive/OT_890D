

#include <linux/types.h>
#include <linux/module.h>
#include <asm/cio.h>
#include "css.h"
#include "chsc.h"

int scsw_is_tm(union scsw *scsw)
{
	return css_general_characteristics.fcx && (scsw->tm.x == 1);
}
EXPORT_SYMBOL(scsw_is_tm);

u32 scsw_key(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.key;
	else
		return scsw->cmd.key;
}
EXPORT_SYMBOL(scsw_key);

u32 scsw_eswf(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.eswf;
	else
		return scsw->cmd.eswf;
}
EXPORT_SYMBOL(scsw_eswf);

u32 scsw_cc(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.cc;
	else
		return scsw->cmd.cc;
}
EXPORT_SYMBOL(scsw_cc);

u32 scsw_ectl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.ectl;
	else
		return scsw->cmd.ectl;
}
EXPORT_SYMBOL(scsw_ectl);

u32 scsw_pno(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.pno;
	else
		return scsw->cmd.pno;
}
EXPORT_SYMBOL(scsw_pno);

u32 scsw_fctl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.fctl;
	else
		return scsw->cmd.fctl;
}
EXPORT_SYMBOL(scsw_fctl);

u32 scsw_actl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.actl;
	else
		return scsw->cmd.actl;
}
EXPORT_SYMBOL(scsw_actl);

u32 scsw_stctl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.stctl;
	else
		return scsw->cmd.stctl;
}
EXPORT_SYMBOL(scsw_stctl);

u32 scsw_dstat(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.dstat;
	else
		return scsw->cmd.dstat;
}
EXPORT_SYMBOL(scsw_dstat);

u32 scsw_cstat(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw->tm.cstat;
	else
		return scsw->cmd.cstat;
}
EXPORT_SYMBOL(scsw_cstat);

int scsw_cmd_is_valid_key(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_key);

int scsw_cmd_is_valid_sctl(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_sctl);

int scsw_cmd_is_valid_eswf(union scsw *scsw)
{
	return (scsw->cmd.stctl & SCSW_STCTL_STATUS_PEND);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_eswf);

int scsw_cmd_is_valid_cc(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC) &&
	       (scsw->cmd.stctl & SCSW_STCTL_STATUS_PEND);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_cc);

int scsw_cmd_is_valid_fmt(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_fmt);

int scsw_cmd_is_valid_pfch(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_pfch);

int scsw_cmd_is_valid_isic(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_isic);

int scsw_cmd_is_valid_alcc(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_alcc);

int scsw_cmd_is_valid_ssi(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_ssi);

int scsw_cmd_is_valid_zcc(union scsw *scsw)
{
	return (scsw->cmd.fctl & SCSW_FCTL_START_FUNC) &&
	       (scsw->cmd.stctl & SCSW_STCTL_INTER_STATUS);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_zcc);

int scsw_cmd_is_valid_ectl(union scsw *scsw)
{
	return (scsw->cmd.stctl & SCSW_STCTL_STATUS_PEND) &&
	       !(scsw->cmd.stctl & SCSW_STCTL_INTER_STATUS) &&
	       (scsw->cmd.stctl & SCSW_STCTL_ALERT_STATUS);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_ectl);

int scsw_cmd_is_valid_pno(union scsw *scsw)
{
	return (scsw->cmd.fctl != 0) &&
	       (scsw->cmd.stctl & SCSW_STCTL_STATUS_PEND) &&
	       (!(scsw->cmd.stctl & SCSW_STCTL_INTER_STATUS) ||
		 ((scsw->cmd.stctl & SCSW_STCTL_INTER_STATUS) &&
		  (scsw->cmd.actl & SCSW_ACTL_SUSPENDED)));
}
EXPORT_SYMBOL(scsw_cmd_is_valid_pno);

int scsw_cmd_is_valid_fctl(union scsw *scsw)
{
	/* Only valid if pmcw.dnv == 1*/
	return 1;
}
EXPORT_SYMBOL(scsw_cmd_is_valid_fctl);

int scsw_cmd_is_valid_actl(union scsw *scsw)
{
	/* Only valid if pmcw.dnv == 1*/
	return 1;
}
EXPORT_SYMBOL(scsw_cmd_is_valid_actl);

int scsw_cmd_is_valid_stctl(union scsw *scsw)
{
	/* Only valid if pmcw.dnv == 1*/
	return 1;
}
EXPORT_SYMBOL(scsw_cmd_is_valid_stctl);

int scsw_cmd_is_valid_dstat(union scsw *scsw)
{
	return (scsw->cmd.stctl & SCSW_STCTL_STATUS_PEND) &&
	       (scsw->cmd.cc != 3);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_dstat);

int scsw_cmd_is_valid_cstat(union scsw *scsw)
{
	return (scsw->cmd.stctl & SCSW_STCTL_STATUS_PEND) &&
	       (scsw->cmd.cc != 3);
}
EXPORT_SYMBOL(scsw_cmd_is_valid_cstat);

int scsw_tm_is_valid_key(union scsw *scsw)
{
	return (scsw->tm.fctl & SCSW_FCTL_START_FUNC);
}
EXPORT_SYMBOL(scsw_tm_is_valid_key);

int scsw_tm_is_valid_eswf(union scsw *scsw)
{
	return (scsw->tm.stctl & SCSW_STCTL_STATUS_PEND);
}
EXPORT_SYMBOL(scsw_tm_is_valid_eswf);

int scsw_tm_is_valid_cc(union scsw *scsw)
{
	return (scsw->tm.fctl & SCSW_FCTL_START_FUNC) &&
	       (scsw->tm.stctl & SCSW_STCTL_STATUS_PEND);
}
EXPORT_SYMBOL(scsw_tm_is_valid_cc);

int scsw_tm_is_valid_fmt(union scsw *scsw)
{
	return 1;
}
EXPORT_SYMBOL(scsw_tm_is_valid_fmt);

int scsw_tm_is_valid_x(union scsw *scsw)
{
	return 1;
}
EXPORT_SYMBOL(scsw_tm_is_valid_x);

int scsw_tm_is_valid_q(union scsw *scsw)
{
	return 1;
}
EXPORT_SYMBOL(scsw_tm_is_valid_q);

int scsw_tm_is_valid_ectl(union scsw *scsw)
{
	return (scsw->tm.stctl & SCSW_STCTL_STATUS_PEND) &&
	       !(scsw->tm.stctl & SCSW_STCTL_INTER_STATUS) &&
	       (scsw->tm.stctl & SCSW_STCTL_ALERT_STATUS);
}
EXPORT_SYMBOL(scsw_tm_is_valid_ectl);

int scsw_tm_is_valid_pno(union scsw *scsw)
{
	return (scsw->tm.fctl != 0) &&
	       (scsw->tm.stctl & SCSW_STCTL_STATUS_PEND) &&
	       (!(scsw->tm.stctl & SCSW_STCTL_INTER_STATUS) ||
		 ((scsw->tm.stctl & SCSW_STCTL_INTER_STATUS) &&
		  (scsw->tm.actl & SCSW_ACTL_SUSPENDED)));
}
EXPORT_SYMBOL(scsw_tm_is_valid_pno);

int scsw_tm_is_valid_fctl(union scsw *scsw)
{
	/* Only valid if pmcw.dnv == 1*/
	return 1;
}
EXPORT_SYMBOL(scsw_tm_is_valid_fctl);

int scsw_tm_is_valid_actl(union scsw *scsw)
{
	/* Only valid if pmcw.dnv == 1*/
	return 1;
}
EXPORT_SYMBOL(scsw_tm_is_valid_actl);

int scsw_tm_is_valid_stctl(union scsw *scsw)
{
	/* Only valid if pmcw.dnv == 1*/
	return 1;
}
EXPORT_SYMBOL(scsw_tm_is_valid_stctl);

int scsw_tm_is_valid_dstat(union scsw *scsw)
{
	return (scsw->tm.stctl & SCSW_STCTL_STATUS_PEND) &&
	       (scsw->tm.cc != 3);
}
EXPORT_SYMBOL(scsw_tm_is_valid_dstat);

int scsw_tm_is_valid_cstat(union scsw *scsw)
{
	return (scsw->tm.stctl & SCSW_STCTL_STATUS_PEND) &&
	       (scsw->tm.cc != 3);
}
EXPORT_SYMBOL(scsw_tm_is_valid_cstat);

int scsw_tm_is_valid_fcxs(union scsw *scsw)
{
	return 1;
}
EXPORT_SYMBOL(scsw_tm_is_valid_fcxs);

int scsw_tm_is_valid_schxs(union scsw *scsw)
{
	return (scsw->tm.cstat & (SCHN_STAT_PROG_CHECK |
				  SCHN_STAT_INTF_CTRL_CHK |
				  SCHN_STAT_PROT_CHECK |
				  SCHN_STAT_CHN_DATA_CHK));
}
EXPORT_SYMBOL(scsw_tm_is_valid_schxs);

int scsw_is_valid_actl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_actl(scsw);
	else
		return scsw_cmd_is_valid_actl(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_actl);

int scsw_is_valid_cc(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_cc(scsw);
	else
		return scsw_cmd_is_valid_cc(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_cc);

int scsw_is_valid_cstat(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_cstat(scsw);
	else
		return scsw_cmd_is_valid_cstat(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_cstat);

int scsw_is_valid_dstat(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_dstat(scsw);
	else
		return scsw_cmd_is_valid_dstat(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_dstat);

int scsw_is_valid_ectl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_ectl(scsw);
	else
		return scsw_cmd_is_valid_ectl(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_ectl);

int scsw_is_valid_eswf(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_eswf(scsw);
	else
		return scsw_cmd_is_valid_eswf(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_eswf);

int scsw_is_valid_fctl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_fctl(scsw);
	else
		return scsw_cmd_is_valid_fctl(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_fctl);

int scsw_is_valid_key(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_key(scsw);
	else
		return scsw_cmd_is_valid_key(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_key);

int scsw_is_valid_pno(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_pno(scsw);
	else
		return scsw_cmd_is_valid_pno(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_pno);

int scsw_is_valid_stctl(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_valid_stctl(scsw);
	else
		return scsw_cmd_is_valid_stctl(scsw);
}
EXPORT_SYMBOL(scsw_is_valid_stctl);

int scsw_cmd_is_solicited(union scsw *scsw)
{
	return (scsw->cmd.cc != 0) || (scsw->cmd.stctl !=
		(SCSW_STCTL_STATUS_PEND | SCSW_STCTL_ALERT_STATUS));
}
EXPORT_SYMBOL(scsw_cmd_is_solicited);

int scsw_tm_is_solicited(union scsw *scsw)
{
	return (scsw->tm.cc != 0) || (scsw->tm.stctl !=
		(SCSW_STCTL_STATUS_PEND | SCSW_STCTL_ALERT_STATUS));
}
EXPORT_SYMBOL(scsw_tm_is_solicited);

int scsw_is_solicited(union scsw *scsw)
{
	if (scsw_is_tm(scsw))
		return scsw_tm_is_solicited(scsw);
	else
		return scsw_cmd_is_solicited(scsw);
}
EXPORT_SYMBOL(scsw_is_solicited);
