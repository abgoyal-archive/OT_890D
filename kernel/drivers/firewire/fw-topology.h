

#ifndef __fw_topology_h
#define __fw_topology_h

enum {
	FW_NODE_CREATED,
	FW_NODE_UPDATED,
	FW_NODE_DESTROYED,
	FW_NODE_LINK_ON,
	FW_NODE_LINK_OFF,
	FW_NODE_INITIATED_RESET,
};

struct fw_node {
	u16 node_id;
	u8 color;
	u8 port_count;
	u8 link_on : 1;
	u8 initiated_reset : 1;
	u8 b_path : 1;
	u8 phy_speed : 2; /* As in the self ID packet. */
	u8 max_speed : 2; /* Minimum of all phy-speeds on the path from the
			   * local node to this node. */
	u8 max_depth : 4; /* Maximum depth to any leaf node */
	u8 max_hops : 4;  /* Max hops in this sub tree */
	atomic_t ref_count;

	/* For serializing node topology into a list. */
	struct list_head link;

	/* Upper layer specific data. */
	void *data;

	struct fw_node *ports[0];
};

static inline struct fw_node *
fw_node_get(struct fw_node *node)
{
	atomic_inc(&node->ref_count);

	return node;
}

static inline void
fw_node_put(struct fw_node *node)
{
	if (atomic_dec_and_test(&node->ref_count))
		kfree(node);
}

void
fw_destroy_nodes(struct fw_card *card);

int
fw_compute_block_crc(u32 *block);


#endif /* __fw_topology_h */
