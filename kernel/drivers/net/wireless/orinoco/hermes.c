

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "hermes.h"

MODULE_DESCRIPTION("Low-level driver helper for Lucent Hermes chipset and Prism II HFA384x wireless MAC controller");
MODULE_AUTHOR("Pavel Roskin <proski@gnu.org>"
	" & David Gibson <hermes@gibson.dropbear.id.au>");
MODULE_LICENSE("Dual MPL/GPL");

/* These are maximum timeouts. Most often, card wil react much faster */
#define CMD_BUSY_TIMEOUT (100) /* In iterations of ~1us */
#define CMD_INIT_TIMEOUT (50000) /* in iterations of ~10us */
#define CMD_COMPL_TIMEOUT (20000) /* in iterations of ~10us */
#define ALLOC_COMPL_TIMEOUT (1000) /* in iterations of ~10us */


#define DMSG(stuff...) do {printk(KERN_DEBUG "hermes @ %p: " , hw->iobase); \
			printk(stuff);} while (0)

#undef HERMES_DEBUG
#ifdef HERMES_DEBUG
#include <stdarg.h>

#define DEBUG(lvl, stuff...) if ( (lvl) <= HERMES_DEBUG) DMSG(stuff)

#else /* ! HERMES_DEBUG */

#define DEBUG(lvl, stuff...) do { } while (0)

#endif /* ! HERMES_DEBUG */



static int hermes_issue_cmd(hermes_t *hw, u16 cmd, u16 param0,
			    u16 param1, u16 param2)
{
	int k = CMD_BUSY_TIMEOUT;
	u16 reg;

	/* First wait for the command register to unbusy */
	reg = hermes_read_regn(hw, CMD);
	while ( (reg & HERMES_CMD_BUSY) && k ) {
		k--;
		udelay(1);
		reg = hermes_read_regn(hw, CMD);
	}
	if (reg & HERMES_CMD_BUSY) {
		return -EBUSY;
	}

	hermes_write_regn(hw, PARAM2, param2);
	hermes_write_regn(hw, PARAM1, param1);
	hermes_write_regn(hw, PARAM0, param0);
	hermes_write_regn(hw, CMD, cmd);
	
	return 0;
}


/* For doing cmds that wipe the magic constant in SWSUPPORT0 */
int hermes_doicmd_wait(hermes_t *hw, u16 cmd,
		       u16 parm0, u16 parm1, u16 parm2,
		       struct hermes_response *resp)
{
	int err = 0;
	int k;
	u16 status, reg;

	err = hermes_issue_cmd(hw, cmd, parm0, parm1, parm2);
	if (err)
		return err;

	reg = hermes_read_regn(hw, EVSTAT);
	k = CMD_INIT_TIMEOUT;
	while ((!(reg & HERMES_EV_CMD)) && k) {
		k--;
		udelay(10);
		reg = hermes_read_regn(hw, EVSTAT);
	}

	hermes_write_regn(hw, SWSUPPORT0, HERMES_MAGIC);

	if (!hermes_present(hw)) {
		DEBUG(0, "hermes @ 0x%x: Card removed during reset.\n",
		       hw->iobase);
		err = -ENODEV;
		goto out;
	}

	if (!(reg & HERMES_EV_CMD)) {
		printk(KERN_ERR "hermes @ %p: "
		       "Timeout waiting for card to reset (reg=0x%04x)!\n",
		       hw->iobase, reg);
		err = -ETIMEDOUT;
		goto out;
	}

	status = hermes_read_regn(hw, STATUS);
	if (resp) {
		resp->status = status;
		resp->resp0 = hermes_read_regn(hw, RESP0);
		resp->resp1 = hermes_read_regn(hw, RESP1);
		resp->resp2 = hermes_read_regn(hw, RESP2);
	}

	hermes_write_regn(hw, EVACK, HERMES_EV_CMD);

	if (status & HERMES_STATUS_RESULT)
		err = -EIO;
out:
	return err;
}
EXPORT_SYMBOL(hermes_doicmd_wait);

void hermes_struct_init(hermes_t *hw, void __iomem *address, int reg_spacing)
{
	hw->iobase = address;
	hw->reg_spacing = reg_spacing;
	hw->inten = 0x0;
}
EXPORT_SYMBOL(hermes_struct_init);

