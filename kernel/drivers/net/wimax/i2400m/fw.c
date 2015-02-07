
#include <linux/firmware.h>
#include <linux/sched.h>
#include <linux/usb.h>
#include "i2400m.h"


#define D_SUBMODULE fw
#include "debug-levels.h"


static const __le32 i2400m_ACK_BARKER[4] = {
	__constant_cpu_to_le32(I2400M_ACK_BARKER),
	__constant_cpu_to_le32(I2400M_ACK_BARKER),
	__constant_cpu_to_le32(I2400M_ACK_BARKER),
	__constant_cpu_to_le32(I2400M_ACK_BARKER)
};


void i2400m_bm_cmd_prepare(struct i2400m_bootrom_header *cmd)
{
	if (i2400m_brh_get_use_checksum(cmd)) {
		int i;
		u32 checksum = 0;
		const u32 *checksum_ptr = (void *) cmd->payload;
		for (i = 0; i < cmd->data_size / 4; i++)
			checksum += cpu_to_le32(*checksum_ptr++);
		checksum += cmd->command + cmd->target_addr + cmd->data_size;
		cmd->block_checksum = cpu_to_le32(checksum);
	}
}
EXPORT_SYMBOL_GPL(i2400m_bm_cmd_prepare);


static
ssize_t __i2400m_bm_ack_verify(struct i2400m *i2400m, int opcode,
			       struct i2400m_bootrom_header *ack,
			       size_t ack_size, int flags)
{
	ssize_t result = -ENOMEM;
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(8, dev, "(i2400m %p opcode %d ack %p size %zu)\n",
		  i2400m, opcode, ack, ack_size);
	if (ack_size < sizeof(*ack)) {
		result = -EIO;
		dev_err(dev, "boot-mode cmd %d: HW BUG? notification didn't "
			"return enough data (%zu bytes vs %zu expected)\n",
			opcode, ack_size, sizeof(*ack));
		goto error_ack_short;
	}
	if (ack_size == sizeof(i2400m_NBOOT_BARKER)
		 && memcmp(ack, i2400m_NBOOT_BARKER, sizeof(*ack)) == 0) {
		result = -ERESTARTSYS;
		i2400m->sboot = 0;
		d_printf(6, dev, "boot-mode cmd %d: "
			 "HW non-signed boot barker\n", opcode);
		goto error_reboot;
	}
	if (ack_size == sizeof(i2400m_SBOOT_BARKER)
		 && memcmp(ack, i2400m_SBOOT_BARKER, sizeof(*ack)) == 0) {
		result = -ERESTARTSYS;
		i2400m->sboot = 1;
		d_printf(6, dev, "boot-mode cmd %d: HW signed reboot barker\n",
			 opcode);
		goto error_reboot;
	}
	if (ack_size == sizeof(i2400m_ACK_BARKER)
		 && memcmp(ack, i2400m_ACK_BARKER, sizeof(*ack)) == 0) {
		result = -EISCONN;
		d_printf(3, dev, "boot-mode cmd %d: HW reboot ack barker\n",
			 opcode);
		goto error_reboot_ack;
	}
	result = 0;
	if (flags & I2400M_BM_CMD_RAW)
		goto out_raw;
	ack->data_size = le32_to_cpu(ack->data_size);
	ack->target_addr = le32_to_cpu(ack->target_addr);
	ack->block_checksum = le32_to_cpu(ack->block_checksum);
	d_printf(5, dev, "boot-mode cmd %d: notification for opcode %u "
		 "response %u csum %u rr %u da %u\n",
		 opcode, i2400m_brh_get_opcode(ack),
		 i2400m_brh_get_response(ack),
		 i2400m_brh_get_use_checksum(ack),
		 i2400m_brh_get_response_required(ack),
		 i2400m_brh_get_direct_access(ack));
	result = -EIO;
	if (i2400m_brh_get_signature(ack) != 0xcbbc) {
		dev_err(dev, "boot-mode cmd %d: HW BUG? wrong signature "
			"0x%04x\n", opcode, i2400m_brh_get_signature(ack));
		goto error_ack_signature;
	}
	if (opcode != -1 && opcode != i2400m_brh_get_opcode(ack)) {
		dev_err(dev, "boot-mode cmd %d: HW BUG? "
			"received response for opcode %u, expected %u\n",
			opcode, i2400m_brh_get_opcode(ack), opcode);
		goto error_ack_opcode;
	}
	if (i2400m_brh_get_response(ack) != 0) {	/* failed? */
		dev_err(dev, "boot-mode cmd %d: error; hw response %u\n",
			opcode, i2400m_brh_get_response(ack));
		goto error_ack_failed;
	}
	if (ack_size < ack->data_size + sizeof(*ack)) {
		dev_err(dev, "boot-mode cmd %d: SW BUG "
			"driver provided only %zu bytes for %zu bytes "
			"of data\n", opcode, ack_size,
			(size_t) le32_to_cpu(ack->data_size) + sizeof(*ack));
		goto error_ack_short_buffer;
	}
	result = ack_size;
	/* Don't you love this stack of empty targets? Well, I don't
	 * either, but it helps track exactly who comes in here and
	 * why :) */
error_ack_short_buffer:
error_ack_failed:
error_ack_opcode:
error_ack_signature:
out_raw:
error_reboot_ack:
error_reboot:
error_ack_short:
	d_fnend(8, dev, "(i2400m %p opcode %d ack %p size %zu) = %d\n",
		i2400m, opcode, ack, ack_size, (int) result);
	return result;
}


