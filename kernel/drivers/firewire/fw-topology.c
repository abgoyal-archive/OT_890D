

#include <linux/module.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <asm/bug.h>
#include <asm/system.h>
#include "fw-transaction.h"
#include "fw-topology.h"

#define SELF_ID_PHY_ID(q)		(((q) >> 24) & 0x3f)
#define SELF_ID_EXTENDED(q)		(((q) >> 23) & 0x01)
#define SELF_ID_LINK_ON(q)		(((q) >> 22) & 0x01)
#define SELF_ID_GAP_COUNT(q)		(((q) >> 16) & 0x3f)
#define SELF_ID_PHY_SPEED(q)		(((q) >> 14) & 0x03)
#define SELF_ID_CONTENDER(q)		(((q) >> 11) & 0x01)
#define SELF_ID_PHY_INITIATOR(q)	(((q) >>  1) & 0x01)
#define SELF_ID_MORE_PACKETS(q)		(((q) >>  0) & 0x01)

#define SELF_ID_EXT_SEQUENCE(q)		(((q) >> 20) & 0x07)

static u32 *count_ports(u32 *sid, int *total_port_count, int *child_port_count)
{
	u32 q;
	int port_type, shift, seq;

	*total_port_count = 0;
	*child_port_count = 0;

	shift = 6;
	q = *sid;
	seq = 0;

	while (1) {
		port_type = (q >> shift) & 0x03;
		switch (port_type) {
		case SELFID_PORT_CHILD:
			(*child_port_count)++;
		case SELFID_PORT_PARENT:
		case SELFID_PORT_NCONN:
			(*total_port_count)++;
		case SELFID_PORT_NONE:
			break;
		}

		shift -= 2;
		if (shift == 0) {
			if (!SELF_ID_MORE_PACKETS(q))
				return sid + 1;

			shift = 16;
			sid++;
			q = *sid;

			/*
			 * Check that the extra packets actually are
			 * extended self ID packets and that the
			 * sequence numbers in the extended self ID
			 * packets increase as expected.
			 */

			if (!SELF_ID_EXTENDED(q) ||
			    seq != SELF_ID_EXT_SEQUENCE(q))
				return NULL;

			seq++;
		}
	}
}

static int get_port_type(u32 *sid, int port_index)
{
	int index, shift;

	index = (port_index + 5) / 8;
	shift = 16 - ((port_index + 5) & 7) * 2;
	return (sid[index] >> shift) & 0x03;
}

static struct fw_node *fw_node_create(u32 sid, int port_count, int color)
{
	struct fw_node *node;

	node = kzalloc(sizeof(*node) + port_count * sizeof(node->ports[0]),
		       GFP_ATOMIC);
	if (node == NULL)
		return NULL;

	node->color = color;
	node->node_id = LOCAL_BUS | SELF_ID_PHY_ID(sid);
	node->link_on = SELF_ID_LINK_ON(sid);
	node->phy_speed = SELF_ID_PHY_SPEED(sid);
	node->initiated_reset = SELF_ID_PHY_INITIATOR(sid);
	node->port_count = port_count;

	atomic_set(&node->ref_count, 1);
	INIT_LIST_HEAD(&node->link);

	return node;
}

static void update_hop_count(struct fw_node *node)
{
	int depths[2] = { -1, -1 };
	int max_child_hops = 0;
	int i;

	for (i = 0; i < node->port_count; i++) {
		if (node->ports[i] == NULL)
			continue;

		if (node->ports[i]->max_hops > max_child_hops)
			max_child_hops = node->ports[i]->max_hops;

		if (node->ports[i]->max_depth > depths[0]) {
			depths[1] = depths[0];
			depths[0] = node->ports[i]->max_depth;
		} else if (node->ports[i]->max_depth > depths[1])
			depths[1] = node->ports[i]->max_depth;
	}

	node->max_depth = depths[0] + 1;
	node->max_hops = max(max_child_hops, depths[0] + depths[1] + 2);
}

static inline struct fw_node *fw_node(struct list_head *l)
{
	return list_entry(l, struct fw_node, link);
}