int hermes_init(hermes_t *hw)
{
	u16 reg;
	int err = 0;
	int k;

	/* We don't want to be interrupted while resetting the chipset */
	hw->inten = 0x0;
	hermes_write_regn(hw, INTEN, 0);
	hermes_write_regn(hw, EVACK, 0xffff);

	/* Normally it's a "can't happen" for the command register to
           be busy when we go to issue a command because we are
           serializing all commands.  However we want to have some
           chance of resetting the card even if it gets into a stupid
           state, so we actually wait to see if the command register
           will unbusy itself here. */
	k = CMD_BUSY_TIMEOUT;
	reg = hermes_read_regn(hw, CMD);
	while (k && (reg & HERMES_CMD_BUSY)) {
		if (reg == 0xffff) /* Special case - the card has probably been removed,
				      so don't wait for the timeout */
			return -ENODEV;

		k--;
		udelay(1);
		reg = hermes_read_regn(hw, CMD);
	}
	
	/* No need to explicitly handle the timeout - if we've timed
	   out hermes_issue_cmd() will probably return -EBUSY below */

	/* According to the documentation, EVSTAT may contain
	   obsolete event occurrence information.  We have to acknowledge
	   it by writing EVACK. */
	reg = hermes_read_regn(hw, EVSTAT);
	hermes_write_regn(hw, EVACK, reg);

	/* We don't use hermes_docmd_wait here, because the reset wipes
	   the magic constant in SWSUPPORT0 away, and it gets confused */
	err = hermes_doicmd_wait(hw, HERMES_CMD_INIT, 0, 0, 0, NULL);

	return err;
}
EXPORT_SYMBOL(hermes_init);

int hermes_docmd_wait(hermes_t *hw, u16 cmd, u16 parm0,
		      struct hermes_response *resp)
{
	int err;
	int k;
	u16 reg;
	u16 status;

	err = hermes_issue_cmd(hw, cmd, parm0, 0, 0);
	if (err) {
		if (! hermes_present(hw)) {
			if (net_ratelimit())
				printk(KERN_WARNING "hermes @ %p: "
				       "Card removed while issuing command "
				       "0x%04x.\n", hw->iobase, cmd);
			err = -ENODEV;
		} else 
			if (net_ratelimit())
				printk(KERN_ERR "hermes @ %p: "
				       "Error %d issuing command 0x%04x.\n",
				       hw->iobase, err, cmd);
		goto out;
	}

	reg = hermes_read_regn(hw, EVSTAT);
	k = CMD_COMPL_TIMEOUT;
	while ( (! (reg & HERMES_EV_CMD)) && k) {
		k--;
		udelay(10);
		reg = hermes_read_regn(hw, EVSTAT);
	}

	if (! hermes_present(hw)) {
		printk(KERN_WARNING "hermes @ %p: Card removed "
		       "while waiting for command 0x%04x completion.\n",
		       hw->iobase, cmd);
		err = -ENODEV;
		goto out;
	}
		
	if (! (reg & HERMES_EV_CMD)) {
		printk(KERN_ERR "hermes @ %p: Timeout waiting for "
		       "command 0x%04x completion.\n", hw->iobase, cmd);
		err = -ETIMEDOUT;
		goto out;
	}

	status = hermes_read_regn(hw, STATUS);
	if (resp) {
		resp->status = status;
		resp->resp0 = hermes_read_regn(hw, RESP0);
		resp->resp1 = hermes_read_regn(hw, RESP1);
		resp->resp2 = hermes_read_regn(hw, RESP2);
	}

	hermes_write_regn(hw, EVACK, HERMES_EV_CMD);

	if (status & HERMES_STATUS_RESULT)
		err = -EIO;

 out:
	return err;
}
EXPORT_SYMBOL(hermes_docmd_wait);

int hermes_allocate(hermes_t *hw, u16 size, u16 *fid)
{
	int err = 0;
	int k;
	u16 reg;
	
	if ( (size < HERMES_ALLOC_LEN_MIN) || (size > HERMES_ALLOC_LEN_MAX) )
		return -EINVAL;

	err = hermes_docmd_wait(hw, HERMES_CMD_ALLOC, size, NULL);
	if (err) {
		return err;
	}

	reg = hermes_read_regn(hw, EVSTAT);
	k = ALLOC_COMPL_TIMEOUT;
	while ( (! (reg & HERMES_EV_ALLOC)) && k) {
		k--;
		udelay(10);
		reg = hermes_read_regn(hw, EVSTAT);
	}
	
	if (! hermes_present(hw)) {
		printk(KERN_WARNING "hermes @ %p: "
		       "Card removed waiting for frame allocation.\n",
		       hw->iobase);
		return -ENODEV;
	}
		
	if (! (reg & HERMES_EV_ALLOC)) {
		printk(KERN_ERR "hermes @ %p: "
		       "Timeout waiting for frame allocation\n",
		       hw->iobase);
		return -ETIMEDOUT;
	}

	*fid = hermes_read_regn(hw, ALLOCFID);
	hermes_write_regn(hw, EVACK, HERMES_EV_ALLOC);
	
	return 0;
}
EXPORT_SYMBOL(hermes_allocate);

