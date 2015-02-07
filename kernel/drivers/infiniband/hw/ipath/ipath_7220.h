
#ifndef _IPATH_7220_H
#define _IPATH_7220_H

int ipath_sd7220_presets(struct ipath_devdata *dd);
int ipath_sd7220_init(struct ipath_devdata *dd, int was_reset);
int ipath_sd7220_prog_ld(struct ipath_devdata *dd, int sdnum, u8 *img,
	int len, int offset);
int ipath_sd7220_prog_vfy(struct ipath_devdata *dd, int sdnum, const u8 *img,
	int len, int offset);
#define IB_7220_SERDES 2

int ipath_sd7220_ib_load(struct ipath_devdata *dd);
int ipath_sd7220_ib_vfy(struct ipath_devdata *dd);

#endif /* _IPATH_7220_H */