static
ssize_t i2400m_bm_cmd(struct i2400m *i2400m,
		      const struct i2400m_bootrom_header *cmd, size_t cmd_size,
		      struct i2400m_bootrom_header *ack, size_t ack_size,
		      int flags)
{
	ssize_t result = -ENOMEM, rx_bytes;
	struct device *dev = i2400m_dev(i2400m);
	int opcode = cmd == NULL ? -1 : i2400m_brh_get_opcode(cmd);

	d_fnstart(6, dev, "(i2400m %p cmd %p size %zu ack %p size %zu)\n",
		  i2400m, cmd, cmd_size, ack, ack_size);
	BUG_ON(ack_size < sizeof(*ack));
	BUG_ON(i2400m->boot_mode == 0);

	if (cmd != NULL) {		/* send the command */
		memcpy(i2400m->bm_cmd_buf, cmd, cmd_size);
		result = i2400m->bus_bm_cmd_send(i2400m, cmd, cmd_size, flags);
		if (result < 0)
			goto error_cmd_send;
		if ((flags & I2400M_BM_CMD_RAW) == 0)
			d_printf(5, dev,
				 "boot-mode cmd %d csum %u rr %u da %u: "
				 "addr 0x%04x size %u block csum 0x%04x\n",
				 opcode, i2400m_brh_get_use_checksum(cmd),
				 i2400m_brh_get_response_required(cmd),
				 i2400m_brh_get_direct_access(cmd),
				 cmd->target_addr, cmd->data_size,
				 cmd->block_checksum);
	}
	result = i2400m->bus_bm_wait_for_ack(i2400m, ack, ack_size);
	if (result < 0) {
		dev_err(dev, "boot-mode cmd %d: error waiting for an ack: %d\n",
			opcode, (int) result);	/* bah, %zd doesn't work */
		goto error_wait_for_ack;
	}
	rx_bytes = result;
	/* verify the ack and read more if neccessary [result is the
	 * final amount of bytes we get in the ack]  */
	result = __i2400m_bm_ack_verify(i2400m, opcode, ack, ack_size, flags);
	if (result < 0)
		goto error_bad_ack;
	/* Don't you love this stack of empty targets? Well, I don't
	 * either, but it helps track exactly who comes in here and
	 * why :) */
	result = rx_bytes;
error_bad_ack:
error_wait_for_ack:
error_cmd_send:
	d_fnend(6, dev, "(i2400m %p cmd %p size %zu ack %p size %zu) = %d\n",
		i2400m, cmd, cmd_size, ack, ack_size, (int) result);
	return result;
}


