

#include "../comedi.h"
#include "../comedilib.h"
#include "../comedidev.h"	/* for comedi_udelay() */

#include <linux/string.h>

int comedi_data_write(comedi_t * dev, unsigned int subdev, unsigned int chan,
	unsigned int range, unsigned int aref, lsampl_t data)
{
	comedi_insn insn;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_WRITE;
	insn.n = 1;
	insn.data = &data;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, range, aref);

	return comedi_do_insn(dev, &insn);
}

int comedi_data_read(comedi_t * dev, unsigned int subdev, unsigned int chan,
	unsigned int range, unsigned int aref, lsampl_t * data)
{
	comedi_insn insn;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_READ;
	insn.n = 1;
	insn.data = data;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, range, aref);

	return comedi_do_insn(dev, &insn);
}

int comedi_data_read_hint(comedi_t * dev, unsigned int subdev,
	unsigned int chan, unsigned int range, unsigned int aref)
{
	comedi_insn insn;
	lsampl_t dummy_data;

	memset(&insn, 0, sizeof(insn));
	insn.insn = INSN_READ;
	insn.n = 0;
	insn.data = &dummy_data;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan, range, aref);

	return comedi_do_insn(dev, &insn);
}

int comedi_data_read_delayed(comedi_t * dev, unsigned int subdev,
	unsigned int chan, unsigned int range, unsigned int aref,
	lsampl_t * data, unsigned int nano_sec)
{
	int retval;

	retval = comedi_data_read_hint(dev, subdev, chan, range, aref);
	if (retval < 0)
		return retval;

	comedi_udelay((nano_sec + 999) / 1000);

	return comedi_data_read(dev, subdev, chan, range, aref, data);
}