static struct fw_node *build_tree(struct fw_card *card,
				  u32 *sid, int self_id_count)
{
	struct fw_node *node, *child, *local_node, *irm_node;
	struct list_head stack, *h;
	u32 *next_sid, *end, q;
	int i, port_count, child_port_count, phy_id, parent_count, stack_depth;
	int gap_count;
	bool beta_repeaters_present;

	local_node = NULL;
	node = NULL;
	INIT_LIST_HEAD(&stack);
	stack_depth = 0;
	end = sid + self_id_count;
	phy_id = 0;
	irm_node = NULL;
	gap_count = SELF_ID_GAP_COUNT(*sid);
	beta_repeaters_present = false;

	while (sid < end) {
		next_sid = count_ports(sid, &port_count, &child_port_count);

		if (next_sid == NULL) {
			fw_error("Inconsistent extended self IDs.\n");
			return NULL;
		}

		q = *sid;
		if (phy_id != SELF_ID_PHY_ID(q)) {
			fw_error("PHY ID mismatch in self ID: %d != %d.\n",
				 phy_id, SELF_ID_PHY_ID(q));
			return NULL;
		}

		if (child_port_count > stack_depth) {
			fw_error("Topology stack underflow\n");
			return NULL;
		}

		/*
		 * Seek back from the top of our stack to find the
		 * start of the child nodes for this node.
		 */
		for (i = 0, h = &stack; i < child_port_count; i++)
			h = h->prev;
		/*
		 * When the stack is empty, this yields an invalid value,
		 * but that pointer will never be dereferenced.
		 */
		child = fw_node(h);

		node = fw_node_create(q, port_count, card->color);
		if (node == NULL) {
			fw_error("Out of memory while building topology.\n");
			return NULL;
		}

		if (phy_id == (card->node_id & 0x3f))
			local_node = node;

		if (SELF_ID_CONTENDER(q))
			irm_node = node;

		parent_count = 0;

		for (i = 0; i < port_count; i++) {
			switch (get_port_type(sid, i)) {
			case SELFID_PORT_PARENT:
				/*
				 * Who's your daddy?  We dont know the
				 * parent node at this time, so we
				 * temporarily abuse node->color for
				 * remembering the entry in the
				 * node->ports array where the parent
				 * node should be.  Later, when we
				 * handle the parent node, we fix up
				 * the reference.
				 */
				parent_count++;
				node->color = i;
				break;

			case SELFID_PORT_CHILD:
				node->ports[i] = child;
				/*
				 * Fix up parent reference for this
				 * child node.
				 */
				child->ports[child->color] = node;
				child->color = card->color;
				child = fw_node(child->link.next);
				break;
			}
		}

		/*
		 * Check that the node reports exactly one parent
		 * port, except for the root, which of course should
		 * have no parents.
		 */
		if ((next_sid == end && parent_count != 0) ||
		    (next_sid < end && parent_count != 1)) {
			fw_error("Parent port inconsistency for node %d: "
				 "parent_count=%d\n", phy_id, parent_count);
			return NULL;
		}

		/* Pop the child nodes off the stack and push the new node. */
		__list_del(h->prev, &stack);
		list_add_tail(&node->link, &stack);
		stack_depth += 1 - child_port_count;

		if (node->phy_speed == SCODE_BETA &&
		    parent_count + child_port_count > 1)
			beta_repeaters_present = true;

		/*
		 * If PHYs report different gap counts, set an invalid count
		 * which will force a gap count reconfiguration and a reset.
		 */
		if (SELF_ID_GAP_COUNT(q) != gap_count)
			gap_count = 0;

		update_hop_count(node);

		sid = next_sid;
		phy_id++;
	}

	card->root_node = node;
	card->irm_node = irm_node;
	card->gap_count = gap_count;
	card->beta_repeaters_present = beta_repeaters_present;

	return local_node;
}

typedef void (*fw_node_callback_t)(struct fw_card * card,
				   struct fw_node * node,
				   struct fw_node * parent);

static void
for_each_fw_node(struct fw_card *card, struct fw_node *root,
		 fw_node_callback_t callback)
{
	struct list_head list;
	struct fw_node *node, *next, *child, *parent;
	int i;

	INIT_LIST_HEAD(&list);

	fw_node_get(root);
	list_add_tail(&root->link, &list);
	parent = NULL;
	list_for_each_entry(node, &list, link) {
		node->color = card->color;

		for (i = 0; i < node->port_count; i++) {
			child = node->ports[i];
			if (!child)
				continue;
			if (child->color == card->color)
				parent = child;
			else {
				fw_node_get(child);
				list_add_tail(&child->link, &list);
			}
		}

		callback(card, node, parent);
	}

	list_for_each_entry_safe(node, next, &list, link)
		fw_node_put(node);
}

static void
report_lost_node(struct fw_card *card,
		 struct fw_node *node, struct fw_node *parent)
{
	fw_node_event(card, node, FW_NODE_DESTROYED);
	fw_node_put(node);

	/* Topology has changed - reset bus manager retry counter */
	card->bm_retries = 0;
}

static void
report_found_node(struct fw_card *card,
		  struct fw_node *node, struct fw_node *parent)
{
	int b_path = (node->phy_speed == SCODE_BETA);

	if (parent != NULL) {
		/* min() macro doesn't work here with gcc 3.4 */
		node->max_speed = parent->max_speed < node->phy_speed ?
					parent->max_speed : node->phy_speed;
		node->b_path = parent->b_path && b_path;
	} else {
		node->max_speed = node->phy_speed;
		node->b_path = b_path;
	}

	fw_node_event(card, node, FW_NODE_CREATED);

	/* Topology has changed - reset bus manager retry counter */
	card->bm_retries = 0;
}

void fw_destroy_nodes(struct fw_card *card)
{
	unsigned long flags;

	spin_lock_irqsave(&card->lock, flags);
	card->color++;
	if (card->local_node != NULL)
		for_each_fw_node(card, card->local_node, report_lost_node);
	card->local_node = NULL;
	spin_unlock_irqrestore(&card->lock, flags);
}

