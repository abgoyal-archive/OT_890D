

#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/poll.h>
#include <net/sock.h>
#include <net/tcp_states.h>

#include <linux/phonet.h>
#include <net/phonet/phonet.h>
#include <net/phonet/pep.h>
#include <net/phonet/pn_dev.h>

static int pn_socket_release(struct socket *sock)
{
	struct sock *sk = sock->sk;

	if (sk) {
		sock->sk = NULL;
		sk->sk_prot->close(sk, 0);
	}
	return 0;
}

static struct  {
	struct hlist_head hlist;
	spinlock_t lock;
} pnsocks = {
	.hlist = HLIST_HEAD_INIT,
	.lock = __SPIN_LOCK_UNLOCKED(pnsocks.lock),
};

struct sock *pn_find_sock_by_sa(struct net *net, const struct sockaddr_pn *spn)
{
	struct hlist_node *node;
	struct sock *sknode;
	struct sock *rval = NULL;
	u16 obj = pn_sockaddr_get_object(spn);
	u8 res = spn->spn_resource;

	spin_lock_bh(&pnsocks.lock);

	sk_for_each(sknode, node, &pnsocks.hlist) {
		struct pn_sock *pn = pn_sk(sknode);
		BUG_ON(!pn->sobject); /* unbound socket */

		if (!net_eq(sock_net(sknode), net))
			continue;
		if (pn_port(obj)) {
			/* Look up socket by port */
			if (pn_port(pn->sobject) != pn_port(obj))
				continue;
		} else {
			/* If port is zero, look up by resource */
			if (pn->resource != res)
				continue;
		}
		if (pn_addr(pn->sobject)
		 && pn_addr(pn->sobject) != pn_addr(obj))
			continue;

		rval = sknode;
		sock_hold(sknode);
		break;
	}

	spin_unlock_bh(&pnsocks.lock);

	return rval;

}

void pn_sock_hash(struct sock *sk)
{
	spin_lock_bh(&pnsocks.lock);
	sk_add_node(sk, &pnsocks.hlist);
	spin_unlock_bh(&pnsocks.lock);
}
EXPORT_SYMBOL(pn_sock_hash);

void pn_sock_unhash(struct sock *sk)
{
	spin_lock_bh(&pnsocks.lock);
	sk_del_node_init(sk);
	spin_unlock_bh(&pnsocks.lock);
}
EXPORT_SYMBOL(pn_sock_unhash);

static int pn_socket_bind(struct socket *sock, struct sockaddr *addr, int len)
{
	struct sock *sk = sock->sk;
	struct pn_sock *pn = pn_sk(sk);
	struct sockaddr_pn *spn = (struct sockaddr_pn *)addr;
	int err;
	u16 handle;
	u8 saddr;

	if (sk->sk_prot->bind)
		return sk->sk_prot->bind(sk, addr, len);

	if (len < sizeof(struct sockaddr_pn))
		return -EINVAL;
	if (spn->spn_family != AF_PHONET)
		return -EAFNOSUPPORT;

	handle = pn_sockaddr_get_object((struct sockaddr_pn *)addr);
	saddr = pn_addr(handle);
	if (saddr && phonet_address_lookup(sock_net(sk), saddr))
		return -EADDRNOTAVAIL;

	lock_sock(sk);
	if (sk->sk_state != TCP_CLOSE || pn_port(pn->sobject)) {
		err = -EINVAL; /* attempt to rebind */
		goto out;
	}
	err = sk->sk_prot->get_port(sk, pn_port(handle));
	if (err)
		goto out;

	/* get_port() sets the port, bind() sets the address if applicable */
	pn->sobject = pn_object(saddr, pn_port(pn->sobject));
	pn->resource = spn->spn_resource;

	/* Enable RX on the socket */
	sk->sk_prot->hash(sk);
out:
	release_sock(sk);
	return err;
}

static int pn_socket_autobind(struct socket *sock)
{
	struct sockaddr_pn sa;
	int err;

	memset(&sa, 0, sizeof(sa));
	sa.spn_family = AF_PHONET;
	err = pn_socket_bind(sock, (struct sockaddr *)&sa,
				sizeof(struct sockaddr_pn));
	if (err != -EINVAL)
		return err;
	BUG_ON(!pn_port(pn_sk(sock->sk)->sobject));
	return 0; /* socket was already bound */
}

