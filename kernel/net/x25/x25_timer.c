

#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <net/sock.h>
#include <net/tcp_states.h>
#include <net/x25.h>

static void x25_heartbeat_expiry(unsigned long);
static void x25_timer_expiry(unsigned long);

void x25_init_timers(struct sock *sk)
{
	struct x25_sock *x25 = x25_sk(sk);

	setup_timer(&x25->timer, x25_timer_expiry, (unsigned long)sk);

	/* initialized by sock_init_data */
	sk->sk_timer.data     = (unsigned long)sk;
	sk->sk_timer.function = &x25_heartbeat_expiry;
}

void x25_start_heartbeat(struct sock *sk)
{
	mod_timer(&sk->sk_timer, jiffies + 5 * HZ);
}

void x25_stop_heartbeat(struct sock *sk)
{
	del_timer(&sk->sk_timer);
}

void x25_start_t2timer(struct sock *sk)
{
	struct x25_sock *x25 = x25_sk(sk);

	mod_timer(&x25->timer, jiffies + x25->t2);
}

void x25_start_t21timer(struct sock *sk)
{
	struct x25_sock *x25 = x25_sk(sk);

	mod_timer(&x25->timer, jiffies + x25->t21);
}

void x25_start_t22timer(struct sock *sk)
{
	struct x25_sock *x25 = x25_sk(sk);

	mod_timer(&x25->timer, jiffies + x25->t22);
}

void x25_start_t23timer(struct sock *sk)
{
	struct x25_sock *x25 = x25_sk(sk);

	mod_timer(&x25->timer, jiffies + x25->t23);
}

void x25_stop_timer(struct sock *sk)
{
	del_timer(&x25_sk(sk)->timer);
}

unsigned long x25_display_timer(struct sock *sk)
{
	struct x25_sock *x25 = x25_sk(sk);

	if (!timer_pending(&x25->timer))
		return 0;

	return x25->timer.expires - jiffies;
}

static void x25_heartbeat_expiry(unsigned long param)
{
	struct sock *sk = (struct sock *)param;

	bh_lock_sock(sk);
	if (sock_owned_by_user(sk)) /* can currently only occur in state 3 */
		goto restart_heartbeat;

	switch (x25_sk(sk)->state) {

		case X25_STATE_0:
			/*
			 * Magic here: If we listen() and a new link dies
			 * before it is accepted() it isn't 'dead' so doesn't
			 * get removed.
			 */
			if (sock_flag(sk, SOCK_DESTROY) ||
			    (sk->sk_state == TCP_LISTEN &&
			     sock_flag(sk, SOCK_DEAD))) {
				bh_unlock_sock(sk);
				x25_destroy_socket(sk);
				return;
			}
			break;

		case X25_STATE_3:
			/*
			 * Check for the state of the receive buffer.
			 */
			x25_check_rbuf(sk);
			break;
	}
restart_heartbeat:
	x25_start_heartbeat(sk);
	bh_unlock_sock(sk);
}

static inline void x25_do_timer_expiry(struct sock * sk)
{
	struct x25_sock *x25 = x25_sk(sk);

	switch (x25->state) {

		case X25_STATE_3:	/* T2 */
			if (x25->condition & X25_COND_ACK_PENDING) {
				x25->condition &= ~X25_COND_ACK_PENDING;
				x25_enquiry_response(sk);
			}
			break;

		case X25_STATE_1:	/* T21 */
		case X25_STATE_4:	/* T22 */
			x25_write_internal(sk, X25_CLEAR_REQUEST);
			x25->state = X25_STATE_2;
			x25_start_t23timer(sk);
			break;

		case X25_STATE_2:	/* T23 */
			x25_disconnect(sk, ETIMEDOUT, 0, 0);
			break;
	}
}

static void x25_timer_expiry(unsigned long param)
{
	struct sock *sk = (struct sock *)param;

	bh_lock_sock(sk);
	if (sock_owned_by_user(sk)) { /* can currently only occur in state 3 */
		if (x25_sk(sk)->state == X25_STATE_3)
			x25_start_t2timer(sk);
	} else
		x25_do_timer_expiry(sk);
	bh_unlock_sock(sk);
}
