

#include <linux/module.h>


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/io.h>
#include <asm/system.h>

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>
#include "hisax_cfg.h"

MODULE_DESCRIPTION("ISDN4Linux: PCMCIA client driver for AVM A1/Fritz!PCMCIA cards");
MODULE_AUTHOR("Carsten Paeth");
MODULE_LICENSE("GPL");

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
module_param(pc_debug, int, 0);
#define DEBUG(n, args...) if (pc_debug>(n)) printk(KERN_DEBUG args);
static char *version =
"avma1_cs.c 1.00 1998/01/23 10:00:00 (Carsten Paeth)";
#else
#define DEBUG(n, args...)
#endif

/*====================================================================*/

/* Parameters that can be set with 'insmod' */

static int isdnprot = 2;

module_param(isdnprot, int, 0);

/*====================================================================*/


static int avma1cs_config(struct pcmcia_device *link);
static void avma1cs_release(struct pcmcia_device *link);


static void avma1cs_detach(struct pcmcia_device *p_dev);



   
typedef struct local_info_t {
    dev_node_t	node;
} local_info_t;


static int avma1cs_probe(struct pcmcia_device *p_dev)
{
    local_info_t *local;

    DEBUG(0, "avma1cs_attach()\n");

    /* Allocate space for private device-specific data */
    local = kzalloc(sizeof(local_info_t), GFP_KERNEL);
    if (!local)
	return -ENOMEM;

    p_dev->priv = local;

    /* The io structure describes IO port mapping */
    p_dev->io.NumPorts1 = 16;
    p_dev->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
    p_dev->io.NumPorts2 = 16;
    p_dev->io.Attributes2 = IO_DATA_PATH_WIDTH_16;
    p_dev->io.IOAddrLines = 5;

    /* Interrupt setup */
    p_dev->irq.Attributes = IRQ_TYPE_EXCLUSIVE;
    p_dev->irq.Attributes = IRQ_TYPE_DYNAMIC_SHARING|IRQ_FIRST_SHARED;

    p_dev->irq.IRQInfo1 = IRQ_LEVEL_ID;

    /* General socket configuration */
    p_dev->conf.Attributes = CONF_ENABLE_IRQ;
    p_dev->conf.IntType = INT_MEMORY_AND_IO;
    p_dev->conf.ConfigIndex = 1;
    p_dev->conf.Present = PRESENT_OPTION;

    return avma1cs_config(p_dev);
} /* avma1cs_attach */


static void avma1cs_detach(struct pcmcia_device *link)
{
	DEBUG(0, "avma1cs_detach(0x%p)\n", link);
	avma1cs_release(link);
	kfree(link->priv);
} /* avma1cs_detach */


static int avma1cs_configcheck(struct pcmcia_device *p_dev,
			       cistpl_cftable_entry_t *cf,
			       cistpl_cftable_entry_t *dflt,
			       unsigned int vcc,
			       void *priv_data)
{
	if (cf->io.nwin <= 0)
		return -ENODEV;

	p_dev->io.BasePort1 = cf->io.win[0].base;
	p_dev->io.NumPorts1 = cf->io.win[0].len;
	p_dev->io.NumPorts2 = 0;
	printk(KERN_INFO "avma1_cs: testing i/o %#x-%#x\n",
	       p_dev->io.BasePort1,
	       p_dev->io.BasePort1+p_dev->io.NumPorts1-1);
	return pcmcia_request_io(p_dev, &p_dev->io);
}


static int avma1cs_config(struct pcmcia_device *link)
{
    local_info_t *dev;
    int i;
    char devname[128];
    IsdnCard_t	icard;
    int busy = 0;

    dev = link->priv;

    DEBUG(0, "avma1cs_config(0x%p)\n", link);

    devname[0] = 0;
    if (link->prod_id[1])
	    strlcpy(devname, link->prod_id[1], sizeof(devname));

    if (pcmcia_loop_config(link, avma1cs_configcheck, NULL))
	    return -ENODEV;

    do {
	/*
	 * allocate an interrupt line
	 */
	i = pcmcia_request_irq(link, &link->irq);
	if (i != 0) {
	    cs_error(link, RequestIRQ, i);
	    /* undo */
	    pcmcia_disable_device(link);
	    break;
	}

	/*
	 * configure the PCMCIA socket
	 */
	i = pcmcia_request_configuration(link, &link->conf);
	if (i != 0) {
	    cs_error(link, RequestConfiguration, i);
	    pcmcia_disable_device(link);
	    break;
	}

    } while (0);

    /* At this point, the dev_node_t structure(s) should be
       initialized and arranged in a linked list at link->dev. */

    strcpy(dev->node.dev_name, "A1");
    dev->node.major = 45;
    dev->node.minor = 0;
    link->dev_node = &dev->node;

    /* If any step failed, release any partially configured state */
    if (i != 0) {
	avma1cs_release(link);
	return -ENODEV;
    }

    printk(KERN_NOTICE "avma1_cs: checking at i/o %#x, irq %d\n",
				link->io.BasePort1, link->irq.AssignedIRQ);

    icard.para[0] = link->irq.AssignedIRQ;
    icard.para[1] = link->io.BasePort1;
    icard.protocol = isdnprot;
    icard.typ = ISDN_CTYPE_A1_PCMCIA;
    
    i = hisax_init_pcmcia(link, &busy, &icard);
    if (i < 0) {
    	printk(KERN_ERR "avma1_cs: failed to initialize AVM A1 PCMCIA %d at i/o %#x\n", i, link->io.BasePort1);
	avma1cs_release(link);
	return -ENODEV;
    }
    dev->node.minor = i;

    return 0;
} /* avma1cs_config */


static void avma1cs_release(struct pcmcia_device *link)
{
	local_info_t *local = link->priv;

	DEBUG(0, "avma1cs_release(0x%p)\n", link);

	/* now unregister function with hisax */
	HiSax_closecard(local->node.minor);

	pcmcia_disable_device(link);
} /* avma1cs_release */


static struct pcmcia_device_id avma1cs_ids[] = {
	PCMCIA_DEVICE_PROD_ID12("AVM", "ISDN A", 0x95d42008, 0xadc9d4bb),
	PCMCIA_DEVICE_PROD_ID12("ISDN", "CARD", 0x8d9761c8, 0x01c5aa7b),
	PCMCIA_DEVICE_NULL
};
MODULE_DEVICE_TABLE(pcmcia, avma1cs_ids);

static struct pcmcia_driver avma1cs_driver = {
	.owner		= THIS_MODULE,
	.drv		= {
		.name	= "avma1_cs",
	},
	.probe		= avma1cs_probe,
	.remove		= avma1cs_detach,
	.id_table	= avma1cs_ids,
};

/*====================================================================*/

static int __init init_avma1_cs(void)
{
	return(pcmcia_register_driver(&avma1cs_driver));
}

static void __exit exit_avma1_cs(void)
{
	pcmcia_unregister_driver(&avma1cs_driver);
}

module_init(init_avma1_cs);
module_exit(exit_avma1_cs);