static int pn_socket_accept(struct socket *sock, struct socket *newsock,
				int flags)
{
	struct sock *sk = sock->sk;
	struct sock *newsk;
	int err;

	newsk = sk->sk_prot->accept(sk, flags, &err);
	if (!newsk)
		return err;

	lock_sock(newsk);
	sock_graft(newsk, newsock);
	newsock->state = SS_CONNECTED;
	release_sock(newsk);
	return 0;
}

static int pn_socket_getname(struct socket *sock, struct sockaddr *addr,
				int *sockaddr_len, int peer)
{
	struct sock *sk = sock->sk;
	struct pn_sock *pn = pn_sk(sk);

	memset(addr, 0, sizeof(struct sockaddr_pn));
	addr->sa_family = AF_PHONET;
	if (!peer) /* Race with bind() here is userland's problem. */
		pn_sockaddr_set_object((struct sockaddr_pn *)addr,
					pn->sobject);

	*sockaddr_len = sizeof(struct sockaddr_pn);
	return 0;
}

static unsigned int pn_socket_poll(struct file *file, struct socket *sock,
					poll_table *wait)
{
	struct sock *sk = sock->sk;
	struct pep_sock *pn = pep_sk(sk);
	unsigned int mask = 0;

	poll_wait(file, &sock->wait, wait);

	switch (sk->sk_state) {
	case TCP_LISTEN:
		return hlist_empty(&pn->ackq) ? 0 : POLLIN;
	case TCP_CLOSE:
		return POLLERR;
	}

	if (!skb_queue_empty(&sk->sk_receive_queue))
		mask |= POLLIN | POLLRDNORM;
	if (!skb_queue_empty(&pn->ctrlreq_queue))
		mask |= POLLPRI;
	if (!mask && sk->sk_state == TCP_CLOSE_WAIT)
		return POLLHUP;

	if (sk->sk_state == TCP_ESTABLISHED && atomic_read(&pn->tx_credits))
		mask |= POLLOUT | POLLWRNORM | POLLWRBAND;

	return mask;
}

static int pn_socket_ioctl(struct socket *sock, unsigned int cmd,
				unsigned long arg)
{
	struct sock *sk = sock->sk;
	struct pn_sock *pn = pn_sk(sk);

	if (cmd == SIOCPNGETOBJECT) {
		struct net_device *dev;
		u16 handle;
		u8 saddr;

		if (get_user(handle, (__u16 __user *)arg))
			return -EFAULT;

		lock_sock(sk);
		if (sk->sk_bound_dev_if)
			dev = dev_get_by_index(sock_net(sk),
						sk->sk_bound_dev_if);
		else
			dev = phonet_device_get(sock_net(sk));
		if (dev && (dev->flags & IFF_UP))
			saddr = phonet_address_get(dev, pn_addr(handle));
		else
			saddr = PN_NO_ADDR;
		release_sock(sk);

		if (dev)
			dev_put(dev);
		if (saddr == PN_NO_ADDR)
			return -EHOSTUNREACH;

		handle = pn_object(saddr, pn_port(pn->sobject));
		return put_user(handle, (__u16 __user *)arg);
	}

	return sk->sk_prot->ioctl(sk, cmd, arg);
}

static int pn_socket_listen(struct socket *sock, int backlog)
{
	struct sock *sk = sock->sk;
	int err = 0;

	if (sock->state != SS_UNCONNECTED)
		return -EINVAL;
	if (pn_socket_autobind(sock))
		return -ENOBUFS;

	lock_sock(sk);
	if (sk->sk_state != TCP_CLOSE) {
		err = -EINVAL;
		goto out;
	}

	sk->sk_state = TCP_LISTEN;
	sk->sk_ack_backlog = 0;
	sk->sk_max_ack_backlog = backlog;
out:
	release_sock(sk);
	return err;
}

