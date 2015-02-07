


#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/sock.h>
#include <net/netlabel.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "objsec.h"
#include "security.h"
#include "netlabel.h"

static int selinux_netlbl_sidlookup_cached(struct sk_buff *skb,
					   struct netlbl_lsm_secattr *secattr,
					   u32 *sid)
{
	int rc;

	rc = security_netlbl_secattr_to_sid(secattr, sid);
	if (rc == 0 &&
	    (secattr->flags & NETLBL_SECATTR_CACHEABLE) &&
	    (secattr->flags & NETLBL_SECATTR_CACHE))
		netlbl_cache_add(skb, secattr);

	return rc;
}

static struct netlbl_lsm_secattr *selinux_netlbl_sock_genattr(struct sock *sk)
{
	int rc;
	struct sk_security_struct *sksec = sk->sk_security;
	struct netlbl_lsm_secattr *secattr;

	if (sksec->nlbl_secattr != NULL)
		return sksec->nlbl_secattr;

	secattr = netlbl_secattr_alloc(GFP_ATOMIC);
	if (secattr == NULL)
		return NULL;
	rc = security_netlbl_sid_to_secattr(sksec->sid, secattr);
	if (rc != 0) {
		netlbl_secattr_free(secattr);
		return NULL;
	}
	sksec->nlbl_secattr = secattr;

	return secattr;
}

static int selinux_netlbl_sock_setsid(struct sock *sk)
{
	int rc;
	struct sk_security_struct *sksec = sk->sk_security;
	struct netlbl_lsm_secattr *secattr;

	if (sksec->nlbl_state != NLBL_REQUIRE)
		return 0;

	secattr = selinux_netlbl_sock_genattr(sk);
	if (secattr == NULL)
		return -ENOMEM;
	rc = netlbl_sock_setattr(sk, secattr);
	switch (rc) {
	case 0:
		sksec->nlbl_state = NLBL_LABELED;
		break;
	case -EDESTADDRREQ:
		sksec->nlbl_state = NLBL_REQSKB;
		rc = 0;
		break;
	}

	return rc;
}

void selinux_netlbl_cache_invalidate(void)
{
	netlbl_cache_invalidate();
}

void selinux_netlbl_err(struct sk_buff *skb, int error, int gateway)
{
	netlbl_skbuff_err(skb, error, gateway);
}

void selinux_netlbl_sk_security_free(struct sk_security_struct *ssec)
{
	if (ssec->nlbl_secattr != NULL)
		netlbl_secattr_free(ssec->nlbl_secattr);
}

void selinux_netlbl_sk_security_reset(struct sk_security_struct *ssec,
				      int family)
{
	if (family == PF_INET)
		ssec->nlbl_state = NLBL_REQUIRE;
	else
		ssec->nlbl_state = NLBL_UNSET;
}

int selinux_netlbl_skbuff_getsid(struct sk_buff *skb,
				 u16 family,
				 u32 *type,
				 u32 *sid)
{
	int rc;
	struct netlbl_lsm_secattr secattr;

	if (!netlbl_enabled()) {
		*sid = SECSID_NULL;
		return 0;
	}

	netlbl_secattr_init(&secattr);
	rc = netlbl_skbuff_getattr(skb, family, &secattr);
	if (rc == 0 && secattr.flags != NETLBL_SECATTR_NONE)
		rc = selinux_netlbl_sidlookup_cached(skb, &secattr, sid);
	else
		*sid = SECSID_NULL;
	*type = secattr.type;
	netlbl_secattr_destroy(&secattr);

	return rc;
}

int selinux_netlbl_skbuff_setsid(struct sk_buff *skb,
				 u16 family,
				 u32 sid)
{
	int rc;
	struct netlbl_lsm_secattr secattr_storage;
	struct netlbl_lsm_secattr *secattr = NULL;
	struct sock *sk;

	/* if this is a locally generated packet check to see if it is already
	 * being labeled by it's parent socket, if it is just exit */
	sk = skb->sk;
	if (sk != NULL) {
		struct sk_security_struct *sksec = sk->sk_security;
		if (sksec->nlbl_state != NLBL_REQSKB)
			return 0;
		secattr = sksec->nlbl_secattr;
	}
	if (secattr == NULL) {
		secattr = &secattr_storage;
		netlbl_secattr_init(secattr);
		rc = security_netlbl_sid_to_secattr(sid, secattr);
		if (rc != 0)
			goto skbuff_setsid_return;
	}

	rc = netlbl_skbuff_setattr(skb, family, secattr);

skbuff_setsid_return:
	if (secattr == &secattr_storage)
		netlbl_secattr_destroy(secattr);
	return rc;
}

