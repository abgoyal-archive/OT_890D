
/* $Id$ */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/system.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/cisreg.h>
#include <pcmcia/ds.h>
#include "hisax_cfg.h"

MODULE_DESCRIPTION("ISDN4Linux: PCMCIA client driver for Teles PCMCIA cards");
MODULE_AUTHOR("Christof Petig, christof.petig@wtal.de, Karsten Keil, kkeil@suse.de");
MODULE_LICENSE("GPL");


#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
module_param(pc_debug, int, 0);
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args);
static char *version =
"teles_cs.c 2.10 2002/07/30 22:23:34 kkeil";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

static int protocol = 2;        /* EURO-ISDN Default */
module_param(protocol, int, 0);

/*====================================================================*/


static int teles_cs_config(struct pcmcia_device *link);
static void teles_cs_release(struct pcmcia_device *link);


static void teles_detach(struct pcmcia_device *p_dev);



typedef struct local_info_t {
	struct pcmcia_device	*p_dev;
    dev_node_t          node;
    int                 busy;
    int			cardnr;
} local_info_t;


static int teles_probe(struct pcmcia_device *link)
{
    local_info_t *local;

    DEBUG(0, "teles_attach()\n");

    /* Allocate space for private device-specific data */
    local = kzalloc(sizeof(local_info_t), GFP_KERNEL);
    if (!local) return -ENOMEM;
    local->cardnr = -1;

    local->p_dev = link;
    link->priv = local;

    /* Interrupt setup */
    link->irq.Attributes = IRQ_TYPE_DYNAMIC_SHARING|IRQ_FIRST_SHARED;
    link->irq.IRQInfo1 = IRQ_LEVEL_ID|IRQ_SHARE_ID;
    link->irq.Handler = NULL;

    /*
      General socket configuration defaults can go here.  In this
      client, we assume very little, and rely on the CIS for almost
      everything.  In most clients, many details (i.e., number, sizes,
      and attributes of IO windows) are fixed by the nature of the
      device, and can be hard-wired here.
    */
    link->io.NumPorts1 = 96;
    link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
    link->io.IOAddrLines = 5;

    link->conf.Attributes = CONF_ENABLE_IRQ;
    link->conf.IntType = INT_MEMORY_AND_IO;

    return teles_cs_config(link);
} /* teles_attach */


static void teles_detach(struct pcmcia_device *link)
{
	local_info_t *info = link->priv;

	DEBUG(0, "teles_detach(0x%p)\n", link);

	info->busy = 1;
	teles_cs_release(link);

	kfree(info);
} /* teles_detach */


static int teles_cs_configcheck(struct pcmcia_device *p_dev,
				cistpl_cftable_entry_t *cf,
				cistpl_cftable_entry_t *dflt,
				unsigned int vcc,
				void *priv_data)
{
	int j;

	if ((cf->io.nwin > 0) && cf->io.win[0].base) {
		printk(KERN_INFO "(teles_cs: looks like the 96 model)\n");
		p_dev->io.BasePort1 = cf->io.win[0].base;
		if (!pcmcia_request_io(p_dev, &p_dev->io))
			return 0;
	} else {
		printk(KERN_INFO "(teles_cs: looks like the 97 model)\n");
		for (j = 0x2f0; j > 0x100; j -= 0x10) {
			p_dev->io.BasePort1 = j;
			if (!pcmcia_request_io(p_dev, &p_dev->io))
				return 0;
		}
	}
	return -ENODEV;
}

static int teles_cs_config(struct pcmcia_device *link)
{
    local_info_t *dev;
    int i, last_fn;
    IsdnCard_t icard;

    DEBUG(0, "teles_config(0x%p)\n", link);
    dev = link->priv;

    i = pcmcia_loop_config(link, teles_cs_configcheck, NULL);
    if (i != 0) {
	last_fn = RequestIO;
	goto cs_failed;
    }

    i = pcmcia_request_irq(link, &link->irq);
    if (i != 0) {
        link->irq.AssignedIRQ = 0;
	last_fn = RequestIRQ;
        goto cs_failed;
    }

    i = pcmcia_request_configuration(link, &link->conf);
    if (i != 0) {
      last_fn = RequestConfiguration;
      goto cs_failed;
    }

    /* At this point, the dev_node_t structure(s) should be
       initialized and arranged in a linked list at link->dev. *//*  */
    sprintf(dev->node.dev_name, "teles");
    dev->node.major = dev->node.minor = 0x0;

    link->dev_node = &dev->node;

    /* Finally, report what we've done */
    printk(KERN_INFO "%s: index 0x%02x:",
           dev->node.dev_name, link->conf.ConfigIndex);
    if (link->conf.Attributes & CONF_ENABLE_IRQ)
        printk(", irq %d", link->irq.AssignedIRQ);
    if (link->io.NumPorts1)
        printk(", io 0x%04x-0x%04x", link->io.BasePort1,
               link->io.BasePort1+link->io.NumPorts1-1);
    if (link->io.NumPorts2)
        printk(" & 0x%04x-0x%04x", link->io.BasePort2,
               link->io.BasePort2+link->io.NumPorts2-1);
    printk("\n");

    icard.para[0] = link->irq.AssignedIRQ;
    icard.para[1] = link->io.BasePort1;
    icard.protocol = protocol;
    icard.typ = ISDN_CTYPE_TELESPCMCIA;
    
    i = hisax_init_pcmcia(link, &(((local_info_t*)link->priv)->busy), &icard);
    if (i < 0) {
    	printk(KERN_ERR "teles_cs: failed to initialize Teles PCMCIA %d at i/o %#x\n",
    		i, link->io.BasePort1);
    	teles_cs_release(link);
	return -ENODEV;
    }

    ((local_info_t*)link->priv)->cardnr = i;
    return 0;

cs_failed:
    cs_error(link, last_fn, i);
    teles_cs_release(link);
    return -ENODEV;
} /* teles_cs_config */


static void teles_cs_release(struct pcmcia_device *link)
{
    local_info_t *local = link->priv;

    DEBUG(0, "teles_cs_release(0x%p)\n", link);

    if (local) {
    	if (local->cardnr >= 0) {
    	    /* no unregister function with hisax */
	    HiSax_closecard(local->cardnr);
	}
    }

    pcmcia_disable_device(link);
} /* teles_cs_release */

static int teles_suspend(struct pcmcia_device *link)
{
	local_info_t *dev = link->priv;

        dev->busy = 1;

	return 0;
}

static int teles_resume(struct pcmcia_device *link)
{
	local_info_t *dev = link->priv;

        dev->busy = 0;

	return 0;
}


static struct pcmcia_device_id teles_ids[] = {
	PCMCIA_DEVICE_PROD_ID12("TELES", "S0/PC", 0x67b50eae, 0xe9e70119),
	PCMCIA_DEVICE_NULL,
};
MODULE_DEVICE_TABLE(pcmcia, teles_ids);

static struct pcmcia_driver teles_cs_driver = {
	.owner		= THIS_MODULE,
	.drv		= {
		.name	= "teles_cs",
	},
	.probe		= teles_probe,
	.remove		= teles_detach,
	.id_table       = teles_ids,
	.suspend	= teles_suspend,
	.resume		= teles_resume,
};

static int __init init_teles_cs(void)
{
	return pcmcia_register_driver(&teles_cs_driver);
}

static void __exit exit_teles_cs(void)
{
	pcmcia_unregister_driver(&teles_cs_driver);
}

module_init(init_teles_cs);
module_exit(exit_teles_cs);