static void move_tree(struct fw_node *node0, struct fw_node *node1, int port)
{
	struct fw_node *tree;
	int i;

	tree = node1->ports[port];
	node0->ports[port] = tree;
	for (i = 0; i < tree->port_count; i++) {
		if (tree->ports[i] == node1) {
			tree->ports[i] = node0;
			break;
		}
	}
}

static void
update_tree(struct fw_card *card, struct fw_node *root)
{
	struct list_head list0, list1;
	struct fw_node *node0, *node1, *next1;
	int i, event;

	INIT_LIST_HEAD(&list0);
	list_add_tail(&card->local_node->link, &list0);
	INIT_LIST_HEAD(&list1);
	list_add_tail(&root->link, &list1);

	node0 = fw_node(list0.next);
	node1 = fw_node(list1.next);

	while (&node0->link != &list0) {
		WARN_ON(node0->port_count != node1->port_count);

		if (node0->link_on && !node1->link_on)
			event = FW_NODE_LINK_OFF;
		else if (!node0->link_on && node1->link_on)
			event = FW_NODE_LINK_ON;
		else if (node1->initiated_reset && node1->link_on)
			event = FW_NODE_INITIATED_RESET;
		else
			event = FW_NODE_UPDATED;

		node0->node_id = node1->node_id;
		node0->color = card->color;
		node0->link_on = node1->link_on;
		node0->initiated_reset = node1->initiated_reset;
		node0->max_hops = node1->max_hops;
		node1->color = card->color;
		fw_node_event(card, node0, event);

		if (card->root_node == node1)
			card->root_node = node0;
		if (card->irm_node == node1)
			card->irm_node = node0;

		for (i = 0; i < node0->port_count; i++) {
			if (node0->ports[i] && node1->ports[i]) {
				/*
				 * This port didn't change, queue the
				 * connected node for further
				 * investigation.
				 */
				if (node0->ports[i]->color == card->color)
					continue;
				list_add_tail(&node0->ports[i]->link, &list0);
				list_add_tail(&node1->ports[i]->link, &list1);
			} else if (node0->ports[i]) {
				/*
				 * The nodes connected here were
				 * unplugged; unref the lost nodes and
				 * queue FW_NODE_LOST callbacks for
				 * them.
				 */

				for_each_fw_node(card, node0->ports[i],
						 report_lost_node);
				node0->ports[i] = NULL;
			} else if (node1->ports[i]) {
				/*
				 * One or more node were connected to
				 * this port. Move the new nodes into
				 * the tree and queue FW_NODE_CREATED
				 * callbacks for them.
				 */
				move_tree(node0, node1, i);
				for_each_fw_node(card, node0->ports[i],
						 report_found_node);
			}
		}

		node0 = fw_node(node0->link.next);
		next1 = fw_node(node1->link.next);
		fw_node_put(node1);
		node1 = next1;
	}
}

static void
update_topology_map(struct fw_card *card, u32 *self_ids, int self_id_count)
{
	int node_count;

	card->topology_map[1]++;
	node_count = (card->root_node->node_id & 0x3f) + 1;
	card->topology_map[2] = (node_count << 16) | self_id_count;
	card->topology_map[0] = (self_id_count + 2) << 16;
	memcpy(&card->topology_map[3], self_ids, self_id_count * 4);
	fw_compute_block_crc(card->topology_map);
}

void
fw_core_handle_bus_reset(struct fw_card *card,
			 int node_id, int generation,
			 int self_id_count, u32 * self_ids)
{
	struct fw_node *local_node;
	unsigned long flags;

	/*
	 * If the selfID buffer is not the immediate successor of the
	 * previously processed one, we cannot reliably compare the
	 * old and new topologies.
	 */
	if (!is_next_generation(generation, card->generation) &&
	    card->local_node != NULL) {
		fw_notify("skipped bus generations, destroying all nodes\n");
		fw_destroy_nodes(card);
		card->bm_retries = 0;
	}

	spin_lock_irqsave(&card->lock, flags);

	card->node_id = node_id;
	/*
	 * Update node_id before generation to prevent anybody from using
	 * a stale node_id together with a current generation.
	 */
	smp_wmb();
	card->generation = generation;
	card->reset_jiffies = jiffies;
	fw_schedule_bm_work(card, 0);

	local_node = build_tree(card, self_ids, self_id_count);

	update_topology_map(card, self_ids, self_id_count);

	card->color++;

	if (local_node == NULL) {
		fw_error("topology build failed\n");
		/* FIXME: We need to issue a bus reset in this case. */
	} else if (card->local_node == NULL) {
		card->local_node = local_node;
		for_each_fw_node(card, local_node, report_found_node);
	} else {
		update_tree(card, local_node);
	}

	spin_unlock_irqrestore(&card->lock, flags);
}
EXPORT_SYMBOL(fw_core_handle_bus_reset);