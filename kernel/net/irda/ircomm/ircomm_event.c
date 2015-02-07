

#include <linux/proc_fs.h>
#include <linux/init.h>

#include <net/irda/irda.h>
#include <net/irda/irlmp.h>
#include <net/irda/iriap.h>
#include <net/irda/irttp.h>
#include <net/irda/irias_object.h>

#include <net/irda/ircomm_core.h>
#include <net/irda/ircomm_event.h>

static int ircomm_state_idle(struct ircomm_cb *self, IRCOMM_EVENT event,
			     struct sk_buff *skb, struct ircomm_info *info);
static int ircomm_state_waiti(struct ircomm_cb *self, IRCOMM_EVENT event,
			      struct sk_buff *skb, struct ircomm_info *info);
static int ircomm_state_waitr(struct ircomm_cb *self, IRCOMM_EVENT event,
			      struct sk_buff *skb, struct ircomm_info *info);
static int ircomm_state_conn(struct ircomm_cb *self, IRCOMM_EVENT event,
			     struct sk_buff *skb, struct ircomm_info *info);

char *ircomm_state[] = {
	"IRCOMM_IDLE",
	"IRCOMM_WAITI",
	"IRCOMM_WAITR",
	"IRCOMM_CONN",
};

#ifdef CONFIG_IRDA_DEBUG
static char *ircomm_event[] = {
	"IRCOMM_CONNECT_REQUEST",
	"IRCOMM_CONNECT_RESPONSE",
	"IRCOMM_TTP_CONNECT_INDICATION",
	"IRCOMM_LMP_CONNECT_INDICATION",
	"IRCOMM_TTP_CONNECT_CONFIRM",
	"IRCOMM_LMP_CONNECT_CONFIRM",

	"IRCOMM_LMP_DISCONNECT_INDICATION",
	"IRCOMM_TTP_DISCONNECT_INDICATION",
	"IRCOMM_DISCONNECT_REQUEST",

	"IRCOMM_TTP_DATA_INDICATION",
	"IRCOMM_LMP_DATA_INDICATION",
	"IRCOMM_DATA_REQUEST",
	"IRCOMM_CONTROL_REQUEST",
	"IRCOMM_CONTROL_INDICATION",
};
#endif /* CONFIG_IRDA_DEBUG */

static int (*state[])(struct ircomm_cb *self, IRCOMM_EVENT event,
		      struct sk_buff *skb, struct ircomm_info *info) =
{
	ircomm_state_idle,
	ircomm_state_waiti,
	ircomm_state_waitr,
	ircomm_state_conn,
};

static int ircomm_state_idle(struct ircomm_cb *self, IRCOMM_EVENT event,
			     struct sk_buff *skb, struct ircomm_info *info)
{
	int ret = 0;

	switch (event) {
	case IRCOMM_CONNECT_REQUEST:
		ircomm_next_state(self, IRCOMM_WAITI);
		ret = self->issue.connect_request(self, skb, info);
		break;
	case IRCOMM_TTP_CONNECT_INDICATION:
	case IRCOMM_LMP_CONNECT_INDICATION:
		ircomm_next_state(self, IRCOMM_WAITR);
		ircomm_connect_indication(self, skb, info);
		break;
	default:
		IRDA_DEBUG(4, "%s(), unknown event: %s\n", __func__ ,
			   ircomm_event[event]);
		ret = -EINVAL;
	}
	return ret;
}

static int ircomm_state_waiti(struct ircomm_cb *self, IRCOMM_EVENT event,
			      struct sk_buff *skb, struct ircomm_info *info)
{
	int ret = 0;

	switch (event) {
	case IRCOMM_TTP_CONNECT_CONFIRM:
	case IRCOMM_LMP_CONNECT_CONFIRM:
		ircomm_next_state(self, IRCOMM_CONN);
		ircomm_connect_confirm(self, skb, info);
		break;
	case IRCOMM_TTP_DISCONNECT_INDICATION:
	case IRCOMM_LMP_DISCONNECT_INDICATION:
		ircomm_next_state(self, IRCOMM_IDLE);
		ircomm_disconnect_indication(self, skb, info);
		break;
	default:
		IRDA_DEBUG(0, "%s(), unknown event: %s\n", __func__ ,
			   ircomm_event[event]);
		ret = -EINVAL;
	}
	return ret;
}

static int ircomm_state_waitr(struct ircomm_cb *self, IRCOMM_EVENT event,
			      struct sk_buff *skb, struct ircomm_info *info)
{
	int ret = 0;

	switch (event) {
	case IRCOMM_CONNECT_RESPONSE:
		ircomm_next_state(self, IRCOMM_CONN);
		ret = self->issue.connect_response(self, skb);
		break;
	case IRCOMM_DISCONNECT_REQUEST:
		ircomm_next_state(self, IRCOMM_IDLE);
		ret = self->issue.disconnect_request(self, skb, info);
		break;
	case IRCOMM_TTP_DISCONNECT_INDICATION:
	case IRCOMM_LMP_DISCONNECT_INDICATION:
		ircomm_next_state(self, IRCOMM_IDLE);
		ircomm_disconnect_indication(self, skb, info);
		break;
	default:
		IRDA_DEBUG(0, "%s(), unknown event = %s\n", __func__ ,
			   ircomm_event[event]);
		ret = -EINVAL;
	}
	return ret;
}

static int ircomm_state_conn(struct ircomm_cb *self, IRCOMM_EVENT event,
			     struct sk_buff *skb, struct ircomm_info *info)
{
	int ret = 0;

	switch (event) {
	case IRCOMM_DATA_REQUEST:
		ret = self->issue.data_request(self, skb, 0);
		break;
	case IRCOMM_TTP_DATA_INDICATION:
		ircomm_process_data(self, skb);
		break;
	case IRCOMM_LMP_DATA_INDICATION:
		ircomm_data_indication(self, skb);
		break;
	case IRCOMM_CONTROL_REQUEST:
		/* Just send a separate frame for now */
		ret = self->issue.data_request(self, skb, skb->len);
		break;
	case IRCOMM_TTP_DISCONNECT_INDICATION:
	case IRCOMM_LMP_DISCONNECT_INDICATION:
		ircomm_next_state(self, IRCOMM_IDLE);
		ircomm_disconnect_indication(self, skb, info);
		break;
	case IRCOMM_DISCONNECT_REQUEST:
		ircomm_next_state(self, IRCOMM_IDLE);
		ret = self->issue.disconnect_request(self, skb, info);
		break;
	default:
		IRDA_DEBUG(0, "%s(), unknown event = %s\n", __func__ ,
			   ircomm_event[event]);
		ret = -EINVAL;
	}
	return ret;
}

int ircomm_do_event(struct ircomm_cb *self, IRCOMM_EVENT event,
		    struct sk_buff *skb, struct ircomm_info *info)
{
	IRDA_DEBUG(4, "%s: state=%s, event=%s\n", __func__ ,
		   ircomm_state[self->state], ircomm_event[event]);

	return (*state[self->state])(self, event, skb, info);
}

void ircomm_next_state(struct ircomm_cb *self, IRCOMM_STATE state)
{
	self->state = state;

	IRDA_DEBUG(4, "%s: next state=%s, service type=%d\n", __func__ ,
		   ircomm_state[self->state], self->service_type);
}