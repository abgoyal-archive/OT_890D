

#include "../comedi.h"
#include "../comedilib.h"

#include <linux/string.h>

int comedi_dio_config(comedi_t * dev, unsigned int subdev, unsigned int chan,
	unsigned int io)
{
	comedi_insn insn;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_CONFIG;
	insn.n = 1;
	insn.data = &io;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, 0, 0);

	return comedi_do_insn(dev, &insn);
}

int comedi_dio_read(comedi_t * dev, unsigned int subdev, unsigned int chan,
	unsigned int *val)
{
	comedi_insn insn;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_READ;
	insn.n = 1;
	insn.data = val;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, 0, 0);

	return comedi_do_insn(dev, &insn);
}

int comedi_dio_write(comedi_t * dev, unsigned int subdev, unsigned int chan,
	unsigned int val)
{
	comedi_insn insn;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_WRITE;
	insn.n = 1;
	insn.data = &val;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, 0, 0);

	return comedi_do_insn(dev, &insn);
}

int comedi_dio_bitfield(comedi_t * dev, unsigned int subdev, unsigned int mask,
	unsigned int *bits)
{
	comedi_insn insn;
	lsampl_t data[2];
	int ret;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_BITS;
	insn.n = 2;
	insn.data = data;
	insn.subdev = subdev;

	data[0] = mask;
	data[1] = *bits;

	ret = comedi_do_insn(dev, &insn);

	*bits = data[1];

	return ret;
}
