

#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/sunrpc/sched.h>
#include <linux/sunrpc/clnt.h>

#include <linux/nfs.h>
#include <linux/nfs4.h>
#include <linux/nfs_fs.h>
#include "nfs4_fs.h"
#include "delegation.h"

#define NFSDBG_FACILITY	NFSDBG_PROC

void
nfs4_renew_state(struct work_struct *work)
{
	struct nfs_client *clp =
		container_of(work, struct nfs_client, cl_renewd.work);
	struct rpc_cred *cred;
	long lease, timeout;
	unsigned long last, now;

	dprintk("%s: start\n", __func__);
	/* Are there any active superblocks? */
	if (list_empty(&clp->cl_superblocks))
		goto out;
	spin_lock(&clp->cl_lock);
	lease = clp->cl_lease_time;
	last = clp->cl_last_renewal;
	now = jiffies;
	timeout = (2 * lease) / 3 + (long)last - (long)now;
	/* Are we close to a lease timeout? */
	if (time_after(now, last + lease/3)) {
		cred = nfs4_get_renew_cred_locked(clp);
		spin_unlock(&clp->cl_lock);
		if (cred == NULL) {
			if (list_empty(&clp->cl_delegations)) {
				set_bit(NFS4CLNT_LEASE_EXPIRED, &clp->cl_state);
				goto out;
			}
			nfs_expire_all_delegations(clp);
		} else {
			/* Queue an asynchronous RENEW. */
			nfs4_proc_async_renew(clp, cred);
			put_rpccred(cred);
		}
		timeout = (2 * lease) / 3;
		spin_lock(&clp->cl_lock);
	} else
		dprintk("%s: failed to call renewd. Reason: lease not expired \n",
				__func__);
	if (timeout < 5 * HZ)    /* safeguard */
		timeout = 5 * HZ;
	dprintk("%s: requeueing work. Lease period = %ld\n",
			__func__, (timeout + HZ - 1) / HZ);
	cancel_delayed_work(&clp->cl_renewd);
	schedule_delayed_work(&clp->cl_renewd, timeout);
	spin_unlock(&clp->cl_lock);
	nfs_expire_unreferenced_delegations(clp);
out:
	dprintk("%s: done\n", __func__);
}

void
nfs4_schedule_state_renewal(struct nfs_client *clp)
{
	long timeout;

	spin_lock(&clp->cl_lock);
	timeout = (2 * clp->cl_lease_time) / 3 + (long)clp->cl_last_renewal
		- (long)jiffies;
	if (timeout < 5 * HZ)
		timeout = 5 * HZ;
	dprintk("%s: requeueing work. Lease period = %ld\n",
			__func__, (timeout + HZ - 1) / HZ);
	cancel_delayed_work(&clp->cl_renewd);
	schedule_delayed_work(&clp->cl_renewd, timeout);
	set_bit(NFS_CS_RENEWD, &clp->cl_res_state);
	spin_unlock(&clp->cl_lock);
}

void
nfs4_renewd_prepare_shutdown(struct nfs_server *server)
{
	cancel_delayed_work(&server->nfs_client->cl_renewd);
}

void
nfs4_kill_renewd(struct nfs_client *clp)
{
	cancel_delayed_work_sync(&clp->cl_renewd);
}