static int pn_socket_sendmsg(struct kiocb *iocb, struct socket *sock,
				struct msghdr *m, size_t total_len)
{
	struct sock *sk = sock->sk;

	if (pn_socket_autobind(sock))
		return -EAGAIN;

	return sk->sk_prot->sendmsg(iocb, sk, m, total_len);
}

const struct proto_ops phonet_dgram_ops = {
	.family		= AF_PHONET,
	.owner		= THIS_MODULE,
	.release	= pn_socket_release,
	.bind		= pn_socket_bind,
	.connect	= sock_no_connect,
	.socketpair	= sock_no_socketpair,
	.accept		= sock_no_accept,
	.getname	= pn_socket_getname,
	.poll		= datagram_poll,
	.ioctl		= pn_socket_ioctl,
	.listen		= sock_no_listen,
	.shutdown	= sock_no_shutdown,
	.setsockopt	= sock_no_setsockopt,
	.getsockopt	= sock_no_getsockopt,
#ifdef CONFIG_COMPAT
	.compat_setsockopt = sock_no_setsockopt,
	.compat_getsockopt = sock_no_getsockopt,
#endif
	.sendmsg	= pn_socket_sendmsg,
	.recvmsg	= sock_common_recvmsg,
	.mmap		= sock_no_mmap,
	.sendpage	= sock_no_sendpage,
};

const struct proto_ops phonet_stream_ops = {
	.family		= AF_PHONET,
	.owner		= THIS_MODULE,
	.release	= pn_socket_release,
	.bind		= pn_socket_bind,
	.connect	= sock_no_connect,
	.socketpair	= sock_no_socketpair,
	.accept		= pn_socket_accept,
	.getname	= pn_socket_getname,
	.poll		= pn_socket_poll,
	.ioctl		= pn_socket_ioctl,
	.listen		= pn_socket_listen,
	.shutdown	= sock_no_shutdown,
	.setsockopt	= sock_common_setsockopt,
	.getsockopt	= sock_common_getsockopt,
#ifdef CONFIG_COMPAT
	.compat_setsockopt = compat_sock_common_setsockopt,
	.compat_getsockopt = compat_sock_common_getsockopt,
#endif
	.sendmsg	= pn_socket_sendmsg,
	.recvmsg	= sock_common_recvmsg,
	.mmap		= sock_no_mmap,
	.sendpage	= sock_no_sendpage,
};
EXPORT_SYMBOL(phonet_stream_ops);

static DEFINE_MUTEX(port_mutex);

/* allocate port for a socket */
int pn_sock_get_port(struct sock *sk, unsigned short sport)
{
	static int port_cur;
	struct net *net = sock_net(sk);
	struct pn_sock *pn = pn_sk(sk);
	struct sockaddr_pn try_sa;
	struct sock *tmpsk;

	memset(&try_sa, 0, sizeof(struct sockaddr_pn));
	try_sa.spn_family = AF_PHONET;

	mutex_lock(&port_mutex);

	if (!sport) {
		/* search free port */
		int port, pmin, pmax;

		phonet_get_local_port_range(&pmin, &pmax);
		for (port = pmin; port <= pmax; port++) {
			port_cur++;
			if (port_cur < pmin || port_cur > pmax)
				port_cur = pmin;

			pn_sockaddr_set_port(&try_sa, port_cur);
			tmpsk = pn_find_sock_by_sa(net, &try_sa);
			if (tmpsk == NULL) {
				sport = port_cur;
				goto found;
			} else
				sock_put(tmpsk);
		}
	} else {
		/* try to find specific port */
		pn_sockaddr_set_port(&try_sa, sport);
		tmpsk = pn_find_sock_by_sa(net, &try_sa);
		if (tmpsk == NULL)
			/* No sock there! We can use that port... */
			goto found;
		else
			sock_put(tmpsk);
	}
	mutex_unlock(&port_mutex);

	/* the port must be in use already */
	return -EADDRINUSE;

found:
	mutex_unlock(&port_mutex);
	pn->sobject = pn_object(pn_addr(pn->sobject), sport);
	return 0;
}
EXPORT_SYMBOL(pn_sock_get_port);
