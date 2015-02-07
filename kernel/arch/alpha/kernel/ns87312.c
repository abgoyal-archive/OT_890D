

#include <linux/init.h>
#include <asm/io.h>
#include "proto.h"



void __init
ns87312_enable_ide(long ide_base)
{
	int data;
	unsigned long flags;

	local_irq_save(flags);
	outb(0, ide_base);		/* set the index register for reg #0 */
	data = inb(ide_base+1);		/* read the current contents */
	outb(0, ide_base);		/* set the index register for reg #0 */
	outb(data | 0x40, ide_base+1);	/* turn on IDE */
	outb(data | 0x40, ide_base+1);	/* turn on IDE, really! */
	local_irq_restore(flags);
}