static int i2400m_download_chunk(struct i2400m *i2400m, const void *chunk,
				 size_t __chunk_len, unsigned long addr,
				 unsigned int direct, unsigned int do_csum)
{
	int ret;
	size_t chunk_len = ALIGN(__chunk_len, I2400M_PL_PAD);
	struct device *dev = i2400m_dev(i2400m);
	struct {
		struct i2400m_bootrom_header cmd;
		u8 cmd_payload[chunk_len];
	} __attribute__((packed)) *buf;
	struct i2400m_bootrom_header ack;

	d_fnstart(5, dev, "(i2400m %p chunk %p __chunk_len %zu addr 0x%08lx "
		  "direct %u do_csum %u)\n", i2400m, chunk, __chunk_len,
		  addr, direct, do_csum);
	buf = i2400m->bm_cmd_buf;
	memcpy(buf->cmd_payload, chunk, __chunk_len);
	memset(buf->cmd_payload + __chunk_len, 0xad, chunk_len - __chunk_len);

	buf->cmd.command = i2400m_brh_command(I2400M_BRH_WRITE,
					      __chunk_len & 0x3 ? 0 : do_csum,
					      __chunk_len & 0xf ? 0 : direct);
	buf->cmd.target_addr = cpu_to_le32(addr);
	buf->cmd.data_size = cpu_to_le32(__chunk_len);
	ret = i2400m_bm_cmd(i2400m, &buf->cmd, sizeof(buf->cmd) + chunk_len,
			    &ack, sizeof(ack), 0);
	if (ret >= 0)
		ret = 0;
	d_fnend(5, dev, "(i2400m %p chunk %p __chunk_len %zu addr 0x%08lx "
		"direct %u do_csum %u) = %d\n", i2400m, chunk, __chunk_len,
		addr, direct, do_csum, ret);
	return ret;
}


static
ssize_t i2400m_dnload_bcf(struct i2400m *i2400m,
			  const struct i2400m_bcf_hdr *bcf, size_t bcf_len)
{
	ssize_t ret;
	struct device *dev = i2400m_dev(i2400m);
	size_t offset,		/* iterator offset */
		data_size,	/* Size of the data payload */
		section_size,	/* Size of the whole section (cmd + payload) */
		section = 1;
	const struct i2400m_bootrom_header *bh;
	struct i2400m_bootrom_header ack;

	d_fnstart(3, dev, "(i2400m %p bcf %p bcf_len %zu)\n",
		  i2400m, bcf, bcf_len);
	/* Iterate over the command blocks in the BCF file that start
	 * after the header */
	offset = le32_to_cpu(bcf->header_len) * sizeof(u32);
	while (1) {	/* start sending the file */
		bh = (void *) bcf + offset;
		data_size = le32_to_cpu(bh->data_size);
		section_size = ALIGN(sizeof(*bh) + data_size, 4);
		d_printf(7, dev,
			 "downloading section #%zu (@%zu %zu B) to 0x%08x\n",
			 section, offset, sizeof(*bh) + data_size,
			 le32_to_cpu(bh->target_addr));
		if (i2400m_brh_get_opcode(bh) == I2400M_BRH_SIGNED_JUMP) {
			/* Secure boot needs to stop here */
			d_printf(5, dev,  "signed jump found @%zu\n", offset);
			break;
		}
		if (offset + section_size == bcf_len)
			/* Non-secure boot stops here */
			break;
		if (offset + section_size > bcf_len) {
			dev_err(dev, "fw %s: bad section #%zu, "
				"end (@%zu) beyond EOF (@%zu)\n",
				i2400m->bus_fw_name, section,
				offset + section_size,  bcf_len);
			ret = -EINVAL;
			goto error_section_beyond_eof;
		}
		__i2400m_msleep(20);
		ret = i2400m_bm_cmd(i2400m, bh, section_size,
				    &ack, sizeof(ack), I2400M_BM_CMD_RAW);
		if (ret < 0) {
			dev_err(dev, "fw %s: section #%zu (@%zu %zu B) "
				"failed %d\n", i2400m->bus_fw_name, section,
				offset, sizeof(*bh) + data_size, (int) ret);
			goto error_send;
		}
		offset += section_size;
		section++;
	}
	ret = offset;
error_section_beyond_eof:
error_send:
	d_fnend(3, dev, "(i2400m %p bcf %p bcf_len %zu) = %d\n",
		i2400m, bcf, bcf_len, (int) ret);
	return ret;
}


