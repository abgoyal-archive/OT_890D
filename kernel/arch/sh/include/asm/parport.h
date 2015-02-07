
#ifndef __ASM_SH_PARPORT_H
#define __ASM_SH_PARPORT_H

static int __devinit parport_pc_find_isa_ports(int autoirq, int autodma);

static int __devinit parport_pc_find_nonpci_ports(int autoirq, int autodma)
{
	return parport_pc_find_isa_ports(autoirq, autodma);
}

#endif /* __ASM_SH_PARPORT_H */