static int hermes_bap_seek(hermes_t *hw, int bap, u16 id, u16 offset)
{
	int sreg = bap ? HERMES_SELECT1 : HERMES_SELECT0;
	int oreg = bap ? HERMES_OFFSET1 : HERMES_OFFSET0;
	int k;
	u16 reg;

	/* Paranoia.. */
	if ( (offset > HERMES_BAP_OFFSET_MAX) || (offset % 2) )
		return -EINVAL;

	k = HERMES_BAP_BUSY_TIMEOUT;
	reg = hermes_read_reg(hw, oreg);
	while ((reg & HERMES_OFFSET_BUSY) && k) {
		k--;
		udelay(1);
		reg = hermes_read_reg(hw, oreg);
	}

	if (reg & HERMES_OFFSET_BUSY)
		return -ETIMEDOUT;

	/* Now we actually set up the transfer */
	hermes_write_reg(hw, sreg, id);
	hermes_write_reg(hw, oreg, offset);

	/* Wait for the BAP to be ready */
	k = HERMES_BAP_BUSY_TIMEOUT;
	reg = hermes_read_reg(hw, oreg);
	while ( (reg & (HERMES_OFFSET_BUSY | HERMES_OFFSET_ERR)) && k) {
		k--;
		udelay(1);
		reg = hermes_read_reg(hw, oreg);
	}

	if (reg != offset) {
		printk(KERN_ERR "hermes @ %p: BAP%d offset %s: "
		       "reg=0x%x id=0x%x offset=0x%x\n", hw->iobase, bap,
		       (reg & HERMES_OFFSET_BUSY) ? "timeout" : "error",
		       reg, id, offset);

		if (reg & HERMES_OFFSET_BUSY) {
			return -ETIMEDOUT;
		}

		return -EIO;		/* error or wrong offset */
	}

	return 0;
}

int hermes_bap_pread(hermes_t *hw, int bap, void *buf, int len,
		     u16 id, u16 offset)
{
	int dreg = bap ? HERMES_DATA1 : HERMES_DATA0;
	int err = 0;

	if ( (len < 0) || (len % 2) )
		return -EINVAL;

	err = hermes_bap_seek(hw, bap, id, offset);
	if (err)
		goto out;

	/* Actually do the transfer */
	hermes_read_words(hw, dreg, buf, len/2);

 out:
	return err;
}
EXPORT_SYMBOL(hermes_bap_pread);

int hermes_bap_pwrite(hermes_t *hw, int bap, const void *buf, int len,
		      u16 id, u16 offset)
{
	int dreg = bap ? HERMES_DATA1 : HERMES_DATA0;
	int err = 0;

	if (len < 0)
		return -EINVAL;

	err = hermes_bap_seek(hw, bap, id, offset);
	if (err)
		goto out;
	
	/* Actually do the transfer */
	hermes_write_bytes(hw, dreg, buf, len);

 out:	
	return err;
}
EXPORT_SYMBOL(hermes_bap_pwrite);

int hermes_read_ltv(hermes_t *hw, int bap, u16 rid, unsigned bufsize,
		    u16 *length, void *buf)
{
	int err = 0;
	int dreg = bap ? HERMES_DATA1 : HERMES_DATA0;
	u16 rlength, rtype;
	unsigned nwords;

	if ( (bufsize < 0) || (bufsize % 2) )
		return -EINVAL;

	err = hermes_docmd_wait(hw, HERMES_CMD_ACCESS, rid, NULL);
	if (err)
		return err;

	err = hermes_bap_seek(hw, bap, rid, 0);
	if (err)
		return err;

	rlength = hermes_read_reg(hw, dreg);

	if (! rlength)
		return -ENODATA;

	rtype = hermes_read_reg(hw, dreg);

	if (length)
		*length = rlength;

	if (rtype != rid)
		printk(KERN_WARNING "hermes @ %p: %s(): "
		       "rid (0x%04x) does not match type (0x%04x)\n",
		       hw->iobase, __func__, rid, rtype);
	if (HERMES_RECLEN_TO_BYTES(rlength) > bufsize)
		printk(KERN_WARNING "hermes @ %p: "
		       "Truncating LTV record from %d to %d bytes. "
		       "(rid=0x%04x, len=0x%04x)\n", hw->iobase,
		       HERMES_RECLEN_TO_BYTES(rlength), bufsize, rid, rlength);

	nwords = min((unsigned)rlength - 1, bufsize / 2);
	hermes_read_words(hw, dreg, buf, nwords);

	return 0;
}
EXPORT_SYMBOL(hermes_read_ltv);

int hermes_write_ltv(hermes_t *hw, int bap, u16 rid, 
		     u16 length, const void *value)
{
	int dreg = bap ? HERMES_DATA1 : HERMES_DATA0;
	int err = 0;
	unsigned count;

	if (length == 0)
		return -EINVAL;

	err = hermes_bap_seek(hw, bap, rid, 0);
	if (err)
		return err;

	hermes_write_reg(hw, dreg, length);
	hermes_write_reg(hw, dreg, rid);

	count = length - 1;

	hermes_write_bytes(hw, dreg, value, count << 1);

	err = hermes_docmd_wait(hw, HERMES_CMD_ACCESS | HERMES_CMD_WRITE,
				rid, NULL);

	return err;
}
EXPORT_SYMBOL(hermes_write_ltv);

static int __init init_hermes(void)
{
	return 0;
}

static void __exit exit_hermes(void)
{
}

module_init(init_hermes);
module_exit(exit_hermes);