static
int i2400m_dnload_finalize(struct i2400m *i2400m,
			   const struct i2400m_bcf_hdr *bcf, size_t offset)
{
	int ret = 0;
	struct device *dev = i2400m_dev(i2400m);
	struct i2400m_bootrom_header *cmd, ack;
	struct {
		struct i2400m_bootrom_header cmd;
		u8 cmd_pl[0];
	} __attribute__((packed)) *cmd_buf;
	size_t signature_block_offset, signature_block_size;

	d_fnstart(3, dev, "offset %zu\n", offset);
	cmd = (void *) bcf + offset;
	if (i2400m->sboot == 0) {
		struct i2400m_bootrom_header jump_ack;
		d_printf(3, dev, "unsecure boot, jumping to 0x%08x\n",
			le32_to_cpu(cmd->target_addr));
		i2400m_brh_set_opcode(cmd, I2400M_BRH_JUMP);
		cmd->data_size = 0;
		ret = i2400m_bm_cmd(i2400m, cmd, sizeof(*cmd),
				    &jump_ack, sizeof(jump_ack), 0);
	} else {
		d_printf(3, dev, "secure boot, jumping to 0x%08x\n",
			 le32_to_cpu(cmd->target_addr));
		cmd_buf = i2400m->bm_cmd_buf;
		memcpy(&cmd_buf->cmd, cmd, sizeof(*cmd));
		signature_block_offset =
			sizeof(*bcf)
			+ le32_to_cpu(bcf->key_size) * sizeof(u32)
			+ le32_to_cpu(bcf->exponent_size) * sizeof(u32);
		signature_block_size =
			le32_to_cpu(bcf->modulus_size) * sizeof(u32);
		memcpy(cmd_buf->cmd_pl, (void *) bcf + signature_block_offset,
		       signature_block_size);
		ret = i2400m_bm_cmd(i2400m, &cmd_buf->cmd,
				    sizeof(cmd_buf->cmd) + signature_block_size,
				    &ack, sizeof(ack), I2400M_BM_CMD_RAW);
	}
	d_fnend(3, dev, "returning %d\n", ret);
	return ret;
}


int i2400m_bootrom_init(struct i2400m *i2400m, enum i2400m_bri flags)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct i2400m_bootrom_header *cmd;
	struct i2400m_bootrom_header ack;
	int count = I2400M_BOOT_RETRIES;
	int ack_timeout_cnt = 1;

	BUILD_BUG_ON(sizeof(*cmd) != sizeof(i2400m_NBOOT_BARKER));
	BUILD_BUG_ON(sizeof(ack) != sizeof(i2400m_ACK_BARKER));

	d_fnstart(4, dev, "(i2400m %p flags 0x%08x)\n", i2400m, flags);
	result = -ENOMEM;
	cmd = i2400m->bm_cmd_buf;
	if (flags & I2400M_BRI_SOFT)
		goto do_reboot_ack;
