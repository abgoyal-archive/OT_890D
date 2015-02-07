

#define LOG_ELS                       0x1	/* ELS events */
#define LOG_DISCOVERY                 0x2	/* Link discovery events */
#define LOG_MBOX                      0x4	/* Mailbox events */
#define LOG_INIT                      0x8	/* Initialization events */
#define LOG_LINK_EVENT                0x10	/* Link events */
#define LOG_IP                        0x20	/* IP traffic history */
#define LOG_FCP                       0x40	/* FCP traffic history */
#define LOG_NODE                      0x80	/* Node table events */
#define LOG_TEMP                      0x100	/* Temperature sensor events */
#define LOG_BG			      0x200	/* BlockBuard events */
#define LOG_MISC                      0x400	/* Miscellaneous events */
#define LOG_SLI                       0x800	/* SLI events */
#define LOG_FCP_ERROR                 0x1000	/* log errors, not underruns */
#define LOG_LIBDFC                    0x2000	/* Libdfc events */
#define LOG_VPORT                     0x4000	/* NPIV events */
#define LOG_ALL_MSG                   0xffff	/* LOG all messages */

#define lpfc_printf_vlog(vport, level, mask, fmt, arg...) \
	do { \
	{ if (((mask) &(vport)->cfg_log_verbose) || (level[1] <= '3')) \
		dev_printk(level, &((vport)->phba->pcidev)->dev, "%d:(%d):" \
			   fmt, (vport)->phba->brd_no, vport->vpi, ##arg); } \
	} while (0)

#define lpfc_printf_log(phba, level, mask, fmt, arg...) \
	do { \
	{ if (((mask) &(phba)->pport->cfg_log_verbose) || (level[1] <= '3')) \
		dev_printk(level, &((phba)->pcidev)->dev, "%d:" \
			   fmt, phba->brd_no, ##arg); } \
	} while (0)