void selinux_netlbl_inet_conn_established(struct sock *sk, u16 family)
{
	int rc;
	struct sk_security_struct *sksec = sk->sk_security;
	struct netlbl_lsm_secattr *secattr;
	struct inet_sock *sk_inet = inet_sk(sk);
	struct sockaddr_in addr;

	if (sksec->nlbl_state != NLBL_REQUIRE)
		return;

	secattr = selinux_netlbl_sock_genattr(sk);
	if (secattr == NULL)
		return;

	rc = netlbl_sock_setattr(sk, secattr);
	switch (rc) {
	case 0:
		sksec->nlbl_state = NLBL_LABELED;
		break;
	case -EDESTADDRREQ:
		/* no PF_INET6 support yet because we don't support any IPv6
		 * labeling protocols */
		if (family != PF_INET) {
			sksec->nlbl_state = NLBL_UNSET;
			return;
		}

		addr.sin_family = family;
		addr.sin_addr.s_addr = sk_inet->daddr;
		if (netlbl_conn_setattr(sk, (struct sockaddr *)&addr,
					secattr) != 0) {
			/* we failed to label the connected socket (could be
			 * for a variety of reasons, the actual "why" isn't
			 * important here) so we have to go to our backup plan,
			 * labeling the packets individually in the netfilter
			 * local output hook.  this is okay but we need to
			 * adjust the MSS of the connection to take into
			 * account any labeling overhead, since we don't know
			 * the exact overhead at this point we'll use the worst
			 * case value which is 40 bytes for IPv4 */
			struct inet_connection_sock *sk_conn = inet_csk(sk);
			sk_conn->icsk_ext_hdr_len += 40 -
				      (sk_inet->opt ? sk_inet->opt->optlen : 0);
			sk_conn->icsk_sync_mss(sk, sk_conn->icsk_pmtu_cookie);

			sksec->nlbl_state = NLBL_REQSKB;
		} else
			sksec->nlbl_state = NLBL_CONNLABELED;
		break;
	default:
		/* note that we are failing to label the socket which could be
		 * a bad thing since it means traffic could leave the system
		 * without the desired labeling, however, all is not lost as
		 * we have a check in selinux_netlbl_inode_permission() to
		 * pick up the pieces that we might drop here because we can't
		 * return an error code */
		break;
	}
}

int selinux_netlbl_socket_post_create(struct socket *sock)
{
	return selinux_netlbl_sock_setsid(sock->sk);
}

int selinux_netlbl_inode_permission(struct inode *inode, int mask)
{
	int rc;
	struct sock *sk;
	struct socket *sock;
	struct sk_security_struct *sksec;

	if (!S_ISSOCK(inode->i_mode) ||
	    ((mask & (MAY_WRITE | MAY_APPEND)) == 0))
		return 0;
	sock = SOCKET_I(inode);
	sk = sock->sk;
	if (sk == NULL)
		return 0;
	sksec = sk->sk_security;
	if (sksec == NULL || sksec->nlbl_state != NLBL_REQUIRE)
		return 0;

	local_bh_disable();
	bh_lock_sock_nested(sk);
	if (likely(sksec->nlbl_state == NLBL_REQUIRE))
		rc = selinux_netlbl_sock_setsid(sk);
	else
		rc = 0;
	bh_unlock_sock(sk);
	local_bh_enable();

	return rc;
}

int selinux_netlbl_sock_rcv_skb(struct sk_security_struct *sksec,
				struct sk_buff *skb,
				u16 family,
				struct avc_audit_data *ad)
{
	int rc;
	u32 nlbl_sid;
	u32 perm;
	struct netlbl_lsm_secattr secattr;

	if (!netlbl_enabled())
		return 0;

	netlbl_secattr_init(&secattr);
	rc = netlbl_skbuff_getattr(skb, family, &secattr);
	if (rc == 0 && secattr.flags != NETLBL_SECATTR_NONE)
		rc = selinux_netlbl_sidlookup_cached(skb, &secattr, &nlbl_sid);
	else
		nlbl_sid = SECINITSID_UNLABELED;
	netlbl_secattr_destroy(&secattr);
	if (rc != 0)
		return rc;

	switch (sksec->sclass) {
	case SECCLASS_UDP_SOCKET:
		perm = UDP_SOCKET__RECVFROM;
		break;
	case SECCLASS_TCP_SOCKET:
		perm = TCP_SOCKET__RECVFROM;
		break;
	default:
		perm = RAWIP_SOCKET__RECVFROM;
	}

	rc = avc_has_perm(sksec->sid, nlbl_sid, sksec->sclass, perm, ad);
	if (rc == 0)
		return 0;

	if (nlbl_sid != SECINITSID_UNLABELED)
		netlbl_skbuff_err(skb, rc, 0);
	return rc;
}

int selinux_netlbl_socket_setsockopt(struct socket *sock,
				     int level,
				     int optname)
{
	int rc = 0;
	struct sock *sk = sock->sk;
	struct sk_security_struct *sksec = sk->sk_security;
	struct netlbl_lsm_secattr secattr;

	if (level == IPPROTO_IP && optname == IP_OPTIONS &&
	    (sksec->nlbl_state == NLBL_LABELED ||
	     sksec->nlbl_state == NLBL_CONNLABELED)) {
		netlbl_secattr_init(&secattr);
		lock_sock(sk);
		rc = netlbl_sock_getattr(sk, &secattr);
		release_sock(sk);
		if (rc == 0)
			rc = -EACCES;
		else if (rc == -ENOMSG)
			rc = 0;
		netlbl_secattr_destroy(&secattr);
	}

	return rc;
}

int selinux_netlbl_socket_connect(struct sock *sk, struct sockaddr *addr)
{
	int rc;
	struct sk_security_struct *sksec = sk->sk_security;
	struct netlbl_lsm_secattr *secattr;

	if (sksec->nlbl_state != NLBL_REQSKB &&
	    sksec->nlbl_state != NLBL_CONNLABELED)
		return 0;

	local_bh_disable();
	bh_lock_sock_nested(sk);

	/* connected sockets are allowed to disconnect when the address family
	 * is set to AF_UNSPEC, if that is what is happening we want to reset
	 * the socket */
	if (addr->sa_family == AF_UNSPEC) {
		netlbl_sock_delattr(sk);
		sksec->nlbl_state = NLBL_REQSKB;
		rc = 0;
		goto socket_connect_return;
	}
	secattr = selinux_netlbl_sock_genattr(sk);
	if (secattr == NULL) {
		rc = -ENOMEM;
		goto socket_connect_return;
	}
	rc = netlbl_conn_setattr(sk, addr, secattr);
	if (rc == 0)
		sksec->nlbl_state = NLBL_CONNLABELED;

socket_connect_return:
	bh_unlock_sock(sk);
	local_bh_enable();
	return rc;
}