do_reboot:
	if (--count < 0)
		goto error_timeout;
	d_printf(4, dev, "device reboot: reboot command [%d # left]\n",
		 count);
	if ((flags & I2400M_BRI_NO_REBOOT) == 0)
		i2400m->bus_reset(i2400m, I2400M_RT_WARM);
	result = i2400m_bm_cmd(i2400m, NULL, 0, &ack, sizeof(ack),
			       I2400M_BM_CMD_RAW);
	flags &= ~I2400M_BRI_NO_REBOOT;
	switch (result) {
	case -ERESTARTSYS:
		d_printf(4, dev, "device reboot: got reboot barker\n");
		break;
	case -EISCONN:	/* we don't know how it got here...but we follow it */
		d_printf(4, dev, "device reboot: got ack barker - whatever\n");
		goto do_reboot;
	case -ETIMEDOUT:	/* device has timed out, we might be in boot
				 * mode already and expecting an ack, let's try
				 * that */
		dev_info(dev, "warm reset timed out, trying an ack\n");
		goto do_reboot_ack;
	case -EPROTO:
	case -ESHUTDOWN:	/* dev is gone */
	case -EINTR:		/* user cancelled */
		goto error_dev_gone;
	default:
		dev_err(dev, "device reboot: error %d while waiting "
			"for reboot barker - rebooting\n", result);
		goto do_reboot;
	}
	/* At this point we ack back with 4 REBOOT barkers and expect
	 * 4 ACK barkers. This is ugly, as we send a raw command --
	 * hence the cast. _bm_cmd() will catch the reboot ack
	 * notification and report it as -EISCONN. */
do_reboot_ack:
	d_printf(4, dev, "device reboot ack: sending ack [%d # left]\n", count);
	if (i2400m->sboot == 0)
		memcpy(cmd, i2400m_NBOOT_BARKER,
		       sizeof(i2400m_NBOOT_BARKER));
	else
		memcpy(cmd, i2400m_SBOOT_BARKER,
		       sizeof(i2400m_SBOOT_BARKER));
	result = i2400m_bm_cmd(i2400m, cmd, sizeof(*cmd),
			       &ack, sizeof(ack), I2400M_BM_CMD_RAW);
	switch (result) {
	case -ERESTARTSYS:
		d_printf(4, dev, "reboot ack: got reboot barker - retrying\n");
		if (--count < 0)
			goto error_timeout;
		goto do_reboot_ack;
	case -EISCONN:
		d_printf(4, dev, "reboot ack: got ack barker - good\n");
		break;
	case -ETIMEDOUT:	/* no response, maybe it is the other type? */
		if (ack_timeout_cnt-- >= 0) {
			d_printf(4, dev, "reboot ack timedout: "
				 "trying the other type?\n");
			i2400m->sboot = !i2400m->sboot;
			goto do_reboot_ack;
		} else {
			dev_err(dev, "reboot ack timedout too long: "
				"trying reboot\n");
			goto do_reboot;
		}
		break;
	case -EPROTO:
	case -ESHUTDOWN:	/* dev is gone */
		goto error_dev_gone;
	default:
		dev_err(dev, "device reboot ack: error %d while waiting for "
			"reboot ack barker - rebooting\n", result);
		goto do_reboot;
	}
	d_printf(2, dev, "device reboot ack: got ack barker - boot done\n");
	result = 0;
exit_timeout:
error_dev_gone:
	d_fnend(4, dev, "(i2400m %p flags 0x%08x) = %d\n",
		i2400m, flags, result);
	return result;

error_timeout:
	dev_err(dev, "Timed out waiting for reboot ack, resetting\n");
	i2400m->bus_reset(i2400m, I2400M_RT_BUS);
	result = -ETIMEDOUT;
	goto exit_timeout;
}


