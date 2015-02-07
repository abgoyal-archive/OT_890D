


#ifndef __KERNEL__
# define __KERNEL__
#endif

#ifndef KBUILD_MODNAME
#  define KBUILD_MODNAME KBUILD_STR(mefirmware)
#endif

#include <linux/pci.h>
#include <linux/delay.h>

#include <linux/firmware.h>

#include "meplx_reg.h"
#include "medebug.h"

#include "mefirmware.h"

int me_xilinx_download(unsigned long register_base_control,
		       unsigned long register_base_data,
		       struct device *dev, const char *firmware_name)
{
	int err = ME_ERRNO_FIRMWARE;
	uint32_t value = 0;
	int idx = 0;

	const struct firmware *fw;

	PDEBUG("executed.\n");

	if (!firmware_name) {
		PERROR("Request for firmware failed. No name provided. \n");
		return err;
	}

	PINFO("Request '%s' firmware.\n", firmware_name);
	err = request_firmware(&fw, firmware_name, dev);

	if (err) {
		PERROR("Request for firmware failed.\n");
		return err;
	}
	// Set PLX local interrupt 2 polarity to high.
	// Interrupt is thrown by init pin of xilinx.
	outl(PLX_INTCSR_LOCAL_INT2_POL, register_base_control + PLX_INTCSR);

	// Set /CS and /WRITE of the Xilinx
	value = inl(register_base_control + PLX_ICR);
	value |= ME_FIRMWARE_CS_WRITE;
	outl(value, register_base_control + PLX_ICR);

	// Init Xilinx with CS1
	inl(register_base_data + ME_XILINX_CS1_REG);

	// Wait for init to complete
	udelay(20);

	// Checkl /INIT pin
	if (!
	    (inl(register_base_control + PLX_INTCSR) &
	     PLX_INTCSR_LOCAL_INT2_STATE)) {
		PERROR("Can't init Xilinx.\n");
		release_firmware(fw);
		return -EIO;
	}
	// Reset /CS and /WRITE of the Xilinx
	value = inl(register_base_control + PLX_ICR);
	value &= ~ME_FIRMWARE_CS_WRITE;
	outl(value, register_base_control + PLX_ICR);

	// Download Xilinx firmware
	udelay(10);

	for (idx = 0; idx < fw->size; idx++) {
		outl(fw->data[idx], register_base_data);
#ifdef ME6000_v2_4
///     This checking only for board's version 2.4
		// Check if BUSY flag is set (low = ready, high = busy)
		if (inl(register_base_control + PLX_ICR) &
		    ME_FIRMWARE_BUSY_FLAG) {
			PERROR("Xilinx is still busy (idx = %d)\n", idx);
			release_firmware(fw);
			return -EIO;
		}
#endif //ME6000_v2_4
	}
	PDEBUG("Download finished. %d bytes written to PLX.\n", idx);

	// If done flag is high download was successful
	if (inl(register_base_control + PLX_ICR) & ME_FIRMWARE_DONE_FLAG) {
		PDEBUG("SUCCESS. Done flag is set.\n");
	} else {
		PERROR("FAILURE. DONE flag is not set.\n");
		release_firmware(fw);
		return -EIO;
	}

	// Set /CS and /WRITE
	value = inl(register_base_control + PLX_ICR);
	value |= ME_FIRMWARE_CS_WRITE;
	outl(value, register_base_control + PLX_ICR);

	PDEBUG("Enable interrupts on the PCI interface.\n");
	outl(ME_PLX_PCI_ACTIVATE, register_base_control + PLX_INTCSR);
	release_firmware(fw);

	return 0;
}
