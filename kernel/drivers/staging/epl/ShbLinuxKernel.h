

#ifndef _SHBLINUXKERNEL_H_
#define _SHBLINUXKERNEL_H_

struct sShbMemTable {
	int m_iBufferId;
	void *m_pBuffer;
	struct sShbMemTable *m_psNextMemTableElement;
};

extern struct sShbMemTable *psMemTableElementFirst_g;

#endif // _SHBLINUXKERNEL_H_