int i2400m_read_mac_addr(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct net_device *net_dev = i2400m->wimax_dev.net_dev;
	struct i2400m_bootrom_header *cmd;
	struct {
		struct i2400m_bootrom_header ack;
		u8 ack_pl[16];
	} __attribute__((packed)) ack_buf;

	d_fnstart(5, dev, "(i2400m %p)\n", i2400m);
	cmd = i2400m->bm_cmd_buf;
	cmd->command = i2400m_brh_command(I2400M_BRH_READ, 0, 1);
	cmd->target_addr = cpu_to_le32(0x00203fe8);
	cmd->data_size = cpu_to_le32(6);
	result = i2400m_bm_cmd(i2400m, cmd, sizeof(*cmd),
			       &ack_buf.ack, sizeof(ack_buf), 0);
	if (result < 0) {
		dev_err(dev, "BM: read mac addr failed: %d\n", result);
		goto error_read_mac;
	}
	d_printf(2, dev,
		 "mac addr is %02x:%02x:%02x:%02x:%02x:%02x\n",
		 ack_buf.ack_pl[0], ack_buf.ack_pl[1],
		 ack_buf.ack_pl[2], ack_buf.ack_pl[3],
		 ack_buf.ack_pl[4], ack_buf.ack_pl[5]);
	if (i2400m->bus_bm_mac_addr_impaired == 1) {
		ack_buf.ack_pl[0] = 0x00;
		ack_buf.ack_pl[1] = 0x16;
		ack_buf.ack_pl[2] = 0xd3;
		get_random_bytes(&ack_buf.ack_pl[3], 3);
		dev_err(dev, "BM is MAC addr impaired, faking MAC addr to "
			"mac addr is %02x:%02x:%02x:%02x:%02x:%02x\n",
			ack_buf.ack_pl[0], ack_buf.ack_pl[1],
			ack_buf.ack_pl[2], ack_buf.ack_pl[3],
			ack_buf.ack_pl[4], ack_buf.ack_pl[5]);
		result = 0;
	}
	net_dev->addr_len = ETH_ALEN;
	memcpy(net_dev->perm_addr, ack_buf.ack_pl, ETH_ALEN);
	memcpy(net_dev->dev_addr, ack_buf.ack_pl, ETH_ALEN);
error_read_mac:
	d_fnend(5, dev, "(i2400m %p) = %d\n", i2400m, result);
	return result;
}


static
int i2400m_dnload_init_nonsigned(struct i2400m *i2400m)
{
#define POKE(a, d) {					\
	.address = __constant_cpu_to_le32(a),		\
	.data = __constant_cpu_to_le32(d)		\
}
	static const struct {
		__le32 address;
		__le32 data;
	} i2400m_pokes[] = {
		POKE(0x081A58, 0xA7810230),
		POKE(0x080040, 0x00000000),
		POKE(0x080048, 0x00000082),
		POKE(0x08004C, 0x0000081F),
		POKE(0x080054, 0x00000085),
		POKE(0x080058, 0x00000180),
		POKE(0x08005C, 0x00000018),
		POKE(0x080060, 0x00000010),
		POKE(0x080574, 0x00000001),
		POKE(0x080550, 0x00000005),
		POKE(0xAE0000, 0x00000000),
	};
#undef POKE
	unsigned i;
	int ret;
	struct device *dev = i2400m_dev(i2400m);

	dev_warn(dev, "WARNING!!! non-signed boot UNTESTED PATH!\n");

	d_fnstart(5, dev, "(i2400m %p)\n", i2400m);
	for (i = 0; i < ARRAY_SIZE(i2400m_pokes); i++) {
		ret = i2400m_download_chunk(i2400m, &i2400m_pokes[i].data,
					    sizeof(i2400m_pokes[i].data),
					    i2400m_pokes[i].address, 1, 1);
		if (ret < 0)
			break;
	}
	d_fnend(5, dev, "(i2400m %p) = %d\n", i2400m, ret);
	return ret;
}


static
int i2400m_dnload_init_signed(struct i2400m *i2400m,
			      const struct i2400m_bcf_hdr *bcf_hdr)
{
	int ret;
	struct device *dev = i2400m_dev(i2400m);
	struct {
		struct i2400m_bootrom_header cmd;
		struct i2400m_bcf_hdr cmd_pl;
	} __attribute__((packed)) *cmd_buf;
	struct i2400m_bootrom_header ack;

	d_fnstart(5, dev, "(i2400m %p bcf_hdr %p)\n", i2400m, bcf_hdr);
	cmd_buf = i2400m->bm_cmd_buf;
	cmd_buf->cmd.command =
		i2400m_brh_command(I2400M_BRH_HASH_PAYLOAD_ONLY, 0, 0);
	cmd_buf->cmd.target_addr = 0;
	cmd_buf->cmd.data_size = cpu_to_le32(sizeof(cmd_buf->cmd_pl));
	memcpy(&cmd_buf->cmd_pl, bcf_hdr, sizeof(*bcf_hdr));
	ret = i2400m_bm_cmd(i2400m, &cmd_buf->cmd, sizeof(*cmd_buf),
			    &ack, sizeof(ack), 0);
	if (ret >= 0)
		ret = 0;
	d_fnend(5, dev, "(i2400m %p bcf_hdr %p) = %d\n", i2400m, bcf_hdr, ret);
	return ret;
}


