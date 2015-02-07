

#ifndef _LINUX_SUNRPC_XPRTRDMA_H
#define _LINUX_SUNRPC_XPRTRDMA_H

#define XPRT_TRANSPORT_RDMA	256

#define RPCBIND_NETID_RDMA	"rdma"

#define RPCRDMA_MIN_SLOT_TABLE	(2U)
#define RPCRDMA_DEF_SLOT_TABLE	(32U)
#define RPCRDMA_MAX_SLOT_TABLE	(256U)

#define RPCRDMA_DEF_INLINE  (1024)	/* default inline max */

#define RPCRDMA_INLINE_PAD_THRESH  (512)/* payload threshold to pad (bytes) */

/* memory registration strategies */
#define RPCRDMA_PERSISTENT_REGISTRATION (1)

enum rpcrdma_memreg {
	RPCRDMA_BOUNCEBUFFERS = 0,
	RPCRDMA_REGISTER,
	RPCRDMA_MEMWINDOWS,
	RPCRDMA_MEMWINDOWS_ASYNC,
	RPCRDMA_MTHCAFMR,
	RPCRDMA_FRMR,
	RPCRDMA_ALLPHYSICAL,
	RPCRDMA_LAST
};

#endif /* _LINUX_SUNRPC_XPRTRDMA_H */