static
int i2400m_dnload_init(struct i2400m *i2400m, const struct i2400m_bcf_hdr *bcf)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	u32 module_id = le32_to_cpu(bcf->module_id);

	if (i2400m->sboot == 0
	    && (module_id & I2400M_BCF_MOD_ID_POKES) == 0) {
		/* non-signed boot process without pokes */
		result = i2400m_dnload_init_nonsigned(i2400m);
		if (result == -ERESTARTSYS)
			return result;
		if (result < 0)
			dev_err(dev, "fw %s: non-signed download "
				"initialization failed: %d\n",
				i2400m->bus_fw_name, result);
	} else if (i2400m->sboot == 0
		 && (module_id & I2400M_BCF_MOD_ID_POKES)) {
		/* non-signed boot process with pokes, nothing to do */
		result = 0;
	} else {		 /* signed boot process */
		result = i2400m_dnload_init_signed(i2400m, bcf);
		if (result == -ERESTARTSYS)
			return result;
		if (result < 0)
			dev_err(dev, "fw %s: signed boot download "
				"initialization failed: %d\n",
				i2400m->bus_fw_name, result);
	}
	return result;
}


static
int i2400m_fw_check(struct i2400m *i2400m,
		    const struct i2400m_bcf_hdr *bcf,
		    size_t bcf_size)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	unsigned module_type, header_len, major_version, minor_version,
		module_id, module_vendor, date, size;

	/* Check hard errors */
	result = -EINVAL;
	if (bcf_size < sizeof(*bcf)) {	/* big enough header? */
		dev_err(dev, "firmware %s too short: "
			"%zu B vs %zu (at least) expected\n",
			i2400m->bus_fw_name, bcf_size, sizeof(*bcf));
		goto error;
	}

	module_type = bcf->module_type;
	header_len = sizeof(u32) * le32_to_cpu(bcf->header_len);
	major_version = le32_to_cpu(bcf->header_version) & 0xffff0000 >> 16;
	minor_version = le32_to_cpu(bcf->header_version) & 0x0000ffff;
	module_id = le32_to_cpu(bcf->module_id);
	module_vendor = le32_to_cpu(bcf->module_vendor);
	date = le32_to_cpu(bcf->date);
	size = sizeof(u32) * le32_to_cpu(bcf->size);

	if (bcf_size != size) {		/* annoyingly paranoid */
		dev_err(dev, "firmware %s: bad size, got "
			"%zu B vs %u expected\n",
			i2400m->bus_fw_name, bcf_size, size);
		goto error;
	}

	d_printf(2, dev, "type 0x%x id 0x%x vendor 0x%x; header v%u.%u (%zu B) "
		 "date %08x (%zu B)\n",
		 module_type, module_id, module_vendor,
		 major_version, minor_version, (size_t) header_len,
		 date, (size_t) size);

	if (module_type != 6) {		/* built for the right hardware? */
		dev_err(dev, "bad fw %s: unexpected module type 0x%x; "
			"aborting\n", i2400m->bus_fw_name, module_type);
		goto error;
	}

	/* Check soft-er errors */
	result = 0;
	if (module_vendor != 0x8086)
		dev_err(dev, "bad fw %s? unexpected vendor 0x%04x\n",
			i2400m->bus_fw_name, module_vendor);
	if (date < 0x20080300)
		dev_err(dev, "bad fw %s? build date too old %08x\n",
			i2400m->bus_fw_name, date);
error:
	return result;
}


static
int i2400m_fw_dnload(struct i2400m *i2400m, const struct i2400m_bcf_hdr *bcf,
		     size_t bcf_size, enum i2400m_bri flags)
{
	int ret = 0;
	struct device *dev = i2400m_dev(i2400m);
	int count = I2400M_BOOT_RETRIES;

	d_fnstart(5, dev, "(i2400m %p bcf %p size %zu)\n",
		  i2400m, bcf, bcf_size);
	i2400m->boot_mode = 1;
hw_reboot:
	if (count-- == 0) {
		ret = -ERESTARTSYS;
		dev_err(dev, "device rebooted too many times, aborting\n");
		goto error_too_many_reboots;
	}
	if (flags & I2400M_BRI_MAC_REINIT) {
		ret = i2400m_bootrom_init(i2400m, flags);
		if (ret < 0) {
			dev_err(dev, "bootrom init failed: %d\n", ret);
			goto error_bootrom_init;
		}
	}
	flags |= I2400M_BRI_MAC_REINIT;

	/*
	 * Initialize the download, push the bytes to the device and
	 * then jump to the new firmware. Note @ret is passed with the
	 * offset of the jump instruction to _dnload_finalize()
	 */
	ret = i2400m_dnload_init(i2400m, bcf);	/* Init device's dnload */
	if (ret == -ERESTARTSYS)
		goto error_dev_rebooted;
	if (ret < 0)
		goto error_dnload_init;

	ret = i2400m_dnload_bcf(i2400m, bcf, bcf_size);
	if (ret == -ERESTARTSYS)
		goto error_dev_rebooted;
	if (ret < 0) {
		dev_err(dev, "fw %s: download failed: %d\n",
			i2400m->bus_fw_name, ret);
		goto error_dnload_bcf;
	}

	ret = i2400m_dnload_finalize(i2400m, bcf, ret);
	if (ret == -ERESTARTSYS)
		goto error_dev_rebooted;
	if (ret < 0) {
		dev_err(dev, "fw %s: "
			"download finalization failed: %d\n",
			i2400m->bus_fw_name, ret);
		goto error_dnload_finalize;
	}

	d_printf(2, dev, "fw %s successfully uploaded\n",
		 i2400m->bus_fw_name);
	i2400m->boot_mode = 0;
error_dnload_finalize:
error_dnload_bcf:
error_dnload_init:
error_bootrom_init:
error_too_many_reboots:
	d_fnend(5, dev, "(i2400m %p bcf %p size %zu) = %d\n",
		i2400m, bcf, bcf_size, ret);
	return ret;

error_dev_rebooted:
	dev_err(dev, "device rebooted, %d tries left\n", count);
	/* we got the notification already, no need to wait for it again */
	flags |= I2400M_BRI_SOFT;
	goto hw_reboot;
}


int i2400m_dev_bootstrap(struct i2400m *i2400m, enum i2400m_bri flags)
{
	int ret = 0;
	struct device *dev = i2400m_dev(i2400m);
	const struct firmware *fw;
	const struct i2400m_bcf_hdr *bcf;	/* Firmware data */

	d_fnstart(5, dev, "(i2400m %p)\n", i2400m);
	/* Load firmware files to memory. */
	ret = request_firmware(&fw, i2400m->bus_fw_name, dev);
	if (ret) {
		dev_err(dev, "fw %s: request failed: %d\n",
			i2400m->bus_fw_name, ret);
		goto error_fw_req;
	}
	bcf = (void *) fw->data;

	ret = i2400m_fw_check(i2400m, bcf, fw->size);
	if (ret < 0)
		goto error_fw_bad;
	ret = i2400m_fw_dnload(i2400m, bcf, fw->size, flags);
error_fw_bad:
	release_firmware(fw);
error_fw_req:
	d_fnend(5, dev, "(i2400m %p) = %d\n", i2400m, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(i2400m_dev_bootstrap);
